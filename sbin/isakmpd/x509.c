/*	$OpenBSD: x509.c,v 1.48 2001/04/09 22:09:53 ho Exp $	*/
/*	$EOM: x509.c,v 1.54 2001/01/16 18:42:16 ho Exp $	*/

/*
 * Copyright (c) 1998, 1999 Niels Provos.  All rights reserved.
 * Copyright (c) 1999, 2000, 2001 Niklas Hallqvist.  All rights reserved.
 * Copyright (c) 1999, 2000, 2001 Angelos D. Keromytis.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Ericsson Radio Systems.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * This code was written under funding by Ericsson Radio Systems.
 */

#ifdef USE_X509

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef USE_POLICY
#include <regex.h>
#include <keynote.h>
#endif /* USE_POLICY */

#include "sysdep.h"

#include "cert.h"
#include "conf.h"
#include "dyn.h"
#include "exchange.h"
#include "hash.h"
#include "ike_auth.h"
#include "ipsec.h"
#include "log.h"
#include "math_mp.h"
#include "policy.h"
#include "sa.h"
#include "x509.h"

/*
 * X509_STOREs do not support subjectAltNames, so we have to build
 * our own hash table.
 */

/*
 * XXX Actually this store is not really useful, we never use it as we have
 * our own hash table.  It also gets collisons if we have several certificates
 * only differing in subjectAltName.
 */
static X509_STORE *x509_certs = 0;
static X509_STORE *x509_cas = 0;

/* Initial number of bits used as hash.  */
#define INITIAL_BUCKET_BITS 6

struct x509_hash {
  LIST_ENTRY (x509_hash) link;

  X509 *cert;
};

static LIST_HEAD (x509_list, x509_hash) *x509_tab = 0;

/* Works both as a maximum index and a mask.  */
static int bucket_mask;

#ifdef USE_POLICY
/*
 * Given an X509 certificate, create a KeyNote assertion where
 * Issuer/Subject -> Authorizer/Licensees.
 * XXX RSA-specific.
 */
