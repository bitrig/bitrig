/* $OpenBSD: ssh-pkcs11.c,v 1.14 2014/06/24 01:13:21 djm Exp $ */
/*
 * Copyright (c) 2010 Markus Friedl.  All rights reserved.
 * Copyright (c) 2014 Pedro Martelletto. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/queue.h>
#include <stdarg.h>
#include <stdio.h>

#include <string.h>
#include <dlfcn.h>

#include <openssl/ecdsa.h>
#include <openssl/x509.h>

#define CRYPTOKI_COMPAT
#include "pkcs11.h"

#include "log.h"
#include "misc.h"
#include "key.h"
#include "ssh-pkcs11.h"
#include "xmalloc.h"

struct pkcs11_slotinfo {
	CK_TOKEN_INFO		token;
	CK_SESSION_HANDLE	session;
	int			logged_in;
};

struct pkcs11_provider {
	char			*name;
	void			*handle;
	CK_FUNCTION_LIST	*function_list;
	CK_INFO			info;
	CK_ULONG		nslots;
	CK_SLOT_ID		*slotlist;
	struct pkcs11_slotinfo	*slotinfo;
	int			valid;
	int			refcount;
	TAILQ_ENTRY(pkcs11_provider) next;
};

TAILQ_HEAD(, pkcs11_provider) pkcs11_providers;

struct pkcs11_key {
	struct pkcs11_provider	*provider;
	CK_ULONG		slotidx;
	int			(*orig_finish)(RSA *rsa);
	RSA_METHOD		rsa_method;
	ECDSA_METHOD		*ecdsa_method;
	char			*keyid;
	int			keyid_len;
};

int pkcs11_interactive = 0;

int
pkcs11_init(int interactive)
{
	pkcs11_interactive = interactive;
	TAILQ_INIT(&pkcs11_providers);
	return (0);
}

/*
 * finalize a provider shared libarary, it's no longer usable.
 * however, there might still be keys referencing this provider,
 * so the actuall freeing of memory is handled by pkcs11_provider_unref().
 * this is called when a provider gets unregistered.
 */
static void
pkcs11_provider_finalize(struct pkcs11_provider *p)
{
	CK_RV rv;
	CK_ULONG i;

	debug("pkcs11_provider_finalize: %p refcount %d valid %d",
	    p, p->refcount, p->valid);
	if (!p->valid)
		return;
	for (i = 0; i < p->nslots; i++) {
		if (p->slotinfo[i].session &&
		    (rv = p->function_list->C_CloseSession(
		    p->slotinfo[i].session)) != CKR_OK)
			error("C_CloseSession failed: %lu", rv);
	}
	if ((rv = p->function_list->C_Finalize(NULL)) != CKR_OK)
		error("C_Finalize failed: %lu", rv);
	p->valid = 0;
	p->function_list = NULL;
#ifdef HAVE_DLOPEN
	dlclose(p->handle);
#endif
}

/*
 * remove a reference to the provider.
 * called when a key gets destroyed or when the provider is unregistered.
 */
static void
pkcs11_provider_unref(struct pkcs11_provider *p)
{
	debug("pkcs11_provider_unref: %p refcount %d", p, p->refcount);
	if (--p->refcount <= 0) {
		if (p->valid)
			error("pkcs11_provider_unref: %p still valid", p);
		free(p->slotlist);
		free(p->slotinfo);
		free(p);
	}
}

/* unregister all providers, keys might still point to the providers */
void
pkcs11_terminate(void)
{
	struct pkcs11_provider *p;

	while ((p = TAILQ_FIRST(&pkcs11_providers)) != NULL) {
		TAILQ_REMOVE(&pkcs11_providers, p, next);
		pkcs11_provider_finalize(p);
		pkcs11_provider_unref(p);
	}
}

/* lookup provider by name */
static struct pkcs11_provider *
pkcs11_provider_lookup(char *provider_id)
{
	struct pkcs11_provider *p;

	TAILQ_FOREACH(p, &pkcs11_providers, next) {
		debug("check %p %s", p, p->name);
		if (!strcmp(provider_id, p->name))
			return (p);
	}
	return (NULL);
}

/* unregister provider by name */
int
pkcs11_del_provider(char *provider_id)
{
	struct pkcs11_provider *p;

	if ((p = pkcs11_provider_lookup(provider_id)) != NULL) {
		TAILQ_REMOVE(&pkcs11_providers, p, next);
		pkcs11_provider_finalize(p);
		pkcs11_provider_unref(p);
		return (0);
	}
	return (-1);
}

/* openssl callback for freeing an RSA key */
static int
pkcs11_rsa_finish(RSA *rsa)
{
	struct pkcs11_key	*k11;
	int rv = -1;

	if ((k11 = RSA_get_app_data(rsa)) != NULL) {
		if (k11->orig_finish)
			rv = k11->orig_finish(rsa);
		if (k11->provider)
			pkcs11_provider_unref(k11->provider);
		free(k11->keyid);
		free(k11);
	}
	return (rv);
}

