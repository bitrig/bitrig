/*	$OpenBSD: omsdhc.c,v 1.13 2014/11/04 13:18:04 jsg Exp $	*/

/*
 * Copyright (c) 2009 Dale Rahn <drahn@openbsd.org>
 * Copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
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
/*-
 * Copyright (c) 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <machine/bus.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcvar.h>

#include <armv7/armv7/armv7var.h>
#include <armv7/omap/prcmvar.h>

/*
 * NOTE: on OMAP4430/AM335x these registers skew by 0x100
 * this is handled by mapping at base address + 0x100
 */
/* registers */
#define MMCHS_SYSCONFIG	0x010
#define  MMCHS_SYSCONFIG_CLOCKACTIVITY_MASK	(3 << 8)
#define  MMCHS_SYSCONFIG_CLOCKACTIVITY_FCLK	(2 << 8)
#define  MMCHS_SYSCONFIG_CLOCKACTIVITY_ICLK	(1 << 8)
#define  MMCHS_SYSCONFIG_SIDLEMODE_MASK		(3 << 3)
#define  MMCHS_SYSCONFIG_SIDLEMODE_AUTO		(2 << 3)
#define  MMCHS_SYSCONFIG_ENAWAKEUP		(1 << 2)
#define  MMCHS_SYSCONFIG_SOFTRESET		(1 << 1)
#define  MMCHS_SYSCONFIG_AUTOIDLE		(1 << 0)
#define MMCHS_SYSSTATUS	0x014
#define  MMCHS_SYSSTATUS_RESETDONE	(1 << 0)
#define MMCHS_CSRE	0x024
#define MMCHS_SYSTEST	0x028
#define  MMCHS_SYSTEST_SDCD	(1 << 15)
#define MMCHS_CON	0x02C
#define  MMCHS_CON_INIT	(1<<1)
#define  MMCHS_CON_DW8	(1<<5)
#define  MMCHS_CON_OD	(1<<0)
#define MMCHS_PWCNT	0x030
#define MMCHS_BLK	0x104
#define  MMCHS_BLK_NBLK_MAX	0xffff
#define  MMCHS_BLK_NBLK_SHIFT	16
#define  MMCHS_BLK_NBLK_MASK	(MMCHS_BLK_NBLK_MAX<<MMCHS_BLK_NBLK_SHIFT)
#define  MMCHS_BLK_BLEN_MAX	0x400
#define  MMCHS_BLK_BLEN_SHIFT	0
#define  MMCHS_BLK_BLEN_MASK	(MMCHS_BLK_BLEN_MAX<<MMCHS_BLK_BLEN_SHIFT)
#define MMCHS_ARG	0x108
#define MMCHS_CMD	0x10C
#define  MMCHS_CMD_INDX_SHIFT		24
#define  MMCHS_CMD_INDX_SHIFT_MASK	(0x3f << MMCHS_CMD_INDX_SHIFT)
#define	 MMCHS_CMD_CMD_TYPE_SHIFT	22
#define	 MMCHS_CMD_DP_SHIFT		21
#define	 MMCHS_CMD_DP			(1 << MMCHS_CMD_DP_SHIFT)
#define	 MMCHS_CMD_CICE_SHIFT		20
#define	 MMCHS_CMD_CICE			(1 << MMCHS_CMD_CICE_SHIFT)
#define	 MMCHS_CMD_CCCE_SHIFT		19
#define	 MMCHS_CMD_CCCE			(1 << MMCHS_CMD_CCCE_SHIFT)
#define	 MMCHS_CMD_RSP_TYPE_SHIFT	16
#define  MMCHS_CMD_RESP_NONE		(0x0 << MMCHS_CMD_RSP_TYPE_SHIFT)
#define  MMCHS_CMD_RESP136		(0x1 << MMCHS_CMD_RSP_TYPE_SHIFT)
#define  MMCHS_CMD_RESP48		(0x2 << MMCHS_CMD_RSP_TYPE_SHIFT)
#define  MMCHS_CMD_RESP48B		(0x3 << MMCHS_CMD_RSP_TYPE_SHIFT)
#define  MMCHS_CMD_MSBS			(1 << 5)
#define  MMCHS_CMD_DDIR			(1 << 4)
#define  MMCHS_CMD_ACEN			(1 << 2)
#define  MMCHS_CMD_BCE			(1 << 1)
#define  MMCHS_CMD_DE			(1 << 0)
#define MMCHS_RSP10	0x110
#define MMCHS_RSP32	0x114
#define MMCHS_RSP54	0x118
#define MMCHS_RSP76	0x11C
#define MMCHS_DATA	0x120
#define MMCHS_PSTATE	0x124
#define  MMCHS_PSTATE_CLEV	(1<<24)
#define  MMCHS_PSTATE_DLEV_SH	20
#define  MMCHS_PSTATE_DLEV_M	(0xf << MMCHS_PSTATE_DLEV_SH)
#define  MMCHS_PSTATE_BRE	(1<<11)
#define  MMCHS_PSTATE_BWE	(1<<10)
#define  MMCHS_PSTATE_RTA	(1<<9)
#define  MMCHS_PSTATE_WTA	(1<<8)
#define  MMCHS_PSTATE_DLA	(1<<2)
#define  MMCHS_PSTATE_DATI	(1<<1)
#define  MMCHS_PSTATE_CMDI	(1<<0)
#define  MMCHS_PSTATE_FMT "\20" \
    "\x098_CLEV" \
    "\x08b_BRE" \
    "\x08a_BWE" \
    "\x089_RTA" \
    "\x088_WTA" \
    "\x082_DLA" \
    "\x081_DATI" \
    "\x080_CMDI"