int
x509_generate_kn (X509 *cert)
{
  char *fmt = "Authorizer: \"rsa-hex:%s\"\nLicensees: \"rsa-hex:%s\"\n"
    "Conditions: %s >= \"%s\" && %s <= \"%s\";\n";
  char *ikey, *skey, *buf, isname[256], subname[256];
  char *fmt2 = "Authorizer: \"DN:%s\"\nLicensees: \"DN:%s\"\n"
    "Conditions: %s >= \"%s\" && %s <= \"%s\";\n";
  X509_NAME *issuer, *subject;
  struct keynote_deckey dc;
  X509_STORE_CTX csc;
  X509_OBJECT obj;
  X509 *icert;
  RSA *key;
  char **new_asserts;
  time_t tt;
  char before[15], after[15];
  ASN1_TIME *tm;
  char *timecomp, *timecomp2;
  int i;

  LOG_DBG ((LOG_POLICY, 90,
	    "x509_generate_kn: generating KeyNote policy for certificate %p",
	    cert));

  issuer = LC (X509_get_issuer_name, (cert));
  subject = LC (X509_get_subject_name, (cert));

  /* Missing or self-signed, ignore cert but don't report failure.  */
  if (!issuer || !subject || !LC (X509_name_cmp, (issuer, subject)))
    return 1;

  if (!x509_cert_get_key (cert, &key))
    {
      LOG_DBG ((LOG_POLICY, 30,
		"x509_generate_kn: failed to get public key from cert"));
      return 0;
    }

  dc.dec_algorithm = KEYNOTE_ALGORITHM_RSA;
  dc.dec_key = key;
  ikey = LK (kn_encode_key, (&dc, INTERNAL_ENC_PKCS1, ENCODING_HEX,
			     KEYNOTE_PUBLIC_KEY));
  if (LKV (keynote_errno) == ERROR_MEMORY)
    {
      log_print ("x509_generate_kn: failed to get memory for public key");
      LC (RSA_free, (key));
      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: cannot get subject key"));
      return 0;
    }
  if (!ikey)
    {
      LC (RSA_free, (key));
      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: cannot get subject key"));
      return 0;
    }
  LC (RSA_free, (key));

  /* Now find issuer's certificate so we can get the public key.  */
  LC (X509_STORE_CTX_init, (&csc, x509_cas, cert, NULL));
  if (LC (X509_STORE_get_by_subject, (&csc, X509_LU_X509, issuer, &obj)) !=
      X509_LU_X509)
    {
      LC (X509_STORE_CTX_cleanup, (&csc));
      LC (X509_STORE_CTX_init, (&csc, x509_certs, cert, NULL));
      if (LC (X509_STORE_get_by_subject, (&csc, X509_LU_X509, issuer, &obj)) !=
          X509_LU_X509)
	{
  	  LC (X509_STORE_CTX_cleanup, (&csc));
	  LOG_DBG ((LOG_POLICY, 30,
		    "x509_generate_kn: no certificate found for issuer"));
	  return 0;
	}
    }

  LC (X509_STORE_CTX_cleanup, (&csc));
  icert = obj.data.x509;

  if (icert == NULL)
    {
      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: "
		"missing certificates, cannot construct X509 chain"));
      free (ikey);
      return 0;
    }

  if (!x509_cert_get_key (icert, &key))
    {
      LOG_DBG ((LOG_POLICY, 30,
		"x509_generate_kn: failed to get public key from cert"));
      free (ikey);
      return 0;
    }

  LC (X509_OBJECT_free_contents, (&obj));

  dc.dec_algorithm = KEYNOTE_ALGORITHM_RSA;
  dc.dec_key = key;
  skey = LK (kn_encode_key, (&dc, INTERNAL_ENC_PKCS1, ENCODING_HEX,
			     KEYNOTE_PUBLIC_KEY));
  if (LKV (keynote_errno) == ERROR_MEMORY)
    {
      log_error ("x509_generate_kn: failed to get memory for public key");
      free (ikey);
      LC (RSA_free, (key));
      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: cannot get issuer key"));
      return 0;
    }

  if (!skey)
    {
      free (ikey);
      LC (RSA_free, (key));
      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: cannot get issuer key"));
      return 0;
    }
  LC (RSA_free, (key));

  buf = calloc (strlen (fmt) + strlen (ikey) + strlen (skey) + 56,
		sizeof (char));
  if (!buf)
    {
      log_error ("x509_generate_kn: "
		 "failed to allocate memory for KeyNote credential");
      free (ikey);
      free (skey);
      return 0;
    }

  if (((tm = X509_get_notBefore (cert)) == NULL) ||
      (tm->type != V_ASN1_UTCTIME && tm->type != V_ASN1_GENERALIZEDTIME))
    {
      tt = time ((time_t) NULL);
      strftime (before, 14, "%G%m%d%H%M%S", localtime (&tt));
      timecomp = "LocalTimeOfDay";
    }
  else
    {
      if (tm->data[tm->length - 1] == 'Z')
	{
	  timecomp = "GMTTimeOfDay";
	  i = tm->length - 2;
	}
      else
        {
	  timecomp = "LocalTimeOfDay";
	  i = tm->length - 1;
	}

      for (; i >= 0; i--)
        {
	  if (tm->data[i] < '0' || tm->data[i] > '9')
	    {
	      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: invalid data in "
			"NotValidBefore time field"));
	      free (ikey);
	      free (skey);
	      free (buf);
	      return 0;
	    }
	}

      if (tm->type == V_ASN1_UTCTIME)
	{
	  if ((tm->length < 10) || (tm->length > 13))
	    {
	      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: invalid length "
			"of NotValidBefore time field (%d)", tm->length));
	      free (ikey);
	      free (skey);
	      free (buf);
	      return 0;
	    }

	  /* Validity checks.  */
	  if ((tm->data[2] != '0' && tm->data[2] != '1')
	      || (tm->data[2] == '0' && tm->data[3] == '0')
	      || (tm->data[2] == '1' && tm->data[3] > '2')
	      || (tm->data[4] > '3')
	      || (tm->data[4] == '0' && tm->data[5] == '0')
	      || (tm->data[4] == '3' && tm->data[5] > '1')
	      || (tm->data[6] > '2')
	      || (tm->data[6] == '2' && tm->data[7] > '3')
	      || (tm->data[8] > '5'))
	    {
	      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: invalid value in "
			"NotValidBefore time field"));
	      free (ikey);
	      free (skey);
	      free (buf);
	      return 0;
	    }

	  /* Stupid UTC tricks.  */
	  if (tm->data[0] < '5')
	    sprintf (before, "20%s", tm->data);
	  else
	    sprintf (before, "19%s", tm->data);
	}
      else
        { /* V_ASN1_GENERICTIME */
	  if ((tm->length < 12) || (tm->length > 15))
	    {
	      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: invalid length of "
			"NotValidBefore time field (%d)", tm->length));
	      free (ikey);
	      free (skey);
	      free (buf);
	      return 0;
	    }

	  /* Validity checks.  */
	  if ((tm->data[4] != '0' && tm->data[4] != '1')
	      || (tm->data[4] == '0' && tm->data[5] == '0')
	      || (tm->data[4] == '1' && tm->data[5] > '2')
	      || (tm->data[6] > '3')
	      || (tm->data[6] == '0' && tm->data[7] == '0')
	      || (tm->data[6] == '3' && tm->data[7] > '1')
	      || (tm->data[8] > '2')
	      || (tm->data[8] == '2' && tm->data[9] > '3')
	      || (tm->data[10] > '5'))
	    {
	      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: invalid value in "
			"NotValidBefore time field"));
	      free (ikey);
	      free (skey);
	      free (buf);
	      return 0;
	    }

	  sprintf (before, "%s", tm->data);
	}

      /* Fix missing seconds.  */
      if (tm->length < 12)
        {
	  before[12] = '0';
	  before[13] = '0';
	}

      /* This will overwrite trailing 'Z'.  */
      before[14] = '\0';
    }

  tm = X509_get_notAfter (cert);
  if (tm == NULL
      && (tm->type != V_ASN1_UTCTIME && tm->type != V_ASN1_GENERALIZEDTIME))
    {
      tt = time (0);
      strftime (after, 14, "%G%m%d%H%M%S", localtime (&tt));
      timecomp2 = "LocalTimeOfDay";
    }
  else
    {
      if (tm->data[tm->length - 1] == 'Z')
	{
	  timecomp2 = "GMTTimeOfDay";
	  i = tm->length - 2;
	}
      else
        {
	  timecomp2 = "LocalTimeOfDay";
	  i = tm->length - 1;
	}

      for (; i >= 0; i--)
        {
	  if (tm->data[i] < '0' || tm->data[i] > '9')
	    {
	      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: invalid data in "
			"NotValidAfter time field"));
	      free (ikey);
	      free (skey);
	      free (buf);
	      return 0;
	    }
	}

      if (tm->type == V_ASN1_UTCTIME)
	{
	  if ((tm->length < 10) || (tm->length > 13))
	    {
	      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: invalid length of "
			"NotValidAfter time field (%d)", tm->length));
	      free (ikey);
	      free (skey);
	      free (buf);
	      return 0;
	    }

	  /* Validity checks. */
	  if ((tm->data[2] != '0' && tm->data[2] != '1')
	      || (tm->data[2] == '0' && tm->data[3] == '0')
	      || (tm->data[2] == '1' && tm->data[3] > '2')
	      || (tm->data[4] > '3')
	      || (tm->data[4] == '0' && tm->data[5] == '0')
	      || (tm->data[4] == '3' && tm->data[5] > '1')
	      || (tm->data[6] > '2')
	      || (tm->data[6] == '2' && tm->data[7] > '3')
	      || (tm->data[8] > '5'))
	    {
	      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: invalid value in "
			"NotValidAfter time field"));
	      free (ikey);
	      free (skey);
	      free (buf);
	      return 0;
	    }

	  /* Stupid UTC tricks.  */
	  if (tm->data[0] < '5')
	    sprintf (after, "20%s", tm->data);
	  else
	    sprintf (after, "19%s", tm->data);
	}
      else
        { /* V_ASN1_GENERICTIME */
	  if ((tm->length < 12) || (tm->length > 15))
	    {
	      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: invalid length of "
			"NotValidAfter time field (%d)", tm->length));
	      free (ikey);
	      free (skey);
	      free (buf);
	      return 0;
	    }

	  /* Validity checks.  */
	  if ((tm->data[4] != '0' && tm->data[4] != '1')
	      || (tm->data[4] == '0' && tm->data[5] == '0')
	      || (tm->data[4] == '1' && tm->data[5] > '2')
	      || (tm->data[6] > '3')
	      || (tm->data[6] == '0' && tm->data[7] == '0')
	      || (tm->data[6] == '3' && tm->data[7] > '1')
	      || (tm->data[8] > '2')
	      || (tm->data[8] == '2' && tm->data[9] > '3')
	      || (tm->data[10] > '5'))
	    {
	      LOG_DBG ((LOG_POLICY, 30, "x509_generate_kn: invalid value in "
			"NotValidAfter time field"));
	      free (ikey);
	      free (skey);
	      free (buf);
	      return 0;
	    }

	  sprintf (after, "%s", tm->data);
        }

      /* Fix missing seconds.  */
      if (tm->length < 12)
        {
	  after[12] = '0';
	  after[13] = '0';
	}

      after[14] = '\0'; /* This will overwrite trailing 'Z' */
    }

  sprintf (buf, fmt, skey, ikey, timecomp, before, timecomp2, after);
  free (ikey);
  free (skey);

  if (LK (kn_add_assertion, (keynote_sessid, buf, strlen (buf),
			     ASSERT_FLAG_LOCAL)) == -1)
    {
      LOG_DBG ((LOG_POLICY, 30,
		"x509_generate_kn: failed to add new KeyNote credential"));
      free (buf);
      return 0;
    }

  /* We could print the assertion here, but log_print() truncates...  */

  free (buf);

  if (!LC (X509_NAME_oneline, (issuer, isname, 256)))
    {
      LOG_DBG ((LOG_POLICY, 50,
		"x509_generate_kn: X509_NAME_oneline (issuer, ...) failed"));
      return 0;
    }

  if (!LC (X509_NAME_oneline, (subject, subname, 256)))
    {
      LOG_DBG ((LOG_POLICY, 50,
		"x509_generate_kn: X509_NAME_oneline (subject, ...) failed"));
      return 0;
    }

  buf = malloc (strlen (fmt2) + strlen (isname) + strlen (subname) + 56);
  if (!buf)
    {
      log_error ("x509_generate_kn: malloc (%d) failed", strlen (fmt2) +
		 strlen (isname) + strlen (subname) + 56);
      return 0;
    }

  sprintf (buf, fmt2, isname, subname, timecomp, before, timecomp2, after);

  if (LK (kn_add_assertion, (keynote_sessid, buf, strlen (buf),
			     ASSERT_FLAG_LOCAL)) == -1)
    {
      LOG_DBG ((LOG_POLICY, 30,
		"x509_generate_kn: failed to add new KeyNote credential"));
      free (buf);
      return 0;
    }
  else
    LOG_DBG ((LOG_POLICY, 80, "x509_generate_kn: added policy:\n%s", buf));

  /* Store the X509-derived assertion so we can use it as a policy.  */
  if (x509_policy_asserts_num == 0)
    {
      x509_policy_asserts = calloc (4, sizeof (char *));
      if (!x509_policy_asserts)
	{
	  log_error ("x509_generate_kn: failed to allocate %d bytes",
		     4 * sizeof (char *));
	  free (buf);
	  return 0;
	}

      x509_policy_asserts_num_alloc = 4;
      x509_policy_asserts_num = 1;
      x509_policy_asserts[0] = buf;
    }
  else
    {
      if (x509_policy_asserts_num + 1 > x509_policy_asserts_num_alloc)
	{
	  x509_policy_asserts_num_alloc *= 2;
	  new_asserts = realloc (x509_policy_asserts,
				 x509_policy_asserts_num_alloc
				 * sizeof (char *));
	  if (!new_asserts)
	    {
	      x509_policy_asserts_num_alloc /= 2;
	      log_error ("x509_generate_kn: failed to allocate %d bytes",
			 x509_policy_asserts_num_alloc * sizeof (char *));
	      free (buf);
	      return 0;
	    }

	  x509_policy_asserts = new_asserts;
	}

      /* Assign to the next available.  */
      x509_policy_asserts[x509_policy_asserts_num++] = buf;
    }

  /*
   * XXX Should add a remove-assertion event set to the expiration of the
   * X509 cert (and remove such events when we reinit and close the keynote
   * session)  -- that's relevant only for really long-lived daemons.
   * Alternatively (and preferably), we can encode the X509 expiration
   * in the KeyNote Conditions.
   */

  return 1;
}
#endif /* USE_POLICY */