/* find a single 'obj' for given attributes */
static int
pkcs11_find(struct pkcs11_provider *p, CK_ULONG slotidx, CK_ATTRIBUTE *attr,
    CK_ULONG nattr, CK_OBJECT_HANDLE *obj)
{
	CK_FUNCTION_LIST	*f;
	CK_SESSION_HANDLE	session;
	CK_ULONG		nfound = 0;
	CK_RV			rv;
	int			ret = -1;

	f = p->function_list;
	session = p->slotinfo[slotidx].session;
	if ((rv = f->C_FindObjectsInit(session, attr, nattr)) != CKR_OK) {
		error("C_FindObjectsInit failed (nattr %lu): %lu", nattr, rv);
		return (-1);
	}
	if ((rv = f->C_FindObjects(session, obj, 1, &nfound)) != CKR_OK ||
	    nfound != 1) {
		debug("C_FindObjects failed (nfound %lu nattr %lu): %lu",
		    nfound, nattr, rv);
	} else
		ret = 0;
	if ((rv = f->C_FindObjectsFinal(session)) != CKR_OK)
		error("C_FindObjectsFinal failed: %lu", rv);
	return (ret);
}

static int
pkcs11_get_key(struct pkcs11_key *k11, CK_MECHANISM_TYPE mech_type)
{
	struct pkcs11_slotinfo	*si;
	CK_FUNCTION_LIST	*f;
	CK_OBJECT_HANDLE	 obj;
	CK_RV			 rv;
	CK_OBJECT_CLASS		 private_key_class;
	CK_BBOOL		 true_val;
	CK_MECHANISM		 mech;
	CK_ATTRIBUTE		 key_filter[3];
	char			*pin, prompt[1024];

	if (!k11->provider || !k11->provider->valid) {
		error("no pkcs11 (valid) provider found");
		return (-1);
	}

	f = k11->provider->function_list;
	si = &k11->provider->slotinfo[k11->slotidx];

	if ((si->token.flags & CKF_LOGIN_REQUIRED) && !si->logged_in) {
		if (!pkcs11_interactive) {
			error("need pin");
			return (-1);
		}
		snprintf(prompt, sizeof(prompt), "Enter PIN for '%s': ",
		    si->token.label);
		pin = read_passphrase(prompt, RP_ALLOW_EOF);
		if (pin == NULL)
			return (-1);	/* bail out */
		if ((rv = f->C_Login(si->session, CKU_USER,
		    (u_char *)pin, strlen(pin))) != CKR_OK) {
			free(pin);
			error("C_Login failed: %lu", rv);
			return (-1);
		}
		free(pin);
		si->logged_in = 1;
	}

	memset(&key_filter, 0, sizeof(key_filter));
	private_key_class = CKO_PRIVATE_KEY;
	key_filter[0].type = CKA_CLASS;
	key_filter[0].pValue = &private_key_class;
	key_filter[0].ulValueLen = sizeof(private_key_class);

	key_filter[1].type = CKA_ID;
	key_filter[1].pValue = k11->keyid;
	key_filter[1].ulValueLen = k11->keyid_len;

	true_val = CK_TRUE;
	key_filter[2].type = CKA_SIGN;
	key_filter[2].pValue = &true_val;
	key_filter[2].ulValueLen = sizeof(true_val);

	/* try to find object w/CKA_SIGN first, retry w/o */
	if (pkcs11_find(k11->provider, k11->slotidx, key_filter, 3, &obj) < 0 &&
	    pkcs11_find(k11->provider, k11->slotidx, key_filter, 2, &obj) < 0) {
		error("cannot find private key");
		return (-1);
	}

	memset(&mech, 0, sizeof(mech));
	mech.mechanism = mech_type;
	mech.pParameter = NULL_PTR;
	mech.ulParameterLen = 0;

	if ((rv = f->C_SignInit(si->session, &mech, obj)) != CKR_OK) {
		error("C_SignInit failed: %lu", rv);
		return (-1);
	}

	return (0);
}

/* openssl callback doing the actual signing operation */
static int
pkcs11_rsa_private_encrypt(int flen, const u_char *from, u_char *to, RSA *rsa,
    int padding)
{
	struct pkcs11_key	*k11;
	struct pkcs11_slotinfo	*si;
	CK_FUNCTION_LIST	*f;
	CK_ULONG		tlen = 0;
	CK_RV			rv;
	int			rval = -1;

	if ((k11 = RSA_get_app_data(rsa)) == NULL) {
		error("RSA_get_app_data failed for rsa %p", rsa);
		return (-1);
	}

	if (pkcs11_get_key(k11, CKM_RSA_PKCS) == -1) {
		error("pkcs11_get_key failed");
		return (-1);
	}

	f = k11->provider->function_list;
	si = &k11->provider->slotinfo[k11->slotidx];
	tlen = RSA_size(rsa);

	/* XXX handle CKR_BUFFER_TOO_SMALL */
	rv = f->C_Sign(si->session, (CK_BYTE *)from, flen, to, &tlen);
	if (rv == CKR_OK)
		rval = tlen;
	else
		error("C_Sign failed: %lu", rv);

	return (rval);
}