#define MMCHS_HCTL	0x128
#define  MMCHS_HCTL_SDVS_SHIFT	9
#define  MMCHS_HCTL_SDVS_MASK	(0x7<<MMCHS_HCTL_SDVS_SHIFT)
#define  MMCHS_HCTL_SDVS_V18	(0x5<<MMCHS_HCTL_SDVS_SHIFT)
#define  MMCHS_HCTL_SDVS_V30	(0x6<<MMCHS_HCTL_SDVS_SHIFT)
#define  MMCHS_HCTL_SDVS_V33	(0x7<<MMCHS_HCTL_SDVS_SHIFT)
#define  MMCHS_HCTL_SDBP	(1<<8)
#define  MMCHS_HCTL_HSPE	(1<<2)
#define  MMCHS_HCTL_DTW		(1<<1)
#define MMCHS_SYSCTL	0x12C
#define  MMCHS_SYSCTL_SRD	(1<<26)
#define  MMCHS_SYSCTL_SRC	(1<<25)
#define  MMCHS_SYSCTL_SRA	(1<<24)
#define  MMCHS_SYSCTL_DTO_SH	16
#define  MMCHS_SYSCTL_DTO_MASK	0x000f0000
#define  MMCHS_SYSCTL_CLKD_SH	6
#define  MMCHS_SYSCTL_CLKD_MASK	0x0000ffc0
#define  MMCHS_SYSCTL_CEN	(1<<2)
#define  MMCHS_SYSCTL_ICS	(1<<1)
#define  MMCHS_SYSCTL_ICE	(1<<0)
#define MMCHS_STAT	0x130
#define  MMCHS_STAT_BADA	(1<<29)
#define  MMCHS_STAT_CERR	(1<<28)
#define  MMCHS_STAT_ACE		(1<<24)
#define  MMCHS_STAT_DEB		(1<<22)
#define  MMCHS_STAT_DCRC	(1<<21)
#define  MMCHS_STAT_DTO		(1<<20)
#define  MMCHS_STAT_CIE		(1<<19)
#define  MMCHS_STAT_CEB		(1<<18)
#define  MMCHS_STAT_CCRC	(1<<17)
#define  MMCHS_STAT_CTO		(1<<16)
#define  MMCHS_STAT_ERRI	(1<<15)
#define  MMCHS_STAT_OBI		(1<<9)
#define  MMCHS_STAT_CIRQ	(1<<8)
#define  MMCHS_STAT_BRR		(1<<5)
#define  MMCHS_STAT_BWR		(1<<4)
#define  MMCHS_STAT_BGE		(1<<2)
#define  MMCHS_STAT_TC		(1<<1)
#define  MMCHS_STAT_CC		(1<<0)
#define  MMCHS_STAT_FMT "\20" \
    "\x09d_BADA" \
    "\x09c_CERR" \
    "\x098_ACE" \
    "\x096_DEB" \
    "\x095_DCRC" \
    "\x094_DTO" \
    "\x093_CIE" \
    "\x092_CEB" \
    "\x091_CCRC" \
    "\x090_CTO" \
    "\x08f_ERRI" \
    "\x089_OBI" \
    "\x088_CIRQ" \
    "\x085_BRR" \
    "\x084_BWR" \
    "\x082_BGE" \
    "\x081_TC" \
    "\x080_CC"