u_int16_t
x509_hash (u_int8_t *id, size_t len)
{
  int i;
  u_int16_t bucket = 0;

  /* XXX We might resize if we are crossing a certain threshold.  */
  for (i = 4; i < (len & ~1); i += 2)
    {
      /* Doing it this way avoids alignment problems.  */
      bucket ^= (id[i] + 1) * (id[i + 1] + 257);
    }
  /* Hash in the last character of odd length IDs too.  */
  if (i < len)
    bucket ^= (id[i] + 1) * (id[i] + 257);

  bucket &= bucket_mask;

  return bucket;
}

void
x509_hash_init ()
{
  struct x509_hash *certh;
  int i;

  bucket_mask = (1 << INITIAL_BUCKET_BITS) - 1;

  /* If reinitializing, free existing entries.  */
  if (x509_tab)
    {
      for (i = 0; i <= bucket_mask; i++)
        for (certh = LIST_FIRST (&x509_tab[i]); certh;
             certh = LIST_NEXT (certh, link))
	  {
	    LIST_REMOVE (certh, link);
	    LC (X509_free, (certh->cert));
	    free (certh);
	  }

      free (x509_tab);
    }

  x509_tab = malloc ((bucket_mask + 1) * sizeof (struct x509_list));
  if (!x509_tab)
    log_fatal ("x509_hash_init: malloc (%d) failed",
	       (bucket_mask + 1) * sizeof (struct x509_list));
  for (i = 0; i <= bucket_mask; i++)
    {
      LIST_INIT (&x509_tab[i]);
    }
}