static int
pkcs11_rsa_private_decrypt(int flen, const u_char *from, u_char *to, RSA *rsa,
    int padding)
{
	return (-1);
}

/* redirect private key operations for rsa key to pkcs11 token */
static int
pkcs11_rsa_wrap(struct pkcs11_provider *provider, CK_ULONG slotidx,
    CK_ATTRIBUTE *keyid_attrib, RSA *rsa)
{
	struct pkcs11_key	*k11;
	const RSA_METHOD	*def = RSA_get_default_method();

	k11 = xcalloc(1, sizeof(*k11));
	k11->provider = provider;
	provider->refcount++;	/* provider referenced by RSA key */
	k11->slotidx = slotidx;
	/* identify key object on smartcard */
	k11->keyid_len = keyid_attrib->ulValueLen;
	k11->keyid = xmalloc(k11->keyid_len);
	memcpy(k11->keyid, keyid_attrib->pValue, k11->keyid_len);
	k11->orig_finish = def->finish;
	memcpy(&k11->rsa_method, def, sizeof(k11->rsa_method));
	k11->rsa_method.name = "pkcs11";
	k11->rsa_method.rsa_priv_enc = pkcs11_rsa_private_encrypt;
	k11->rsa_method.rsa_priv_dec = pkcs11_rsa_private_decrypt;
	k11->rsa_method.finish = pkcs11_rsa_finish;
	RSA_set_method(rsa, &k11->rsa_method);
	RSA_set_app_data(rsa, k11);
	return (0);
}

/* ~*kingdom of unimplemented callbacks*~ */

static void *
ecdsa_k11_dup(void *k11)
{
	fatal("%s called", __func__);
}

static void
ecdsa_k11_free(void *k11)
{
	fatal("%s called", __func__);
}

static void
ecdsa_k11_clear_free(void *k11)
{
	fatal("%s called", __func__);
}

static int
ecdsa_sign_setup(EC_KEY *ec, BN_CTX *ctx, BIGNUM **kinvp, BIGNUM **rp)
{
	error("%s called, returning -1", __func__);
	return (-1);
}

/* openssl callback doing the actual signing operation */
static ECDSA_SIG *
ecdsa_do_sign(const unsigned char *dgst, int dgst_len, const BIGNUM *inv,
    const BIGNUM *rp, EC_KEY *ec)
{
	struct pkcs11_key	*k11;
	struct pkcs11_slotinfo	*si;
	CK_FUNCTION_LIST	*f;
	CK_ULONG		siglen = 0;
	CK_RV			rv;
	ECDSA_SIG		*ret = NULL;
	u_char			*sig;
	const u_char		*cp;

	if ((k11 = EC_KEY_get_key_method_data(ec, ecdsa_k11_dup, ecdsa_k11_free,
	    ecdsa_k11_clear_free)) == NULL) {
		error("EC_KEY_get_key_method_data failed for ec %p", ec);
		return (NULL);
	}

	if (pkcs11_get_key(k11, CKM_ECDSA) == -1) {
		error("pkcs11_get_key failed");
		return (NULL);
	}

	f = k11->provider->function_list;
	si = &k11->provider->slotinfo[k11->slotidx];

	siglen = ECDSA_size(ec);
	sig = xmalloc(siglen);

	/* XXX handle CKR_BUFFER_TOO_SMALL */
	rv = f->C_Sign(si->session, (CK_BYTE *)dgst, dgst_len, sig, &siglen);
	if (rv == CKR_OK) {
		cp = sig;
		ret = d2i_ECDSA_SIG(NULL, &cp, siglen);
	} else
		error("C_Sign failed: %lu", rv);

	free(sig);

	return (ret);
}

static int
ecdsa_do_verify(const unsigned char *dgst, int dgst_len, const ECDSA_SIG *sig,
    EC_KEY *ec)
{
	error("%s called, returning -1", __func__);
	return (-1);
}

static ECDSA_METHOD *ecdsa_method;

static int
pkcs11_ecdsa_start_wrapper(void)
{
	if (ecdsa_method != NULL)
		return (0);

	ecdsa_method = ECDSA_METHOD_new(NULL);
	if (ecdsa_method == NULL) {
		error("ECDSA_METHOD_new() failed");
		return (-1);
	}

	ECDSA_METHOD_set_sign_setup(ecdsa_method, ecdsa_sign_setup);
	ECDSA_METHOD_set_sign(ecdsa_method, ecdsa_do_sign);
	ECDSA_METHOD_set_verify(ecdsa_method, ecdsa_do_verify);

	return (0);
}