#define MMCHS_IE	0x134
#define MMCHS_ISE	0x138
#define MMCHS_AC12	0x13C
#define MMCHS_CAPA	0x140
#define  MMCHS_CAPA_VS18	(1 << 26)
#define  MMCHS_CAPA_VS30	(1 << 25)
#define  MMCHS_CAPA_VS33	(1 << 24)
#define  MMCHS_CAPA_SRS		(1 << 23)
#define  MMCHS_CAPA_DS		(1 << 22)
#define  MMCHS_CAPA_HSS		(1 << 21)
#define  MMCHS_CAPA_MBL_SHIFT	16
#define  MMCHS_CAPA_MBL_MASK	(3 << MMCHS_CAPA_MBL_SHIFT)
#define MMCHS_CUR_CAPA	0x148
#define MMCHS_REV	0x1fc

#define CLKD(kz)	(ssc->sc_clkbase / (kz))

#define SDHC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_iot, (sc)->sc_sdhc_ioh, (reg))
#define SDHC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_iot, (sc)->sc_sdhc_ioh, (reg), (val))

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

void	omsdhc_attach(struct device *, struct device *, void *);
int	omsdhc_card_detect(struct sdhc_softc *);
int	omsdhc_bus_clock(struct sdhc_softc *, int);
int	omsdhc_bus_rod(struct sdhc_softc *, int);

struct omsdhc_softc {
	struct device		 sc_dev;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
	bus_space_handle_t	 sc_sdhc_ioh;
	void			*sc_ih;
};

struct sdhc_omap_softc {
	struct sdhc_softc	 sc;
	bus_space_tag_t		 sc_iot;
	bus_space_handle_t	 sc_ioh;
};

struct cfdriver omsdhc_cd = {
	NULL, "omsdhc", DV_DULL
};

struct cfattach omsdhc_ca = {
	sizeof(struct omsdhc_softc), NULL, omsdhc_attach
};