/* Lookup a certificate by an ID blob.  */
X509 *
x509_hash_find (u_int8_t *id, size_t len)
{
  struct x509_hash *cert;
  u_int8_t **cid;
  size_t *clen;
  int n, i, id_found;

  for (cert = LIST_FIRST (&x509_tab[x509_hash (id, len)]); cert;
       cert = LIST_NEXT (cert, link))
    {
      if (!x509_cert_get_subjects (cert->cert, &n, &cid, &clen))
	continue;

      id_found = 0;
      for (i = 0; i < n; i++)
	{
	  LOG_DBG_BUF ((LOG_CRYPTO, 70, "cert_cmp", id, len));
	  LOG_DBG_BUF ((LOG_CRYPTO, 70, "cert_cmp", cid[i], clen[i]));
	  /* XXX This identity predicate needs to be understood.  */
	  if (clen[i] == len && id[0] == cid[i][0]
	      && memcmp (id + 4, cid[i] + 4, len - 4) == 0)
	    {
	      id_found++;
	      break;
	    }
	}
      cert_free_subjects (n, cid, clen);
      if (!id_found)
	continue;

      LOG_DBG ((LOG_CRYPTO, 70, "x509_hash_find: return X509 %p",
		cert->cert));
      return cert->cert;
    }

  LOG_DBG ((LOG_CRYPTO, 70, "x509_hash_find: no certificate matched query"));
  return 0;
}