static int
pkcs11_ecdsa_wrap(struct pkcs11_provider *provider, CK_ULONG slotidx,
    CK_ATTRIBUTE *keyid_attrib, EC_KEY *ec)
{
	struct pkcs11_key	*k11;

	if (pkcs11_ecdsa_start_wrapper() == -1)
		return (-1);

	k11 = xcalloc(1, sizeof(*k11));
	k11->provider = provider;
	provider->refcount++;	/* provider referenced by ECDSA key */
	k11->slotidx = slotidx;
	/* identify key object on smartcard */
	k11->keyid_len = keyid_attrib->ulValueLen;
	k11->keyid = xmalloc(k11->keyid_len);
	memcpy(k11->keyid, keyid_attrib->pValue, k11->keyid_len);
	k11->ecdsa_method = ecdsa_method;

	ECDSA_set_method(ec, k11->ecdsa_method);
	EC_KEY_insert_key_method_data(ec, k11, ecdsa_k11_dup, ecdsa_k11_free,
	    ecdsa_k11_clear_free);

	return (0);
}

/* remove trailing spaces */
static void
rmspace(u_char *buf, size_t len)
{
	size_t i;

	if (!len)
		return;
	for (i = len - 1;  i > 0; i--)
		if (i == len - 1 || buf[i] == ' ')
			buf[i] = '\0';
		else
			break;
}

/*
 * open a pkcs11 session and login if required.
 * if pin == NULL we delay login until key use
 */
static int
pkcs11_open_session(struct pkcs11_provider *p, CK_ULONG slotidx, char *pin)
{
	CK_RV			rv;
	CK_FUNCTION_LIST	*f;
	CK_SESSION_HANDLE	session;
	int			login_required;

	f = p->function_list;
	login_required = p->slotinfo[slotidx].token.flags & CKF_LOGIN_REQUIRED;
	if (pin && login_required && !strlen(pin)) {
		error("pin required");
		return (-1);
	}
	if ((rv = f->C_OpenSession(p->slotlist[slotidx], CKF_RW_SESSION|
	    CKF_SERIAL_SESSION, NULL, NULL, &session))
	    != CKR_OK) {
		error("C_OpenSession failed: %lu", rv);
		return (-1);
	}
	if (login_required && pin) {
		if ((rv = f->C_Login(session, CKU_USER,
		    (u_char *)pin, strlen(pin))) != CKR_OK) {
			error("C_Login failed: %lu", rv);
			if ((rv = f->C_CloseSession(session)) != CKR_OK)
				error("C_CloseSession failed: %lu", rv);
			return (-1);
		}
		p->slotinfo[slotidx].logged_in = 1;
	}
	p->slotinfo[slotidx].session = session;
	return (0);
}

static int
pkcs11_key_included(Key ***keysp, int *nkeys, Key *key)
{
	int i;

	for (i = 0; i < *nkeys; i++)
		if (key_equal(key, (*keysp)[i]))
			return (1);
	return (0);
}

static Key *
pkcs11_fetch_ecdsa_pubkey(struct pkcs11_provider *p, CK_ULONG slotidx,
    CK_OBJECT_HANDLE *obj)
{
	CK_ATTRIBUTE		 key_attr[3];
	CK_SESSION_HANDLE	 session;
	CK_FUNCTION_LIST	*f = NULL;
	CK_RV			 rv;
	EC_KEY			*ec = NULL;
	EC_GROUP		*group = NULL;
	Key			*key = NULL;
	const unsigned char	*attrp = NULL;
	int			 i;
	int			 nid;

	memset(&key_attr, 0, sizeof(key_attr));
	key_attr[0].type = CKA_ID;
	key_attr[1].type = CKA_EC_POINT;
	key_attr[2].type = CKA_EC_PARAMS;

	session = p->slotinfo[slotidx].session;
	f = p->function_list;

	/* figure out size of the attributes */
	rv = f->C_GetAttributeValue(session, *obj, key_attr, 3);
	if (rv != CKR_OK) {
		error("C_GetAttributeValue failed: %lu", rv);
		return (NULL);
	}

	/* check that none of the attributes are zero length */
	if (key_attr[0].ulValueLen == 0 ||
	    key_attr[1].ulValueLen == 0 ||
	    key_attr[2].ulValueLen == 0) {
		error("invalid attribute length");
		return (NULL);
	}

	/* allocate buffers for attributes */
	for (i = 0; i < 3; i++)
		key_attr[i].pValue = xcalloc(1, key_attr[i].ulValueLen);

	/* retrieve ID, public point and curve parameters of EC key */
	rv = f->C_GetAttributeValue(session, *obj, key_attr, 3);
	if (rv != CKR_OK) {
		error("C_GetAttributeValue failed: %lu", rv);
		goto fail;
	}

	ec = EC_KEY_new();
	if (ec == NULL) {
		error("EC_KEY_new failed");
		goto fail;
	}

	attrp = key_attr[2].pValue;
	group = d2i_ECPKParameters(NULL, &attrp, key_attr[2].ulValueLen);
	if (group == NULL || EC_KEY_set_group(ec, group) == 0) {
		error("d2i_ECPKParameters failed");
		goto fail;
	}

	if (key_attr[1].ulValueLen <= 2) {
		error("CKA_EC_POINT too small");
		goto fail;
	}

	attrp = (const unsigned char *)key_attr[1].pValue + 2;
	if (o2i_ECPublicKey(&ec, &attrp, key_attr[1].ulValueLen - 2) == NULL) {
		error("o2i_ECPublicKey failed");
		goto fail;
	}

	nid = sshkey_ecdsa_key_to_nid(ec);
	if (nid < 0) {
		error("couldn't get curve nid");
		goto fail;
	}

	if (pkcs11_ecdsa_wrap(p, slotidx, &key_attr[0], ec))
		goto fail;

	key = key_new(KEY_UNSPEC);
	if (key == NULL) {
		error("key_new failed");
		goto fail;
	}

	key->ecdsa = ec;
	key->ecdsa_nid = nid;
	key->type = KEY_ECDSA;
	key->flags |= SSHKEY_FLAG_EXT;
	ec = NULL;	/* now owned by key */

fail:
	for (i = 0; i < 3; i++)
		free(key_attr[i].pValue);
	if (ec)
		EC_KEY_free(ec);
	if (group)
		EC_GROUP_free(group);

	return (key);
}