void
omsdhc_attach(struct device *parent, struct device *self, void *args)
{
	struct omsdhc_softc		*sc = (struct omsdhc_softc *) self;
	struct armv7_attach_args	*aa = args;
	struct sdhc_omap_softc *soc;
	struct sdhc_softc *ssc;
	uint32_t clkd, stat;
	int timo, clksft, n;

	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(sc->sc_iot, aa->aa_dev->mem[0].addr,
	    aa->aa_dev->mem[0].size, 0, &sc->sc_ioh))
		panic("%s: bus_space_map failed!", __func__);

	bus_space_subregion(sc->sc_iot, sc->sc_ioh, 0x100, 0x100,
	    &sc->sc_sdhc_ioh);

	printf("\n");

	if ((ssc = (struct sdhc_softc *)config_found(self, NULL, NULL)) == NULL)
		goto err;
	soc = (struct sdhc_omap_softc *)ssc;

	soc->sc_iot = sc->sc_iot;
	soc->sc_ioh = sc->sc_ioh;
	ssc->sc_dmat = aa->aa_dmat;
	ssc->sc_clkbase = 96 * 1000;
	ssc->sc_clkmsk = 0x0000ffc0;
	ssc->sc_flags = 0;
	ssc->sc_flags |= SDHC_F_32BIT_ACCESS;
	ssc->sc_flags |= SDHC_F_NO_LED_ON;
	ssc->sc_flags |= SDHC_F_RSP136_CRC;
	ssc->sc_flags |= SDHC_F_WAIT_RESET;
	if (board_id == BOARD_ID_OMAP3_BEAGLE) {
		ssc->sc_clkmsk = 0;
		ssc->sc_flags |= SDHC_F_8BIT_MODE;
		ssc->sc_flags |= SDHC_F_SINGLE_ONLY; /* Pre ES3 */
		ssc->sc_flags &= ~SDHC_F_WAIT_RESET;
	}
	ssc->sc_vendor_rod = omsdhc_bus_rod;
	ssc->sc_vendor_card_detect = omsdhc_card_detect;
	ssc->sc_vendor_bus_clock = omsdhc_bus_clock;

	clksft = ffs(ssc->sc_clkmsk) - 1;

	/* Enable ICLKEN, FCLKEN? */
	prcm_enablemodule(PRCM_MMC0 + aa->aa_dev->unit);

	/* Reset hardware. */
	HWRITE4(sc, MMCHS_SYSCONFIG, MMCHS_SYSCONFIG_SOFTRESET);

	timo = 3000000; /* XXXX 3 sec. */
	while (timo--) {
		if (HREAD4(sc, MMCHS_SYSSTATUS) & MMCHS_SYSSTATUS_RESETDONE)
			break;
		delay(1);
	}
	if (timo == 0)
		printf("%s: soft reset timeout\n", DEVNAME(sc));

	HWRITE4(sc, MMCHS_SYSCONFIG, MMCHS_SYSCONFIG_ENAWAKEUP |
	    MMCHS_SYSCONFIG_AUTOIDLE | MMCHS_SYSCONFIG_SIDLEMODE_AUTO |
	    MMCHS_SYSCONFIG_CLOCKACTIVITY_FCLK |
	    MMCHS_SYSCONFIG_CLOCKACTIVITY_ICLK);

	/* Allocate an array big enough to hold all the possible hosts */
	ssc->sc_host = mallocarray(1, sizeof(struct sdhc_host *),
	    M_DEVBUF, M_WAITOK);

	sc->sc_ih = arm_intr_establish(aa->aa_dev->irq[0], IPL_SDMMC,
	    sdhc_intr, ssc, DEVNAME(sc));
	if (sc->sc_ih == NULL) {
		printf("%s: cannot map interrupt\n", DEVNAME(sc));
		goto err;
	}

	if (sdhc_host_found(ssc, sc->sc_iot, sc->sc_sdhc_ioh,
	    aa->aa_dev->mem[0].size) != 0) {
		/* XXX: sc->sc_host leak */
		printf("%s: can't initialize host\n", ssc->sc_dev.dv_xname);
		goto err;
	}

	/* Set SDVS 1.8v and DTW 1bit mode */
	SDHC_WRITE(sc, SDHC_HOST_CTL,
	    SDHC_VOLTAGE_1_8V << (SDHC_VOLTAGE_SHIFT + 8));
	HWRITE4(sc, MMCHS_CON,
	    HREAD4(sc, MMCHS_CON) | MMCHS_CON_OD);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | SDHC_INTCLK_ENABLE |
	    SDHC_SDCLK_ENABLE);
	SDHC_WRITE(sc, SDHC_HOST_CTL,
	    SDHC_READ(sc, SDHC_HOST_CTL) | SDHC_BUS_POWER << 8);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | CLKD(150) << clksft);

	/*
	 * 22.6.1.3.1.5 MMCHS Controller INIT Procedure Start
	 * from 'OMAP35x Applications Processor  Technical Reference Manual'.
	 *
	 * During the INIT procedure, the MMCHS controller generates 80 clock
	 * periods. In order to keep the 1ms gap, the MMCHS controller should
	 * be configured to generate a clock whose frequency is smaller or
	 * equal to 80 KHz.
	 */

	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) & ~SDHC_SDCLK_ENABLE);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) & ~ssc->sc_clkmsk);
	clkd = CLKD(80);
	n = 1;
	while (clkd & ~(ssc->sc_clkmsk >> clksft)) {
		clkd >>= 1;
		n <<= 1;
	}
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | (clkd << clksft));
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | SDHC_SDCLK_ENABLE);

	HWRITE4(sc, MMCHS_CON,
	    HREAD4(sc, MMCHS_CON) | MMCHS_CON_INIT);
	for (; n > 0; n--) {
		SDHC_WRITE(sc, SDHC_TRANSFER_MODE, 0x00000000);
		timo = 3000000; /* XXXX 3 sec. */
		stat = 0;
		while (!(stat & SDHC_COMMAND_COMPLETE)) {
			stat = SDHC_READ(sc, SDHC_NINTR_STATUS);
			if (--timo == 0)
				break;
			delay(1);
		}
		if (timo == 0) {
			printf("%s: INIT Procedure timeout\n", DEVNAME(sc));
			break;
		}
		SDHC_WRITE(sc, SDHC_NINTR_STATUS, stat);
	}
	HWRITE4(sc, MMCHS_CON,
	    HREAD4(sc, MMCHS_CON) & ~MMCHS_CON_INIT);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) & ~SDHC_SDCLK_ENABLE);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) & ~ssc->sc_clkmsk);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | CLKD(150) << clksft);
	SDHC_WRITE(sc, SDHC_CLOCK_CTL,
	    SDHC_READ(sc, SDHC_CLOCK_CTL) | SDHC_SDCLK_ENABLE);

	return;