int
x509_hash_enter (X509 *cert)
{
  u_int16_t bucket = 0;
  u_int8_t **id;
  u_int32_t *len;
  struct x509_hash *certh;
  int n, i;

  if (!x509_cert_get_subjects (cert, &n, &id, &len))
    {
      log_print ("x509_hash_enter: cannot retrieve subjects");
      return 0;
    }

  for (i = 0; i < n; i++)
    {
      certh = calloc (1, sizeof *certh);
      if (!certh)
        {
          cert_free_subjects (n, id, len);
          log_error ("x509_hash_enter: calloc (1, %d) failed", sizeof *certh);
          return 0;
        }

      certh->cert = cert;

      bucket = x509_hash (id[i], len[i]);

      LIST_INSERT_HEAD (&x509_tab[bucket], certh, link);
      LOG_DBG ((LOG_CRYPTO, 70, "x509_hash_enter: cert %p added to bucket %d",
		cert, bucket));
    }
  cert_free_subjects (n, id, len);

  return 1;
}

/* X509 Certificate Handling functions.  */

int
x509_read_from_dir (X509_STORE *ctx, char *name, int hash)
{
  DIR *dir;
  struct dirent *file;
  BIO *certh;
  X509 *cert;
  char fullname[PATH_MAX];
  int off, size;

  if (strlen (name) >= sizeof fullname - 1)
    {
      log_print ("x509_read_from_dir: directory name too long");
      return 0;
    }

  LOG_DBG ((LOG_CRYPTO, 40, "x509_read_from_dir: reading certs from %s",
	    name));

  dir = opendir (name);
  if (!dir)
    {
      log_error ("x509_read_from_dir: opendir (\"%s\") failed", name);
      return 0;
    }

  strncpy (fullname, name, sizeof fullname - 1);
  fullname[sizeof fullname - 1] = 0;
  off = strlen (fullname);
  size = sizeof fullname - off - 1;

  while ((file = readdir (dir)) != NULL)
    {
      if (file->d_type != DT_REG && file->d_type != DT_LNK)
	continue;

      LOG_DBG ((LOG_CRYPTO, 60, "x509_read_from_dir: reading certificate %s",
		file->d_name));

      certh = LC (BIO_new, (LC (BIO_s_file, ())));
      if (!certh)
	{
	  log_error ("x509_read_from_dir: BIO_new (BIO_s_file ()) failed");
	  continue;
	}

      strncpy (fullname + off, file->d_name, size);
      fullname[off + size] = 0;

      if (LC (BIO_read_filename, (certh, fullname)) == -1)
	{
	  LC (BIO_free, (certh));
	  log_error ("x509_read_from_dir: "
		     "BIO_read_filename (certh, \"%s\") failed",
		     fullname);
	  continue;
	}

#if SSLEAY_VERSION_NUMBER >= 0x00904100L
      cert = LC (PEM_read_bio_X509, (certh, NULL, NULL, NULL));
#else
      cert = LC (PEM_read_bio_X509, (certh, NULL, NULL));
#endif
      LC (BIO_free, (certh));
      if (cert == NULL)
	{
	  log_error ("x509_read_from_dir: PEM_read_bio_X509 failed for %s",
		     file->d_name);
	  continue;
	}

      if (!LC (X509_STORE_add_cert, (ctx, cert)))
	{
	  /*
	   * This is actually expected if we have several certificates only
	   * differing in subjectAltName, which is not an something that is
	   * strange.  Consider multi-homed machines.
	   */
	  LOG_DBG ((LOG_CRYPTO, 50,
		    "x509_read_from_dir: X509_STORE_add_cert failed for %s",
		    file->d_name));
	}

      if (hash)
	{

	  if (!x509_hash_enter (cert))
	    log_print ("x509_read_from_dir: x509_hash_enter (%s) failed",
		       file->d_name);
#ifdef USE_POLICY
#ifdef USE_KEYNOTE
	  if (x509_generate_kn (cert) == 0)
#else
	    if (libkeynote && x509_generate_kn (cert) == 0)
#endif
	      LOG_DBG ((LOG_POLICY, 50,
			"x509_read_from_dir: x509_generate_kn failed"));
#endif /* USE_POLICY */
	}
    }

  closedir (dir);

  return 1;
}