static Key *
pkcs11_fetch_rsa_pubkey(struct pkcs11_provider *p, CK_ULONG slotidx,
    CK_OBJECT_HANDLE *obj)
{
	CK_ATTRIBUTE		 key_attr[3];
	CK_SESSION_HANDLE	 session;
	CK_FUNCTION_LIST	*f = NULL;
	CK_RV			 rv;
	RSA			*rsa = NULL;
	Key			*key = NULL;
	int			 i;

	memset(&key_attr, 0, sizeof(key_attr));
	key_attr[0].type = CKA_ID;
	key_attr[1].type = CKA_MODULUS;
	key_attr[2].type = CKA_PUBLIC_EXPONENT;

	session = p->slotinfo[slotidx].session;
	f = p->function_list;

	/* figure out size of the attributes */
	rv = f->C_GetAttributeValue(session, *obj, key_attr, 3);
	if (rv != CKR_OK) {
		error("C_GetAttributeValue failed: %lu", rv);
		return (NULL);
	}

	/* check that none of the attributes are zero length */
	if (key_attr[0].ulValueLen == 0 ||
	    key_attr[1].ulValueLen == 0 ||
	    key_attr[2].ulValueLen == 0) {
		error("invalid attribute length");
		return (NULL);
	}

	/* allocate buffers for attributes */
	for (i = 0; i < 3; i++)
		key_attr[i].pValue = xcalloc(1, key_attr[i].ulValueLen);

	/* retrieve ID, modulus and public exponent of RSA key */
	rv = f->C_GetAttributeValue(session, *obj, key_attr, 3);
	if (rv != CKR_OK) {
		error("C_GetAttributeValue failed: %lu", rv);
		goto fail;
	}

	rsa = RSA_new();
	if (rsa == NULL) {
		error("RSA_new failed");
		goto fail;
	}

	rsa->n = BN_bin2bn(key_attr[1].pValue, key_attr[1].ulValueLen, NULL);
	rsa->e = BN_bin2bn(key_attr[2].pValue, key_attr[2].ulValueLen, NULL);
	if (rsa->n == NULL || rsa->e == NULL) {
		error("BN_bin2bn failed");
		goto fail;
	}

	if (pkcs11_rsa_wrap(p, slotidx, &key_attr[0], rsa))
		goto fail;

	key = key_new(KEY_UNSPEC);
	if (key == NULL) {
		error("key_new failed");
		goto fail;
	}

	key->rsa = rsa;
	key->type = KEY_RSA;
	key->flags |= SSHKEY_FLAG_EXT;
	rsa = NULL;	/* now owned by key */

fail:
	for (i = 0; i < 3; i++)
		free(key_attr[i].pValue);
	if (rsa)
		RSA_free(rsa);

	return (key);
}

