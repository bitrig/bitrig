/*
 * Copyright (c) 2014 Patrick Wildt <patrick@blueri.se>
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <arm/mainbus/mainbus.h>

/* Prototypes for functions provided */

int  mainbusmatch(struct device *, void *, void *);
void mainbusattach(struct device *, struct device *, void *);
int  mainbusprint(void *aux, const char *mainbus);
int mainbussearch(struct device *,  void *, void *);

/* attach and device structures for the device */

struct cfattach mainbus_ca = {
	sizeof(struct device), mainbusmatch, mainbusattach
};

struct cfdriver mainbus_cd = {
	NULL, "mainbus", DV_DULL
};

/*
 * int mainbusmatch(struct device *parent, struct cfdata *cf, void *aux)
 */

int
mainbusmatch(struct device *parent, void *cf, void *aux)
{
	return (1);
}

/*
 * void mainbusattach(struct device *parent, struct device *self, void *aux)
 *
 * probe and attach all children
 */

void
mainbusattach(struct device *parent, struct device *self, void *aux)
{
	printf("\n");

	config_search(mainbussearch, self, aux);
}

int
mainbussearch(struct device *parent, void *vcf, void *aux)
{
	struct mainbus_attach_args ma;
	struct cfdata *cf = vcf;

	ma.ma_name = cf->cf_driver->cd_name;

	/* allow for devices to be disabled in UKC */
	if ((*cf->cf_attach->ca_match)(parent, cf, &ma) == 0)
		return 0;

	config_attach(parent, cf, &ma, mainbusprint);
	return 1;
}

/*
 * int mainbusprint(void *aux, const char *mainbus)
 *
 * print routine used during config of children
 */

int
mainbusprint(void *aux, const char *mainbus)
{
	struct mainbus_attach_args *ma = aux;

	if (mainbus != NULL)
		printf("%s at %s", ma->ma_name, mainbus);

	return (UNCONF);
}