/* Initialize our databases and load our own certificates.  */
int
x509_cert_init (void)
{
  char *dirname;
#if defined(USE_KEYNOTE) || defined(USE_POLICY)
  int i;
#endif

  x509_hash_init ();

  /* Process CA certificates we will trust.  */
  dirname = conf_get_str ("X509-certificates", "CA-directory");
  if (!dirname)
    {
      log_print ("x509_cert_init: no CA-directory");
      return 0;
    }

  /* Free if already initialized.  */
  if (x509_cas)
    LC (X509_STORE_free, (x509_cas));

  x509_cas = LC (X509_STORE_new, ());
  if (!x509_cas)
    {
      log_print ("x509_cert_init: creating new X509_STORE failed");
      return 0;
    }

  if (!x509_read_from_dir (x509_cas, dirname, 0))
    {
      log_print ("x509_cert_init: x509_read_from_dir failed");
      return 0;
    }

  /* Process client certificates we will accept.  */
  dirname = conf_get_str ("X509-certificates", "Cert-directory");
  if (!dirname)
    {
      log_print ("x509_cert_init: no Cert-directory");
      return 0;
    }

  /* Free if already initialized.  */
  if (x509_certs)
    LC (X509_STORE_free, (x509_certs));

  x509_certs = LC (X509_STORE_new, ());
  if (!x509_certs)
    {
      log_print ("x509_cert_init: creating new X509_STORE failed");
      return 0;
    }

#if defined(USE_KEYNOTE) || defined(USE_POLICY)
  /* Cleanup */
  if (x509_policy_asserts)
    {
      for (i = 0; i < x509_policy_asserts_num; i++)
        if (x509_policy_asserts[i])
          free (x509_policy_asserts[i]);

      free (x509_policy_asserts);
    }

  x509_policy_asserts = 0;
  x509_policy_asserts_num = x509_policy_asserts_num_alloc = 0;
#endif

  if (!x509_read_from_dir (x509_certs, dirname, 1))
    {
      log_print ("x509_cert_init: x509_read_from_dir failed");
      return 0;
    }

  return 1;
}

void *
x509_cert_get (u_int8_t *asn, u_int32_t len)
{
#ifndef USE_LIBCRYPTO
  /*
   * If we don't have a statically linked libcrypto, the dlopen must have
   * succeeded for X.509 to be usable.
   */
  if (!libcrypto)
    return 0;
#endif

  return x509_from_asn (asn, len);
}

int
x509_cert_validate (void *scert)
{
  X509_STORE_CTX csc;
  X509_NAME *issuer, *subject;
  X509 *cert = (X509 *)scert;
  EVP_PKEY *key;
  int res;

  /*
   * Validate the peer certificate by checking with the CA certificates we
   * trust.
   */
  LC (X509_STORE_CTX_init, (&csc, x509_cas, cert, NULL));
  res = LC (X509_verify_cert, (&csc));
  LC (X509_STORE_CTX_cleanup, (&csc));

  /* Return if validation succeeded or self-signed certs are not accepted.  */
  if (res || !conf_get_str ("X509-certificates", "Accept-self-signed"))
    return res;

  issuer = LC (X509_get_issuer_name, (cert));
  subject = LC (X509_get_subject_name, (cert));

  if (!issuer || !subject || LC (X509_name_cmp, (issuer, subject)))
    return 0;

  key = LC (X509_get_pubkey, (cert));
  if (!key)
    return 0;

  if (LC (X509_verify, (cert, key)) == -1)
    return 0;

  return 1;
}

int
x509_cert_insert (int id, void *scert)
{
  X509 *cert;
  int res;

  cert = LC (X509_dup, ((X509 *)scert));
  if (!cert)
    {
      log_print ("x509_cert_insert: X509_dup failed");
      return 0;
    }

#ifdef USE_POLICY
#ifdef USE_KEYNOTE
  if (x509_generate_kn (cert) == 0)
#else
    if (libkeynote && x509_generate_kn (cert) == 0)
#endif
      {
	LOG_DBG ((LOG_POLICY, 50,
		  "x509_cert_insert: x509_generate_kn failed"));
	LC (X509_free, (cert));
	return 0;
      }
#endif /* USE_POLICY */

  res = x509_hash_enter (cert);
  if (!res)
    LC (X509_free, (cert));

  return res;
}

void
x509_cert_free (void *cert)
{
  LC (X509_free, ((X509 *)cert));
}

/* Validate the BER Encoding of a RDNSequence in the CERT_REQ payload.  */
int
x509_certreq_validate (u_int8_t *asn, u_int32_t len)
{
  int res = 1;
#if 0
  struct norm_type name = SEQOF ("issuer", RDNSequence);

  if (!asn_template_clone (&name, 1)
      || (asn = asn_decode_sequence (asn, len, &name)) == 0)
    {
      log_print ("x509_certreq_validate: can not decode 'acceptable CA' info");
      res = 0;
    }
  asn_free (&name);
#endif

  /* XXX - not supported directly in SSL - later.  */

  return res;
}