static Key *
pkcs11_fetch_x509_pubkey(struct pkcs11_provider *p, CK_ULONG slotidx,
    CK_OBJECT_HANDLE *obj)
{
	CK_ATTRIBUTE		 cert_attr[3];
	CK_SESSION_HANDLE	 session;
	CK_FUNCTION_LIST	*f = NULL;
	CK_RV			 rv;
	X509			*x509 = NULL;
	EVP_PKEY		*evp;
	RSA			*rsa = NULL;
	EC_KEY			*ec = NULL;
	Key			*key = NULL;
	int			 i;
	int			 nid;
	const u_char		 *cp;

	memset(&cert_attr, 0, sizeof(cert_attr));
	cert_attr[0].type = CKA_ID;
	cert_attr[1].type = CKA_SUBJECT;
	cert_attr[2].type = CKA_VALUE;

	session = p->slotinfo[slotidx].session;
	f = p->function_list;

	/* figure out size of the attributes */
	rv = f->C_GetAttributeValue(session, *obj, cert_attr, 3);
	if (rv != CKR_OK) {
		error("C_GetAttributeValue failed: %lu", rv);
		return (NULL);
	}

	/* check that none of the attributes are zero length */
	if (cert_attr[0].ulValueLen == 0 ||
	    cert_attr[1].ulValueLen == 0 ||
	    cert_attr[2].ulValueLen == 0) {
		error("invalid attribute length");
		return (NULL);
	}

	/* allocate buffers for attributes */
	for (i = 0; i < 3; i++)
		cert_attr[i].pValue = xcalloc(1, cert_attr[i].ulValueLen);

	/* retrieve ID, subject and value of certificate */
	rv = f->C_GetAttributeValue(session, *obj, cert_attr, 3);
	if (rv != CKR_OK) {
		error("C_GetAttributeValue failed: %lu", rv);
		goto fail;
	}

	x509 = X509_new();
	if (x509 == NULL) {
		error("x509_new failed");
		goto fail;
	}

	cp = cert_attr[2].pValue;
	if (d2i_X509(&x509, &cp, cert_attr[2].ulValueLen) == NULL) {
		error("d2i_x509 failed");
		goto fail;
	}

	evp = X509_get_pubkey(x509);
	if (evp == NULL) {
		error("X509_get_pubkey failed");
		goto fail;
	}

	if (evp->type == EVP_PKEY_RSA) {
		if (evp->pkey.rsa == NULL) {
			error("invalid x509; no rsa key");
			goto fail;
		}
		if ((rsa = RSAPublicKey_dup(evp->pkey.rsa)) == NULL) {
			error("RSAPublicKey_dup failed");
			goto fail;
		}

		if (pkcs11_rsa_wrap(p, slotidx, &cert_attr[0], rsa))
			goto fail;

		key = key_new(KEY_UNSPEC);
		if (key == NULL) {
			error("key_new failed");
			goto fail;
		}

		key->rsa = rsa;
		key->type = KEY_RSA;
		key->flags |= SSHKEY_FLAG_EXT;
		rsa = NULL;	/* now owned by key */
	} else if (evp->type == EVP_PKEY_EC) {
		if (evp->pkey.ec == NULL) {
			error("invalid x509; no ec key");
			goto fail;
		}
		if ((ec = EC_KEY_dup(evp->pkey.ec)) == NULL) {
			error("EC_KEY_dup failed");
			goto fail;
		}

		nid = sshkey_ecdsa_key_to_nid(ec);
		if (nid < 0) {
			error("couldn't get curve nid");
			goto fail;
		}

		if (pkcs11_ecdsa_wrap(p, slotidx, &cert_attr[0], ec))
			goto fail;

		key = key_new(KEY_UNSPEC);
		if (key == NULL) {
			error("key_new failed");
			goto fail;
		}

		key->ecdsa = ec;
		key->ecdsa_nid = nid;
		key->type = KEY_ECDSA;
		key->flags |= SSHKEY_FLAG_EXT;
		ec = NULL;	/* now owned by key */
	} else
		error("unknown certificate key type");

fail:
	for (i = 0; i < 3; i++)
		free(cert_attr[i].pValue);
	if (x509)
		X509_free(x509);
	if (rsa)
		RSA_free(rsa);
	if (ec)
		EC_KEY_free(ec);

	return (key);
}

/*
 * lookup certificates for token in slot identified by slotidx,
 * add 'wrapped' public keys to the 'keysp' array and increment nkeys.
 * keysp points to an (possibly empty) array with *nkeys keys.
 */