err:
	if (sc->sc_ih != NULL)
		arm_intr_disestablish(sc->sc_ih);
	bus_space_unmap(sc->sc_iot, sc->sc_ioh, aa->aa_dev->mem[0].size);
}


/*
 * Return non-zero if the card is currently inserted.
 */
int
omsdhc_card_detect(struct sdhc_softc *ssc)
{
	struct sdhc_omap_softc *sc = (struct sdhc_omap_softc *)ssc;

	return !ISSET(HREAD4(sc, MMCHS_SYSTEST), MMCHS_SYSTEST_SDCD) ? 1 : 0;
}

/*
 * Enable or disable the SD clock.
 */
int
omsdhc_bus_clock(struct sdhc_softc *ssc, int freq)
{
	struct sdhc_omap_softc *sc = (struct sdhc_omap_softc *)ssc;

	if (freq == 0)
		HCLR4(sc, MMCHS_SYSCTL, MMCHS_SYSCTL_CEN);
	else
		HSET4(sc, MMCHS_SYSCTL, MMCHS_SYSCTL_CEN);

	return 0;
}

int
omsdhc_bus_rod(struct sdhc_softc *ssc, int on)
{
	struct sdhc_omap_softc *sc = (struct sdhc_omap_softc *)ssc;

	if (on)
		HSET4(sc, MMCHS_CON, MMCHS_CON_OD);
	else
		HCLR4(sc, MMCHS_CON, MMCHS_CON_OD);

	return 0;
}

int 	sdhc_omap_match(struct device *, void *, void *);
void	sdhc_omap_attach(struct device *, struct device *, void *);

struct cfattach sdhc_omap_ca = {
	sizeof (struct sdhc_omap_softc), sdhc_omap_match, sdhc_omap_attach
};

int
sdhc_omap_match(struct device *parent, void *v, void *aux)
{
	return 1;
}

void
sdhc_omap_attach(struct device *parent, struct device *self, void *aux)
{
	printf("\n");
}