/* Decode the BER Encoding of a RDNSequence in the CERT_REQ payload.  */
void *
x509_certreq_decode (u_int8_t *asn, u_int32_t len)
{
#if 0
  /* XXX This needs to be done later.  */
  struct norm_type aca = SEQOF ("aca", RDNSequence);
  struct norm_type *tmp;
  struct x509_aca naca, *ret;

  if (!asn_template_clone (&aca, 1)
      || (asn = asn_decode_sequence (asn, len, &aca)) == 0)
    {
      log_print ("x509_certreq_validate: can not decode 'acceptable CA' info");
      goto fail;
    }
  memset (&naca, 0, sizeof (naca));

  tmp = asn_decompose ("aca.RelativeDistinguishedName.AttributeValueAssertion",
		       &aca);
  if (!tmp)
    goto fail;
  x509_get_attribval (tmp, &naca.name1);

  tmp = asn_decompose ("aca.RelativeDistinguishedName[1]"
		       ".AttributeValueAssertion", &aca);
  if (tmp)
    x509_get_attribval (tmp, &naca.name2);

  asn_free (&aca);

  ret = malloc (sizeof (struct x509_aca));
  if (ret)
    memcpy (ret, &naca, sizeof (struct x509_aca));
  else
    {
      log_error ("x509_certreq_decode: malloc (%d) failed",
		 sizeof (struct x509_aca));
      x509_free_aca (&aca);
    }

  return ret;

 fail:
  asn_free (&aca);
#endif
  return 0;
}

void
x509_free_aca (void *blob)
{
  struct x509_aca *aca = blob;

  if (aca->name1.type)
    free (aca->name1.type);
  if (aca->name1.val)
    free (aca->name1.val);

  if (aca->name2.type)
    free (aca->name2.type);
  if (aca->name2.val)
    free (aca->name2.val);
}

X509 *
x509_from_asn (u_char *asn, u_int len)
{
  BIO *certh;
  X509 *scert = 0;

  certh = LC (BIO_new, (LC (BIO_s_mem, ())));
  if (!certh)
    {
      log_error ("x509_from_asn: BIO_new (BIO_s_mem ()) failed");
      return 0;
    }

  if (LC (BIO_write, (certh, asn, len)) == -1)
    {
      log_error ("x509_from_asn: BIO_write failed\n");
      goto end;
    }

  scert = LC (d2i_X509_bio, (certh, NULL));
  if (!scert)
    {
      log_print ("x509_from_asn: d2i_X509_bio failed\n");
      goto end;
    }

 end:
  LC (BIO_free, (certh));
  return scert;
}

/*
 * Obtain a certificate from an acceptable CA.
 * XXX We don't check if the certificate we find is from an accepted CA.
 */
int
x509_cert_obtain (u_int8_t *id, size_t id_len, void *data, u_int8_t **cert,
		  u_int32_t *certlen)
{
  struct x509_aca *aca = data;
  X509 *scert;
  u_char *p;

  if (aca)
    LOG_DBG ((LOG_CRYPTO, 60,
	      "x509_cert_obtain: acceptable certificate authorities here"));

  /* We need our ID to find a certificate.  */
  if (!id)
    {
      log_print ("x509_cert_obtain: ID is missing");
      return 0;
    }

  scert = x509_hash_find (id, id_len);
  if (!scert)
    return 0;

  *certlen = LC (i2d_X509, (scert, NULL));
  p = *cert = malloc (*certlen);
  if (!p)
    {
      log_error ("x509_cert_obtain: malloc (%d) failed", *certlen);
      return 0;
    }
  *certlen = LC (i2d_X509, (scert, &p));

  return 1;
}

/* Returns a pointer to the subjectAltName information of X509 certificate.  */
int
x509_cert_subjectaltname (X509 *scert, u_int8_t **altname, u_int32_t *len)
{
  X509_EXTENSION *subjectaltname;
  u_int8_t *sandata;
  int extpos;
  int santype, sanlen;

  extpos = LC (X509_get_ext_by_NID, (scert, NID_subject_alt_name, -1));
  if (extpos == -1)
    {
      log_print ("x509_cert_subjectaltname: "
		 "certificate does not contain subjectAltName");
      return 0;
    }

  subjectaltname = LC (X509_get_ext, (scert, extpos));

  if (!subjectaltname || !subjectaltname->value
      || !subjectaltname->value->data || subjectaltname->value->length < 4)
    {
      log_print ("x509_cert_subjectaltname: invalid subjectaltname extension");
      return 0;
    }

  /* SSL does not handle unknown ASN stuff well, do it by hand.  */
  sandata = subjectaltname->value->data;
  santype = sandata[2] & 0x3f;
  sanlen = sandata[3];
  sandata += 4;

  if (sanlen + 4 != subjectaltname->value->length)
    {
      log_print ("x509_cert_subjectaltname: subjectaltname invalid length");
      return 0;
    }

  *len = sanlen;
  *altname = sandata;

  return santype;
}