static int
pkcs11_fetch_certs(struct pkcs11_provider *p, CK_ULONG slotidx, Key ***keysp,
    int *nkeys)
{
	Key			*key = NULL;
	CK_OBJECT_CLASS		 key_class;
	CK_ATTRIBUTE		 key_attr[1];
	CK_SESSION_HANDLE	 session;
	CK_FUNCTION_LIST	*f = NULL;
	CK_RV			 rv;
	CK_OBJECT_HANDLE	 obj;
	CK_ULONG		 n = 0;
	int			 ret = -1;

	memset(&key_attr, 0, sizeof(key_attr));
	memset(&obj, 0, sizeof(obj));

	key_class = CKO_CERTIFICATE;
	key_attr[0].type = CKA_CLASS;
	key_attr[0].pValue = &key_class;
	key_attr[0].ulValueLen = sizeof(key_class);

	session = p->slotinfo[slotidx].session;
	f = p->function_list;

	rv = f->C_FindObjectsInit(session, key_attr, 1);
	if (rv != CKR_OK) {
		error("C_FindObjectsInit failed: %lu", rv);
		goto fail;
	}

	while (1) {
		CK_CERTIFICATE_TYPE	ck_cert_type;

		rv = f->C_FindObjects(session, &obj, 1, &n);
		if (rv != CKR_OK) {
			error("C_FindObjects failed: %lu", rv);
			goto fail;
		}
		if (n == 0)
			break;

		memset(&ck_cert_type, 0, sizeof(ck_cert_type));
		memset(&key_attr, 0, sizeof(key_attr));
		key_attr[0].type = CKA_CERTIFICATE_TYPE;
		key_attr[0].pValue = &ck_cert_type;
		key_attr[0].ulValueLen = sizeof(ck_cert_type);

		rv = f->C_GetAttributeValue(session, obj, key_attr, 1);
		if (rv != CKR_OK) {
			error("C_GetAttributeValue failed: %lu", rv);
			goto fail;
		}

		switch (ck_cert_type) {
		case CKC_X_509:
			key = pkcs11_fetch_x509_pubkey(p, slotidx, &obj);
			break;
		default:
			/* XXX print key type? */
			error("skipping unsupported certificate type");
		}

		if (key == NULL) {
			error("failed to fetch key");
			continue;
		}

		if (pkcs11_key_included(keysp, nkeys, key)) {
			key_free(key);
		} else {
			/* expand key array and add key */
			*keysp = xrealloc(*keysp, *nkeys + 1, sizeof(Key *));
			(*keysp)[*nkeys] = key;
			*nkeys = *nkeys + 1;
			debug("have %d keys", *nkeys);
		}
	}

	ret = 0;
fail:
	rv = f->C_FindObjectsFinal(session);
	if (rv != CKR_OK) {
		error("C_FindObjectsFinal failed: %lu", rv);
		ret = -1;
	}

	return (ret);
}

/*
 * lookup public keys for token in slot identified by slotidx,
 * add 'wrapped' public keys to the 'keysp' array and increment nkeys.
 * keysp points to an (possibly empty) array with *nkeys keys.
 */
static int
pkcs11_fetch_keys(struct pkcs11_provider *p, CK_ULONG slotidx, Key ***keysp,
    int *nkeys)
{
	Key			*key = NULL;
	CK_OBJECT_CLASS		 key_class;
	CK_ATTRIBUTE		 key_attr[1];
	CK_SESSION_HANDLE	 session;
	CK_FUNCTION_LIST	*f = NULL;
	CK_RV			 rv;
	CK_OBJECT_HANDLE	 obj;
	CK_ULONG		 n = 0;
	int			 ret = -1;

	memset(&key_attr, 0, sizeof(key_attr));
	memset(&obj, 0, sizeof(obj));

	key_class = CKO_PUBLIC_KEY;
	key_attr[0].type = CKA_CLASS;
	key_attr[0].pValue = &key_class;
	key_attr[0].ulValueLen = sizeof(key_class);

	session = p->slotinfo[slotidx].session;
	f = p->function_list;

	rv = f->C_FindObjectsInit(session, key_attr, 1);
	if (rv != CKR_OK) {
		error("C_FindObjectsInit failed: %lu", rv);
		goto fail;
	}

	while (1) {
		CK_KEY_TYPE	ck_key_type;

		rv = f->C_FindObjects(session, &obj, 1, &n);
		if (rv != CKR_OK) {
			error("C_FindObjects failed: %lu", rv);
			goto fail;
		}
		if (n == 0)
			break;

		memset(&ck_key_type, 0, sizeof(ck_key_type));
		memset(&key_attr, 0, sizeof(key_attr));
		key_attr[0].type = CKA_KEY_TYPE;
		key_attr[0].pValue = &ck_key_type;
		key_attr[0].ulValueLen = sizeof(ck_key_type);

		rv = f->C_GetAttributeValue(session, obj, key_attr, 1);
		if (rv != CKR_OK) {
			error("C_GetAttributeValue failed: %lu", rv);
			goto fail;
		}

		switch (ck_key_type) {
		case CKK_RSA:
			key = pkcs11_fetch_rsa_pubkey(p, slotidx, &obj);
			break;
		case CKK_ECDSA:
			key = pkcs11_fetch_ecdsa_pubkey(p, slotidx, &obj);
			break;
		default:
			/* XXX print key type? */
			error("skipping unsupported key type");
		}

		if (key == NULL) {
			error("failed to fetch key");
			continue;
		}

		if (pkcs11_key_included(keysp, nkeys, key)) {
			key_free(key);
		} else {
			/* expand key array and add key */
			*keysp = xrealloc(*keysp, *nkeys + 1, sizeof(Key *));
			(*keysp)[*nkeys] = key;
			*nkeys = *nkeys + 1;
			debug("have %d keys", *nkeys);
		}
	}

	ret = 0;
fail:
	rv = f->C_FindObjectsFinal(session);
	if (rv != CKR_OK) {
		error("C_FindObjectsFinal failed: %lu", rv);
		ret = -1;
	}

	return (ret);
}