int
x509_cert_get_subjects (void *scert, int *cnt, u_int8_t ***id,
			u_int32_t **id_len)
{
  X509 *cert = scert;
  X509_NAME *subject;
  int type;
  u_int8_t *altname;
  u_int32_t altlen;
  char *buf = 0;
  unsigned char *ubuf;
  int i;

  *id = 0;
  *id_len = 0;

  /*
   * XXX There can be a collection of subjectAltNames, but for now
   * I only return the subjectName and a single subjectAltName.
   */
  *cnt = 2;

  *id = calloc (*cnt, sizeof **id);
  if (!*id)
    {
      log_print ("x509_cert_get_subject: malloc (%d) failed",
		 *cnt * sizeof **id);
      goto fail;
    }

  *id_len = malloc (*cnt * sizeof **id_len);
  if (!*id_len)
    {
      log_print ("x509_cert_get_subject: malloc (%d) failed",
		 *cnt * sizeof **id_len);
      goto fail;
    }

  /* Stash the subjectName into the first slot.  */
  subject = LC (X509_get_subject_name, (cert));
  if (!subject)
    goto fail;


  (*id_len)[0] =
    ISAKMP_ID_DATA_OFF + LC (i2d_X509_NAME, (subject, NULL)) - ISAKMP_GEN_SZ;
  (*id)[0] = malloc ((*id_len)[0]);
  if (!(*id)[0])
    {
      log_print ("x509_cert_get_subject: malloc (%d) failed", (*id_len)[0]);
      goto fail;
    }
  SET_ISAKMP_ID_TYPE ((*id)[0] - ISAKMP_GEN_SZ, IPSEC_ID_DER_ASN1_DN);
  ubuf = (*id)[0] + ISAKMP_ID_DATA_OFF - ISAKMP_GEN_SZ;
  LC (i2d_X509_NAME, (subject, &ubuf));

  /* Stash the subjectAltName into the second slot.  */
  type = x509_cert_subjectaltname (cert, &altname, &altlen);
  if (!type)
    goto fail;

  buf = malloc (altlen + ISAKMP_ID_DATA_OFF);
  if (!buf)
    {
      log_print ("x509_cert_get_subject: malloc (%d) failed",
		 altlen + ISAKMP_ID_DATA_OFF);
      goto fail;
    }

  switch (type)
    {
    case X509v3_DNS_NAME:
      SET_ISAKMP_ID_TYPE (buf, IPSEC_ID_FQDN);
      break;

    case X509v3_RFC_NAME:
      SET_ISAKMP_ID_TYPE (buf, IPSEC_ID_USER_FQDN);
      break;

    case X509v3_IP_ADDR:
      /*
       * XXX I dislike the numeric constants, but I don't know what we
       * should use otherwise.
       */
      switch (altlen)
	{
	case 4:
	  SET_ISAKMP_ID_TYPE (buf, IPSEC_ID_IPV4_ADDR);
	  break;

	case 16:
	  SET_ISAKMP_ID_TYPE (buf, IPSEC_ID_IPV6_ADDR);
	  break;

	default:
	  log_print ("x509_cert_get_subject: "
		     "invalid subjectAltName iPAdress length %d ", altlen);
	  goto fail;
	}
      break;
    }

  SET_IPSEC_ID_PROTO (buf + ISAKMP_ID_DOI_DATA_OFF, 0);
  SET_IPSEC_ID_PORT (buf + ISAKMP_ID_DOI_DATA_OFF, 0);
  memcpy (buf + ISAKMP_ID_DATA_OFF, altname, altlen);

  (*id_len)[1] = ISAKMP_ID_DATA_OFF + altlen - ISAKMP_GEN_SZ;
  (*id)[1] = malloc ((*id_len)[1]);
  if (!(*id)[1])
    {
      log_print ("x509_cert_get_subject: malloc (%d) failed", (*id_len)[1]);
      goto fail;
    }
  memcpy ((*id)[1], buf + ISAKMP_GEN_SZ, (*id_len)[1]);

  free (buf);
  buf = 0;
  return 1;

 fail:
  for (i = 0; i < *cnt; i++)
    if ((*id)[i])
      free ((*id)[i]);
  if (*id)
    free (*id);
  if (*id_len)
    free (*id_len);
  if (buf)
    free (buf);
  return 0;
}

int
x509_cert_get_key (void *scert, void *keyp)
{
  X509 *cert = scert;
  EVP_PKEY *key;

  key = LC (X509_get_pubkey, (cert));

  /* Check if we got the right key type.  */
  if (key->type != EVP_PKEY_RSA)
    {
      log_print ("x509_cert_get_key: public key is not a RSA key");
      LC (X509_free, (cert));
      return 0;
    }

  *(RSA **)keyp = LC (RSAPublicKey_dup, (key->pkey.rsa));

  return *(RSA **)keyp == NULL ? 0 : 1;
}

#endif /* USE_X509 */