#ifdef HAVE_DLOPEN
/* register a new provider, fails if provider already exists */
int
pkcs11_add_provider(char *provider_id, char *pin, Key ***keyp)
{
	int nkeys, need_finalize = 0;
	struct pkcs11_provider *p = NULL;
	void *handle = NULL;
	CK_RV (*getfunctionlist)(CK_FUNCTION_LIST **);
	CK_RV rv;
	CK_FUNCTION_LIST *f = NULL;
	CK_TOKEN_INFO *token;
	CK_ULONG i;

	*keyp = NULL;
	if (pkcs11_provider_lookup(provider_id) != NULL) {
		error("provider already registered: %s", provider_id);
		goto fail;
	}
	/* open shared pkcs11-libarary */
	if ((handle = dlopen(provider_id, RTLD_NOW)) == NULL) {
		error("dlopen %s failed: %s", provider_id, dlerror());
		goto fail;
	}
	if ((getfunctionlist = dlsym(handle, "C_GetFunctionList")) == NULL) {
		error("dlsym(C_GetFunctionList) failed: %s", dlerror());
		goto fail;
	}
	p = xcalloc(1, sizeof(*p));
	p->name = xstrdup(provider_id);
	p->handle = handle;
	/* setup the pkcs11 callbacks */
	if ((rv = (*getfunctionlist)(&f)) != CKR_OK) {
		error("C_GetFunctionList failed: %lu", rv);
		goto fail;
	}
	p->function_list = f;
	if ((rv = f->C_Initialize(NULL)) != CKR_OK) {
		error("C_Initialize failed: %lu", rv);
		goto fail;
	}
	need_finalize = 1;
	if ((rv = f->C_GetInfo(&p->info)) != CKR_OK) {
		error("C_GetInfo failed: %lu", rv);
		goto fail;
	}
	rmspace(p->info.manufacturerID, sizeof(p->info.manufacturerID));
	rmspace(p->info.libraryDescription, sizeof(p->info.libraryDescription));
	debug("manufacturerID <%s> cryptokiVersion %d.%d"
	    " libraryDescription <%s> libraryVersion %d.%d",
	    p->info.manufacturerID,
	    p->info.cryptokiVersion.major,
	    p->info.cryptokiVersion.minor,
	    p->info.libraryDescription,
	    p->info.libraryVersion.major,
	    p->info.libraryVersion.minor);
	if ((rv = f->C_GetSlotList(CK_TRUE, NULL, &p->nslots)) != CKR_OK) {
		error("C_GetSlotList failed: %lu", rv);
		goto fail;
	}
	if (p->nslots == 0) {
		error("no slots");
		goto fail;
	}
	p->slotlist = xcalloc(p->nslots, sizeof(CK_SLOT_ID));
	if ((rv = f->C_GetSlotList(CK_TRUE, p->slotlist, &p->nslots))
	    != CKR_OK) {
		error("C_GetSlotList failed: %lu", rv);
		goto fail;
	}
	p->slotinfo = xcalloc(p->nslots, sizeof(struct pkcs11_slotinfo));
	p->valid = 1;
	nkeys = 0;
	for (i = 0; i < p->nslots; i++) {
		token = &p->slotinfo[i].token;
		if ((rv = f->C_GetTokenInfo(p->slotlist[i], token))
		    != CKR_OK) {
			error("C_GetTokenInfo failed: %lu", rv);
			continue;
		}
		rmspace(token->label, sizeof(token->label));
		rmspace(token->manufacturerID, sizeof(token->manufacturerID));
		rmspace(token->model, sizeof(token->model));
		rmspace(token->serialNumber, sizeof(token->serialNumber));
		debug("label <%s> manufacturerID <%s> model <%s> serial <%s>"
		    " flags 0x%lx",
		    token->label, token->manufacturerID, token->model,
		    token->serialNumber, token->flags);
		/* open session, login with pin and retrieve public keys */
		if (pkcs11_open_session(p, i, pin) == 0) {
			pkcs11_fetch_keys(p, i, keyp, &nkeys);
			pkcs11_fetch_certs(p, i, keyp, &nkeys);
		}
	}
	if (nkeys > 0) {
		TAILQ_INSERT_TAIL(&pkcs11_providers, p, next);
		p->refcount++;	/* add to provider list */
		return (nkeys);
	}
	error("no keys");
	/* don't add the provider, since it does not have any keys */
fail:
	if (need_finalize && (rv = f->C_Finalize(NULL)) != CKR_OK)
		error("C_Finalize failed: %lu", rv);
	if (p) {
		free(p->slotlist);
		free(p->slotinfo);
		free(p);
	}
	if (handle)
		dlclose(handle);
	return (-1);
}
#else
int
pkcs11_add_provider(char *provider_id, char *pin, Key ***keyp)
{
	error("dlopen() not supported");
	return (-1);
}
#endif
