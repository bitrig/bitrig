/*	$OpenBSD: sdhc.c,v 1.38 2014/07/12 18:48:52 tedu Exp $	*/

/*
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

/*
 * SD Host Controller driver based on the SD Host Controller Standard
 * Simplified Specification Version 1.00 (www.sdcard.com).
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/kthread.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <dev/sdmmc/sdhcreg.h>
#include <dev/sdmmc/sdhcvar.h>
#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/sdmmc/sdmmcvar.h>

#define SDHC_COMMAND_TIMEOUT	hz
#define SDHC_BUFFER_TIMEOUT	hz
#define SDHC_TRANSFER_TIMEOUT	hz
#define SDHC_DMA_TIMEOUT	hz

struct sdhc_host {
	struct sdhc_softc *sc;		/* host controller device */
	struct device *sdmmc;		/* generic SD/MMC device */
	bus_space_tag_t iot;		/* host register set tag */
	bus_space_handle_t ioh;		/* host register set handle */
	bus_dma_tag_t dmat;		/* host DMA tag */
	u_int clkbase;			/* base clock frequency in KHz */
	int maxblklen;			/* maximum block length */
	int flags;			/* flags for this host */
	int specver;			/* spec. version */
	u_int32_t ocr;			/* OCR value from capabilities */
	u_int8_t regs[14];		/* host controller state */
	u_int16_t intr_status;		/* soft interrupt status */
	u_int16_t intr_error_status;	/* soft error status */
};

/* flag values */
#define SHF_USE_DMA		0x0001
#define SHF_MODE_DMAEN		0x0008 /* needs SDHC_DMA_ENABLE in mode */

static uint8_t
hread1(struct sdhc_host *hp, bus_size_t reg)
{
	if (!ISSET(hp->sc->sc_flags, SDHC_F_32BIT_ACCESS))
		return bus_space_read_1(hp->iot, hp->ioh, reg);
	return bus_space_read_4(hp->iot, hp->ioh, reg & -4) >> (8 * (reg & 3));
}

static uint16_t
hread2(struct sdhc_host *hp, bus_size_t reg)
{
	if (!ISSET(hp->sc->sc_flags, SDHC_F_32BIT_ACCESS))
		return bus_space_read_2(hp->iot, hp->ioh, reg);
	return bus_space_read_4(hp->iot, hp->ioh, reg & -4) >> (8 * (reg & 2));
}


#define HREAD1(hp, reg)		hread1(hp, reg)
#define HREAD2(hp, reg)		hread2(hp, reg)
#define HREAD4(hp, reg)							\
	(bus_space_read_4((hp)->iot, (hp)->ioh, (reg)))

static void
hwrite1(struct sdhc_host *hp, bus_size_t o, uint8_t val)
{
	if (!ISSET(hp->sc->sc_flags, SDHC_F_32BIT_ACCESS)) {
		bus_space_write_1(hp->iot, hp->ioh, o, val);
	} else {
		const size_t shift = 8 * (o & 3);
		o &= -4;
		uint32_t tmp = bus_space_read_4(hp->iot, hp->ioh, o);
		tmp = (val << shift) | (tmp & ~(0xff << shift));
		bus_space_write_4(hp->iot, hp->ioh, o, tmp);
	}
}

static void
hwrite2(struct sdhc_host *hp, bus_size_t o, uint16_t val)
{
	if (!ISSET(hp->sc->sc_flags, SDHC_F_32BIT_ACCESS)) {
		bus_space_write_2(hp->iot, hp->ioh, o, val);
	} else {
		const size_t shift = 8 * (o & 2);
		o &= -4;
		uint32_t tmp = bus_space_read_4(hp->iot, hp->ioh, o);
		tmp = (val << shift) | (tmp & ~(0xffff << shift));
		bus_space_write_4(hp->iot, hp->ioh, o, tmp);
	}
}

#define HWRITE1(hp, reg, val)		hwrite1(hp, reg, val)
#define HWRITE2(hp, reg, val)		hwrite2(hp, reg, val)
#define HWRITE4(hp, reg, val)						\
	bus_space_write_4((hp)->iot, (hp)->ioh, (reg), (val))

#define HCLR1(hp, reg, bits)						\
	HWRITE1((hp), (reg), HREAD1((hp), (reg)) & ~(bits))
#define HCLR2(hp, reg, bits)						\
	HWRITE2((hp), (reg), HREAD2((hp), (reg)) & ~(bits))
#define HSET1(hp, reg, bits)						\
	HWRITE1((hp), (reg), HREAD1((hp), (reg)) | (bits))
#define HSET2(hp, reg, bits)						\
	HWRITE2((hp), (reg), HREAD2((hp), (reg)) | (bits))

int	sdhc_host_reset(sdmmc_chipset_handle_t);
u_int32_t sdhc_host_ocr(sdmmc_chipset_handle_t);
int	sdhc_host_maxblklen(sdmmc_chipset_handle_t);
int	sdhc_card_detect(sdmmc_chipset_handle_t);
int	sdhc_bus_power(sdmmc_chipset_handle_t, u_int32_t);
int	sdhc_bus_clock(sdmmc_chipset_handle_t, int);
int	sdhc_bus_width(sdmmc_chipset_handle_t, int);
int	sdhc_bus_rod(sdmmc_chipset_handle_t, int);
void	sdhc_card_intr_mask(sdmmc_chipset_handle_t, int);
void	sdhc_card_intr_ack(sdmmc_chipset_handle_t);
void	sdhc_exec_command(sdmmc_chipset_handle_t, struct sdmmc_command *);
int	sdhc_start_command(struct sdhc_host *, struct sdmmc_command *);
int	sdhc_wait_state(struct sdhc_host *, u_int32_t, u_int32_t);
int	sdhc_soft_reset(struct sdhc_host *, int);
int	sdhc_wait_intr(struct sdhc_host *, int, int);
void	sdhc_transfer_data(struct sdhc_host *, struct sdmmc_command *);
int	sdhc_transfer_data_dma(struct sdhc_host *, struct sdmmc_command *);
int	sdhc_transfer_data_pio(struct sdhc_host *, struct sdmmc_command *);
void	sdhc_read_data(struct sdhc_host *, u_char *, int);
void	sdhc_write_data(struct sdhc_host *, u_char *, int);

#ifdef SDHC_DEBUG
int sdhcdebug = 0;
#define DPRINTF(n,s)	do { if ((n) <= sdhcdebug) printf s; } while (0)
void	sdhc_dump_regs(struct sdhc_host *);
#else
#define DPRINTF(n,s)	do {} while(0)
#endif

struct sdmmc_chip_functions sdhc_functions = {
	/* host controller reset */
	sdhc_host_reset,
	/* host controller capabilities */
	sdhc_host_ocr,
	sdhc_host_maxblklen,
	/* card detection */
	sdhc_card_detect,
	/* bus power, clock frequency and width */
	sdhc_bus_power,
	sdhc_bus_clock,
	sdhc_bus_width,
	sdhc_bus_rod,
	/* command execution */
	sdhc_exec_command,
	/* card interrupt */
	sdhc_card_intr_mask,
	sdhc_card_intr_ack
};

struct cfdriver sdhc_cd = {
	NULL, "sdhc", DV_DULL
};

/*
 * Called by attachment driver.  For each SD card slot there is one SD
 * host controller standard register set. (1.3)
 */
int
sdhc_host_found(struct sdhc_softc *sc, bus_space_tag_t iot,
    bus_space_handle_t ioh, bus_size_t iosize)
{
	struct sdmmcbus_attach_args saa;
	struct sdhc_host *hp;
	uint32_t caps = 0;
	uint16_t version;
	int error = 1;

	/* Allocate one more host structure. */
	sc->sc_nhosts++;
	hp = malloc(sizeof(*hp), M_DEVBUF, M_WAITOK | M_ZERO);
	sc->sc_host[sc->sc_nhosts - 1] = hp;

	/* Fill in the new host structure. */
	hp->sc = sc;
	hp->iot = iot;
	hp->ioh = ioh;
	hp->dmat = sc->sc_dmat;

	version = HREAD2(hp, SDHC_HOST_CTL_VERSION);
	hp->specver = SDHC_SPEC_VERSION(version);

#ifdef SDHC_DEBUG
	printf("%s: SD Host Specification/Vendor Version ",
	    sc->sc_dev.dv_xname);
	switch(SDHC_SPEC_VERSION(version)) {
	case 0x00:
		printf("1.0/%u\n", SDHC_VENDOR_VERSION(version));
		break;
	default:
		printf(">1.0/%u\n", SDHC_VENDOR_VERSION(version));
		break;
	}
#endif

	/*
	 * Reset the host controller and enable interrupts.
	 */
	(void)sdhc_host_reset(hp);

	/* Determine host capabilities. */
	if (ISSET(sc->sc_flags, SDHC_F_HOSTCAPS))
		caps = sc->sc_caps;
	else
		caps = HREAD4(hp, SDHC_CAPABILITIES);

	/* Use DMA if the host system and the controller support it. */
	if (ISSET(sc->sc_flags, SDHC_F_FORCE_DMA) ||
	    (ISSET(sc->sc_flags, SDHC_F_USE_DMA) &&
	    ISSET(caps, SDHC_DMA_SUPPORT))) {
		SET(hp->flags, SHF_USE_DMA);
		if (!ISSET(sc->sc_flags, SDHC_F_EXTERNAL_DMA) ||
		    ISSET(sc->sc_flags, SDHC_F_EXTDMA_DMAEN))
			SET(hp->flags, SHF_MODE_DMAEN);
	}

	/*
	 * Determine the base clock frequency. (2.2.24)
	 */
	if (hp->specver >= SDHC_SPEC_VERS_300) {
		hp->clkbase = SDHC_BASE_V3_FREQ_KHZ(caps);
	} else {
		hp->clkbase = SDHC_BASE_FREQ_KHZ(caps);
	}
	if (hp->clkbase == 0) {
		if (sc->sc_clkbase == 0) {
			/* The attachment driver must tell us. */
			printf("%s: base clock frequency unknown\n",
			    sc->sc_dev.dv_xname);
			goto err;
		}
		hp->clkbase = sc->sc_clkbase;
	}
	if (hp->clkbase < 10000 || hp->clkbase > 10000 * 256) {
		/* SDHC 1.0 supports only 10-63 MHz. */
		printf("%s: base clock frequency out of range: %u MHz\n",
		    sc->sc_dev.dv_xname, hp->clkbase / 1000);
		goto err;
	}

	/*
	 * XXX Set the data timeout counter value according to
	 * capabilities. (2.2.15)
	 */
	HWRITE1(hp, SDHC_TIMEOUT_CTL, SDHC_TIMEOUT_MAX);

	/*
	 * Determine SD bus voltage levels supported by the controller.
	 */
	if (ISSET(caps, SDHC_VOLTAGE_SUPP_1_8V) &&
	    hp->specver < SDHC_SPEC_VERS_300)
		SET(hp->ocr, MMC_OCR_1_7V_1_8V | MMC_OCR_1_8V_1_9V);
	if (ISSET(caps, SDHC_VOLTAGE_SUPP_3_0V))
		SET(hp->ocr, MMC_OCR_2_9V_3_0V | MMC_OCR_3_0V_3_1V);
	if (ISSET(caps, SDHC_VOLTAGE_SUPP_3_3V))
		SET(hp->ocr, MMC_OCR_3_2V_3_3V | MMC_OCR_3_3V_3_4V);

	/*
	 * Determine the maximum block length supported by the host
	 * controller. (2.2.24)
	 */
	switch((caps >> SDHC_MAX_BLK_LEN_SHIFT) & SDHC_MAX_BLK_LEN_MASK) {
	case SDHC_MAX_BLK_LEN_512:
		hp->maxblklen = 512;
		break;
	case SDHC_MAX_BLK_LEN_1024:
		hp->maxblklen = 1024;
		break;
	case SDHC_MAX_BLK_LEN_2048:
		hp->maxblklen = 2048;
		break;
	case SDHC_MAX_BLK_LEN_4096:
		hp->maxblklen = 4096;
		break;
	default:
		hp->maxblklen = 1;
		break;
	}

	/*
	 * Attach the generic SD/MMC bus driver.  (The bus driver must
	 * not invoke any chipset functions before it is attached.)
	 */
	bzero(&saa, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.sct = &sdhc_functions;
	saa.sch = hp;
	saa.dmat = hp->dmat;
	saa.clkmax = hp->clkbase;
	if (hp->sc->sc_clkmsk != 0)
		saa.clkmin = hp->clkbase / (hp->sc->sc_clkmsk >>
		    (ffs(hp->sc->sc_clkmsk) - 1));
	else if (hp->specver >= SDHC_SPEC_VERS_300)
		saa.clkmin = hp->clkbase / 0x3ff;
	else
		saa.clkmin = hp->clkbase / 256;
	saa.caps = SMC_CAPS_4BIT_MODE|SMC_CAPS_AUTO_STOP;
	if (ISSET(sc->sc_flags, SDHC_F_8BIT_MODE))
		saa.caps |= SMC_CAPS_8BIT_MODE;
	if (ISSET(caps, SDHC_HIGH_SPEED_SUPP))
		saa.caps |= SMC_CAPS_SD_HIGHSPEED;
	if (ISSET(hp->flags, SHF_USE_DMA)) {
		saa.caps |= SMC_CAPS_DMA;
		if (!ISSET(hp->sc->sc_flags, SDHC_F_ENHANCED))
			saa.caps |= SMC_CAPS_MULTI_SEG_DMA;
	}
	if (ISSET(sc->sc_flags, SDHC_F_SINGLE_ONLY))
		saa.caps |= SMC_CAPS_SINGLE_ONLY;

	hp->sdmmc = config_found(&sc->sc_dev, &saa, NULL);
	if (hp->sdmmc == NULL) {
		error = 0;
		goto err;
	}
	
	return 0;

err:
	free(hp, M_DEVBUF, 0);
	sc->sc_host[sc->sc_nhosts - 1] = NULL;
	sc->sc_nhosts--;
	return (error);
}

int
sdhc_activate(struct device *self, int act)
{
	struct sdhc_softc *sc = (struct sdhc_softc *)self;
	struct sdhc_host *hp;
	int n, i, rv = 0;

	switch (act) {
	case DVACT_SUSPEND:
		rv = config_activate_children(self, act);

		/* Save the host controller state. */
		for (n = 0; n < sc->sc_nhosts; n++) {
			hp = sc->sc_host[n];
			if (ISSET(sc->sc_flags, SDHC_F_32BIT_ACCESS)) {
				for (i = 0; i < sizeof hp->regs; i += 4) {
					uint32_t v = HREAD4(hp, i);
					hp->regs[i + 0] = (v >> 0);
					hp->regs[i + 1] = (v >> 8);
					if (i + 3 < sizeof hp->regs) {
						hp->regs[i + 2] = (v >> 16);
						hp->regs[i + 3] = (v >> 24);
					}
				}
			} else {
				for (i = 0; i < sizeof hp->regs; i++)
					hp->regs[i] = HREAD1(hp, i);
			}
		}
		break;
	case DVACT_RESUME:
		/* Restore the host controller state. */
		for (n = 0; n < sc->sc_nhosts; n++) {
			hp = sc->sc_host[n];
			(void)sdhc_host_reset(hp);
			if (ISSET(sc->sc_flags, SDHC_F_32BIT_ACCESS)) {
				for (i = 0; i < sizeof hp->regs; i += 4) {
					if (i + 3 < sizeof hp->regs) {
						HWRITE4(hp, i,
						    (hp->regs[i + 0] << 0)
						    | (hp->regs[i + 1] << 8)
						    | (hp->regs[i + 2] << 16)
						    | (hp->regs[i + 3] << 24));
					} else {
						HWRITE4(hp, i,
						    (hp->regs[i + 0] << 0)
						    | (hp->regs[i + 1] << 8));
					}
				}
			} else {
				for (i = 0; i < sizeof hp->regs; i++)
					HWRITE1(hp, i, hp->regs[i]);
			}
		}
		rv = config_activate_children(self, act);
		break;
	case DVACT_POWERDOWN:
		rv = config_activate_children(self, act);
		sdhc_shutdown(self);
		break;
	default:
		rv = config_activate_children(self, act);
		break;
	}
	return (rv);
}

/*
 * Shutdown hook established by or called from attachment driver.
 */
void
sdhc_shutdown(void *arg)
{
	struct sdhc_softc *sc = arg;
	struct sdhc_host *hp;
	int i;

	/* XXX chip locks up if we don't disable it before reboot. */
	for (i = 0; i < sc->sc_nhosts; i++) {
		hp = sc->sc_host[i];
		(void)sdhc_host_reset(hp);
	}
}

/*
 * Reset the host controller.  Called during initialization, when
 * cards are removed, upon resume, and during error recovery.
 */
int
sdhc_host_reset(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = sch;
	u_int16_t imask;
	int error;
	int s;

	s = splsdmmc();

	/* Disable all interrupts. */
	if (ISSET(hp->sc->sc_flags, SDHC_F_32BIT_ACCESS)) {
		HWRITE4(hp, SDHC_NINTR_SIGNAL_EN, 0);
	} else {
		HWRITE2(hp, SDHC_NINTR_SIGNAL_EN, 0);
	}

	/*
	 * Reset the entire host controller and wait up to 100ms for
	 * the controller to clear the reset bit.
	 */
	if ((error = sdhc_soft_reset(hp, SDHC_RESET_ALL)) != 0) {
		splx(s);
		return (error);
	}	

	/* Set data timeout counter value to max for now. */
	HWRITE1(hp, SDHC_TIMEOUT_CTL, SDHC_TIMEOUT_MAX);

	/* Enable interrupts. */
	imask = SDHC_CARD_REMOVAL | SDHC_CARD_INSERTION |
	    SDHC_BUFFER_READ_READY | SDHC_BUFFER_WRITE_READY |
	    SDHC_DMA_INTERRUPT | SDHC_BLOCK_GAP_EVENT |
	    SDHC_TRANSFER_COMPLETE | SDHC_COMMAND_COMPLETE;

	if (ISSET(hp->sc->sc_flags, SDHC_F_32BIT_ACCESS)) {
		imask |= SDHC_EINTR_STATUS_MASK << 16;
		HWRITE4(hp, SDHC_NINTR_STATUS_EN, imask);
		imask ^=
		    (SDHC_EINTR_STATUS_MASK ^ SDHC_EINTR_SIGNAL_MASK) << 16;
		HWRITE4(hp, SDHC_NINTR_SIGNAL_EN, imask);
	} else {
		HWRITE2(hp, SDHC_NINTR_STATUS_EN, imask);
		HWRITE2(hp, SDHC_EINTR_STATUS_EN, SDHC_EINTR_STATUS_MASK);
		HWRITE2(hp, SDHC_NINTR_SIGNAL_EN, imask);
		HWRITE2(hp, SDHC_EINTR_SIGNAL_EN, SDHC_EINTR_SIGNAL_MASK);
	}

	splx(s);
	return 0;
}

u_int32_t
sdhc_host_ocr(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = sch;
	return hp->ocr;
}

int
sdhc_host_maxblklen(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = sch;
	return hp->maxblklen;
}

/*
 * Return non-zero if the card is currently inserted.
 */
int
sdhc_card_detect(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = sch;

	if (hp->sc->sc_vendor_card_detect)
		return (*hp->sc->sc_vendor_card_detect)(hp->sc);

	return ISSET(HREAD4(hp, SDHC_PRESENT_STATE), SDHC_CARD_INSERTED) ?
	    1 : 0;
}

/*
 * Set or change SD bus voltage and enable or disable SD bus power.
 * Return zero on success.
 */
int
sdhc_bus_power(sdmmc_chipset_handle_t sch, u_int32_t ocr)
{
	struct sdhc_host *hp = sch;
	u_int8_t vdd;
	int s;

	s = splsdmmc();

	/*
	 * Disable bus power before voltage change.
	 */
	if (!ISSET(hp->sc->sc_flags, SDHC_F_32BIT_ACCESS)
	    && !ISSET(hp->sc->sc_flags, SDHC_F_NOPWR0))
		HWRITE1(hp, SDHC_POWER_CTL, 0);

	/* If power is disabled, reset the host and return now. */
	if (ocr == 0) {
		splx(s);
		(void)sdhc_host_reset(hp);
		return 0;
	}

	/*
	 * Select the lowest voltage according to capabilities.
	 */
	ocr &= hp->ocr;
	if (ISSET(ocr, MMC_OCR_1_7V_1_8V|MMC_OCR_1_8V_1_9V))
		vdd = SDHC_VOLTAGE_1_8V;
	else if (ISSET(ocr, MMC_OCR_2_9V_3_0V|MMC_OCR_3_0V_3_1V))
		vdd = SDHC_VOLTAGE_3_0V;
	else if (ISSET(ocr, MMC_OCR_3_2V_3_3V|MMC_OCR_3_3V_3_4V))
		vdd = SDHC_VOLTAGE_3_3V;
	else {
		/* Unsupported voltage level requested. */
		splx(s);
		return EINVAL;
	}

	/*
	 * Enable bus power.  Wait at least 1 ms (or 74 clocks) plus
	 * voltage ramp until power rises.
	 */
	HWRITE1(hp, SDHC_POWER_CTL, (vdd << SDHC_VOLTAGE_SHIFT) |
	    SDHC_BUS_POWER);
	sdmmc_delay(10000);

	/*
	 * The host system may not power the bus due to battery low,
	 * etc.  In that case, the host controller should clear the
	 * bus power bit.
	 */
	if (!ISSET(HREAD1(hp, SDHC_POWER_CTL), SDHC_BUS_POWER)) {
		splx(s);
		return ENXIO;
	}

	splx(s);
	return 0;
}

/*
 * Return the smallest possible base clock frequency divisor value
 * for the CLOCK_CTL register to produce `freq' (KHz).
 */
static bool
sdhc_clock_divisor(struct sdhc_host *hp, u_int freq, u_int *divp)
{
	u_int div;

	if (hp->sc->sc_clkmsk != 0) {
		div = howmany(hp->clkbase, freq);
		if (div > (hp->sc->sc_clkmsk >> (ffs(hp->sc->sc_clkmsk) - 1)))
			return false;
		*divp = div << (ffs(hp->sc->sc_clkmsk) - 1);
		//freq = hp->clkbase / div;
		return true;
	}
	if (hp->specver >= SDHC_SPEC_VERS_300) {
		div = howmany(hp->clkbase, freq);
		div = div > 1 ? howmany(div, 2) : 0;
		if (div > 0x3ff)
			return false;
		*divp = (((div >> 8) & SDHC_SDCLK_XDIV_MASK)
			 << SDHC_SDCLK_XDIV_SHIFT) |
			(((div >> 0) & SDHC_SDCLK_DIV_MASK)
			 << SDHC_SDCLK_DIV_SHIFT);
		//freq = hp->clkbase / div;
		return true;
	} else {
		for (div = 1; div <= 256; div *= 2) {
			if ((hp->clkbase / div) <= freq) {
				*divp = (div / 2) << SDHC_SDCLK_DIV_SHIFT;
				//freq = hp->clkbase / div;
				return true;
			}
		}
		/* No divisor found. */
		return false;
	}
	/* No divisor found. */
	return false;
}

/*
 * Set or change SDCLK frequency or disable the SD clock.
 * Return zero on success.
 */
int
sdhc_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct sdhc_host *hp = sch;
	int s;
	int div;
	int timo;
	int error = 0;

	s = splsdmmc();

#ifdef DIAGNOSTIC
	/* Must not stop the clock if commands are in progress. */
	if (ISSET(HREAD4(hp, SDHC_PRESENT_STATE), SDHC_CMD_INHIBIT_MASK) &&
	    sdhc_card_detect(hp))
		printf("sdhc_sdclk_frequency_select: command in progress\n");
#endif

	if (hp->sc->sc_vendor_bus_clock) {
		error = (*hp->sc->sc_vendor_bus_clock)(hp->sc, freq);
		if (error != 0)
			goto ret;
	}

	/*
	 * Stop SD clock before changing the frequency.
	 */
	HWRITE2(hp, SDHC_CLOCK_CTL, 0);
	if (freq == SDMMC_SDCLK_OFF)
		goto ret;

	/*
	 * Set the minimum base clock frequency divisor.
	 */
	if (!sdhc_clock_divisor(hp, freq, &div)) {
		/* Invalid base clock frequency or `freq' value. */
		error = EINVAL;
		goto ret;
	}
	HWRITE2(hp, SDHC_CLOCK_CTL, div);

	/*
	 * Start internal clock.  Wait 10ms for stabilization.
	 */
	HSET2(hp, SDHC_CLOCK_CTL, SDHC_INTCLK_ENABLE);
	for (timo = 1000; timo > 0; timo--) {
		if (ISSET(HREAD2(hp, SDHC_CLOCK_CTL), SDHC_INTCLK_STABLE))
			break;
		sdmmc_delay(10);
	}
	if (timo == 0) {
		error = ETIMEDOUT;
		goto ret;
	}

	/*
	 * Enable SD clock.
	 */
	HSET2(hp, SDHC_CLOCK_CTL, SDHC_SDCLK_ENABLE);

	if (freq > 25000 &&
	    !ISSET(hp->sc->sc_flags, SDHC_F_NO_HS_BIT))
		HSET1(hp, SDHC_HOST_CTL, SDHC_HIGH_SPEED);
	else
		HCLR1(hp, SDHC_HOST_CTL, SDHC_HIGH_SPEED);

ret:
	splx(s);
	return error;
}

int
sdhc_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;
	int reg;

	switch (width) {
	case 1:
	case 4:
		break;

	case 8:
		if (ISSET(hp->sc->sc_flags, SDHC_F_8BIT_MODE))
			break;
		/* FALLTHROUGH */
	default:
		DPRINTF(0,("%s: unsupported bus width (%d)\n",
		    DEVNAME(hp->sc), width));
		return 1;
	}

	reg = HREAD1(hp, SDHC_HOST_CTL);
	if (ISSET(hp->sc->sc_flags, SDHC_F_ENHANCED)) {
		reg &= ~(SDHC_4BIT_MODE|SDHC_ESDHC_8BIT_MODE);
		if (width == 4)
			reg |= SDHC_4BIT_MODE;
		else if (width == 8)
			reg |= SDHC_ESDHC_8BIT_MODE;
	} else {
		reg &= ~SDHC_4BIT_MODE;
		if (width == 4)
			reg |= SDHC_4BIT_MODE;
	}
	HWRITE1(hp, SDHC_HOST_CTL, reg);

	return 0;
}

int
sdhc_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	struct sdhc_host *hp = (struct sdhc_host *)sch;

	if (hp->sc->sc_vendor_rod)
		return (*hp->sc->sc_vendor_rod)(hp->sc, on);

	return 0;
}

void
sdhc_card_intr_mask(sdmmc_chipset_handle_t sch, int enable)
{
	struct sdhc_host *hp = sch;

	if (enable) {
		HSET2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
		HSET2(hp, SDHC_NINTR_SIGNAL_EN, SDHC_CARD_INTERRUPT);
	} else {
		HCLR2(hp, SDHC_NINTR_SIGNAL_EN, SDHC_CARD_INTERRUPT);
		HCLR2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
	}
}

void
sdhc_card_intr_ack(sdmmc_chipset_handle_t sch)
{
	struct sdhc_host *hp = sch;

	HSET2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
}

int
sdhc_wait_state(struct sdhc_host *hp, u_int32_t mask, u_int32_t value)
{
	u_int32_t state;
	int timeout;

	for (timeout = 10; timeout > 0; timeout--) {
		if (((state = HREAD4(hp, SDHC_PRESENT_STATE)) & mask)
		    == value)
			return 0;
		sdmmc_delay(10000);
	}
	DPRINTF(0,("%s: timeout waiting for %x (state=%b)\n", DEVNAME(hp->sc),
	    value, state, SDHC_PRESENT_STATE_BITS));
	return ETIMEDOUT;
}

void
sdhc_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct sdhc_host *hp = sch;
	int error;

	/*
	 * Start the MMC command, or mark `cmd' as failed and return.
	 */
	error = sdhc_start_command(hp, cmd);
	if (error != 0) {
		cmd->c_error = error;
		SET(cmd->c_flags, SCF_ITSDONE);
		return;
	}

	/*
	 * Wait until the command phase is done, or until the command
	 * is marked done for any other reason.
	 */
	if (!sdhc_wait_intr(hp, SDHC_COMMAND_COMPLETE,
	    SDHC_COMMAND_TIMEOUT)) {
		cmd->c_error = ETIMEDOUT;
		SET(cmd->c_flags, SCF_ITSDONE);
		return;
	}

	/*
	 * The host controller removes bits [0:7] from the response
	 * data (CRC) and we pass the data up unchanged to the bus
	 * driver (without padding).
	 */
	if (cmd->c_error == 0 && ISSET(cmd->c_flags, SCF_RSP_PRESENT)) {
		cmd->c_resp[0] = HREAD4(hp, SDHC_RESPONSE + 0);
		if (ISSET(cmd->c_flags, SCF_RSP_136)) {
			cmd->c_resp[1] = HREAD4(hp, SDHC_RESPONSE + 4);
			cmd->c_resp[2] = HREAD4(hp, SDHC_RESPONSE + 8);
			cmd->c_resp[3] = HREAD4(hp, SDHC_RESPONSE + 12);
			if (ISSET(hp->sc->sc_flags, SDHC_F_RSP136_CRC)) {
				cmd->c_resp[0] = (cmd->c_resp[0] >> 8) |
				    (cmd->c_resp[1] << 24);
				cmd->c_resp[1] = (cmd->c_resp[1] >> 8) |
				    (cmd->c_resp[2] << 24);
				cmd->c_resp[2] = (cmd->c_resp[2] >> 8) |
				    (cmd->c_resp[3] << 24);
				cmd->c_resp[3] = (cmd->c_resp[3] >> 8);
			}
		}
	}

	/*
	 * If the command has data to transfer in any direction,
	 * execute the transfer now.
	 */
	if (cmd->c_error == 0 && cmd->c_data != NULL)
		sdhc_transfer_data(hp, cmd);

	if (!ISSET(hp->sc->sc_flags, SDHC_F_ENHANCED)
	    && !ISSET(hp->sc->sc_flags, SDHC_F_NO_LED_ON)) {
		/* Turn off the LED. */
		HCLR1(hp, SDHC_HOST_CTL, SDHC_LED_ON);
	}

	DPRINTF(1,("%s: cmd %u done (flags=%#x error=%d)\n",
	    DEVNAME(hp->sc), cmd->c_opcode, cmd->c_flags, cmd->c_error));
	SET(cmd->c_flags, SCF_ITSDONE);
}

int
sdhc_start_command(struct sdhc_host *hp, struct sdmmc_command *cmd)
{
	u_int16_t blksize = 0;
	u_int16_t blkcount = 0;
	u_int16_t mode;
	u_int16_t command;
	int error;
	int s;
	
	DPRINTF(1,("%s: start cmd %u arg=%#x data=%#x dlen=%d flags=%#x "
	    "proc=\"%s\"\n", DEVNAME(hp->sc), cmd->c_opcode, cmd->c_arg,
	    cmd->c_data, cmd->c_datalen, cmd->c_flags, curproc ?
	    curproc->p_comm : ""));

	/*
	 * The maximum block length for commands should be the minimum
	 * of the host buffer size and the card buffer size. (1.7.2)
	 */

	/* Fragment the data into proper blocks. */
	if (cmd->c_datalen > 0) {
		blksize = MIN(cmd->c_datalen, cmd->c_blklen);
		blkcount = cmd->c_datalen / blksize;
		if (cmd->c_datalen % blksize > 0) {
			/* XXX: Split this command. (1.7.4) */
			printf("%s: data not a multiple of %d bytes\n",
			    DEVNAME(hp->sc), blksize);
			return EINVAL;
		}
	}

	/* Check limit imposed by 9-bit block count. (1.7.2) */
	if (blkcount > SDHC_BLOCK_COUNT_MAX) {
		printf("%s: too much data\n", DEVNAME(hp->sc));
		return EINVAL;
	}

	/* Prepare transfer mode register value. (2.2.5) */
	mode = 0;
	if (ISSET(cmd->c_flags, SCF_CMD_READ))
		mode |= SDHC_READ_MODE;
	if (blkcount > 0) {
		mode |= SDHC_BLOCK_COUNT_ENABLE;
		if (blkcount > 1) {
			mode |= SDHC_MULTI_BLOCK_MODE;
			/* XXX only for memory commands? */
			mode |= SDHC_AUTO_CMD12_ENABLE;
		}
	}
	if (cmd->c_dmamap != NULL && cmd->c_datalen > 0 &&
	    ISSET(hp->flags, SHF_MODE_DMAEN)) {
		mode |= SDHC_DMA_ENABLE;
	}

	/*
	 * Prepare command register value. (2.2.6)
	 */
	command = (cmd->c_opcode & SDHC_COMMAND_INDEX_MASK) <<
	    SDHC_COMMAND_INDEX_SHIFT;

	if (ISSET(cmd->c_flags, SCF_RSP_CRC))
		command |= SDHC_CRC_CHECK_ENABLE;
	if (ISSET(cmd->c_flags, SCF_RSP_IDX))
		command |= SDHC_INDEX_CHECK_ENABLE;
	if (cmd->c_data != NULL)
		command |= SDHC_DATA_PRESENT_SELECT;

	if (!ISSET(cmd->c_flags, SCF_RSP_PRESENT))
		command |= SDHC_NO_RESPONSE;
	else if (ISSET(cmd->c_flags, SCF_RSP_136))
		command |= SDHC_RESP_LEN_136;
	else if (ISSET(cmd->c_flags, SCF_RSP_BSY))
		command |= SDHC_RESP_LEN_48_CHK_BUSY;
	else
		command |= SDHC_RESP_LEN_48;

	/* Wait until command and data inhibit bits are clear. (1.5) */
	if ((error = sdhc_wait_state(hp, SDHC_CMD_INHIBIT_MASK, 0)) != 0)
		return error;

	s = splsdmmc();

	DPRINTF(1,("%s: cmd=%#x mode=%#x blksize=%d blkcount=%d\n",
	    DEVNAME(hp->sc), command, mode, blksize, blkcount));

	blksize |= (MAX(0, PAGE_SHIFT - 12) & SDHC_DMA_BOUNDARY_MASK) <<
	    SDHC_DMA_BOUNDARY_SHIFT;	/* PAGE_SIZE DMA boundary */

	if (!ISSET(hp->sc->sc_flags, SDHC_F_ENHANCED)) {
		/* Alert the user not to remove the card. */
		HSET1(hp, SDHC_HOST_CTL, SDHC_LED_ON);
	}

	/* Set DMA start address. */
	if (ISSET(mode, SDHC_DMA_ENABLE) &&
	    !ISSET(hp->sc->sc_flags, SDHC_F_EXTERNAL_DMA))
		HWRITE4(hp, SDHC_DMA_ADDR, cmd->c_dmamap->dm_segs[0].ds_addr);

	/*
	 * Start a CPU data transfer.  Writing to the high order byte
	 * of the SDHC_COMMAND register triggers the SD command. (1.5)
	 */
	if (ISSET(hp->sc->sc_flags, SDHC_F_32BIT_ACCESS)) {
		HWRITE4(hp, SDHC_BLOCK_SIZE, blksize | (blkcount << 16));
		HWRITE4(hp, SDHC_ARGUMENT, cmd->c_arg);
		HWRITE4(hp, SDHC_TRANSFER_MODE, mode | (command << 16));
	} else {
		HWRITE2(hp, SDHC_TRANSFER_MODE, mode);
		HWRITE2(hp, SDHC_BLOCK_SIZE, blksize);
		if (blkcount > 1)
			HWRITE2(hp, SDHC_BLOCK_COUNT, blkcount);
		HWRITE4(hp, SDHC_ARGUMENT, cmd->c_arg);
		HWRITE2(hp, SDHC_COMMAND, command);
	}

	splx(s);
	return 0;
}

void
sdhc_transfer_data(struct sdhc_host *hp, struct sdmmc_command *cmd)
{
	struct sdhc_softc *sc = hp->sc;
	int error;

	DPRINTF(1,("%s: resp=%#x datalen=%d\n", DEVNAME(hp->sc),
	    MMC_R1(cmd->c_resp), cmd->c_datalen));

#ifdef SDHC_DEBUG
	/* XXX I forgot why I wanted to know when this happens :-( */
	if ((cmd->c_opcode == 52 || cmd->c_opcode == 53) &&
	    ISSET(MMC_R1(cmd->c_resp), 0xcb00))
		printf("%s: CMD52/53 error response flags %#x\n",
		    DEVNAME(hp->sc), MMC_R1(cmd->c_resp) & 0xff00);
#endif

	if (cmd->c_dmamap != NULL) {
		if (hp->sc->sc_vendor_transfer_data_dma != NULL) {
			error = hp->sc->sc_vendor_transfer_data_dma(sc, cmd);
			if (error == 0 && !sdhc_wait_intr(hp,
			    SDHC_TRANSFER_COMPLETE, SDHC_TRANSFER_TIMEOUT)) {
				error = ETIMEDOUT;
			}
		} else {
			error = sdhc_transfer_data_dma(hp, cmd);
		}
	} else
		error = sdhc_transfer_data_pio(hp, cmd);
	if (error != 0)
		cmd->c_error = error;
	SET(cmd->c_flags, SCF_ITSDONE);

	DPRINTF(1,("%s: data transfer done (error=%d)\n",
	    DEVNAME(hp->sc), cmd->c_error));
}

int
sdhc_transfer_data_dma(struct sdhc_host *hp, struct sdmmc_command *cmd)
{
	bus_dma_segment_t *dm_segs = cmd->c_dmamap->dm_segs;
	bus_addr_t posaddr;
	bus_addr_t segaddr;
	bus_size_t seglen;
	u_int seg = 0;
	int error = 0;
	int status;

	KASSERT(HREAD2(hp, SDHC_NINTR_STATUS_EN) & SDHC_DMA_INTERRUPT);
	KASSERT(HREAD2(hp, SDHC_NINTR_SIGNAL_EN) & SDHC_DMA_INTERRUPT);
	KASSERT(HREAD2(hp, SDHC_NINTR_STATUS_EN) & SDHC_TRANSFER_COMPLETE);
	KASSERT(HREAD2(hp, SDHC_NINTR_SIGNAL_EN) & SDHC_TRANSFER_COMPLETE);

	for (;;) {
		status = sdhc_wait_intr(hp,
		    SDHC_DMA_INTERRUPT|SDHC_TRANSFER_COMPLETE,
		    SDHC_DMA_TIMEOUT);

		if (status & SDHC_TRANSFER_COMPLETE) {
			break;
		}
		if (!status) {
			error = ETIMEDOUT;
			break;
		}
		if ((status & SDHC_DMA_INTERRUPT) == 0) {
			continue;
		}

		/* DMA Interrupt (boundary crossing) */

		segaddr = dm_segs[seg].ds_addr;
		seglen = dm_segs[seg].ds_len;
		posaddr = HREAD4(hp, SDHC_DMA_ADDR);

		if ((seg == (cmd->c_dmamap->dm_nsegs-1)) && (posaddr == (segaddr + seglen))) {
			continue;
		}
		if ((posaddr >= segaddr) && (posaddr < (segaddr + seglen)))
			HWRITE4(hp, SDHC_DMA_ADDR, posaddr);
		else if ((posaddr >= segaddr) && (posaddr == (segaddr + seglen)) && (seg + 1) < cmd->c_dmamap->dm_nsegs)
			HWRITE4(hp, SDHC_DMA_ADDR, dm_segs[++seg].ds_addr);
		KASSERT(seg < cmd->c_dmamap->dm_nsegs);
	}

	return error;
}

int
sdhc_transfer_data_pio(struct sdhc_host *hp, struct sdmmc_command *cmd)
{
	u_char *datap = cmd->c_data;
	int i, datalen, mask;
	int error = 0;

	datalen = cmd->c_datalen;
	mask = ISSET(cmd->c_flags, SCF_CMD_READ) ?
	    SDHC_BUFFER_READ_ENABLE : SDHC_BUFFER_WRITE_ENABLE;

	while (datalen > 0) {
		if (!sdhc_wait_intr(hp, SDHC_BUFFER_READ_READY|
		    SDHC_BUFFER_WRITE_READY, SDHC_BUFFER_TIMEOUT)) {
			error = ETIMEDOUT;
			break;
		}

		if ((error = sdhc_wait_state(hp, mask, mask)) != 0)
			break;

		i = MIN(datalen, cmd->c_blklen);
		if (ISSET(cmd->c_flags, SCF_CMD_READ))
			sdhc_read_data(hp, datap, i);
		else
			sdhc_write_data(hp, datap, i);

		datap += i;
		datalen -= i;
	}

	if (error == 0 && !sdhc_wait_intr(hp, SDHC_TRANSFER_COMPLETE,
	    SDHC_TRANSFER_TIMEOUT))
		error = ETIMEDOUT;

	return error;
}

void
sdhc_read_data(struct sdhc_host *hp, u_char *datap, int datalen)
{
	while (datalen > 3) {
		*(u_int32_t *)datap = HREAD4(hp, SDHC_DATA);
		datap += 4;
		datalen -= 4;
	}
	if (datalen > 0) {
		u_int32_t rv = HREAD4(hp, SDHC_DATA);
		do {
			*datap++ = rv & 0xff;
			rv = rv >> 8;
		} while (--datalen > 0);
	}
}

void
sdhc_write_data(struct sdhc_host *hp, u_char *datap, int datalen)
{
	while (datalen > 3) {
		DPRINTF(3,("%08x\n", *(u_int32_t *)datap));
		HWRITE4(hp, SDHC_DATA, *((u_int32_t *)datap));
		datap += 4;
		datalen -= 4;
	}
	if (datalen > 0) {
		u_int32_t rv = *datap++;
		if (datalen > 1)
			rv |= *datap++ << 8;
		if (datalen > 2)
			rv |= *datap++ << 16;
		DPRINTF(3,("rv %08x\n", rv));
		HWRITE4(hp, SDHC_DATA, rv);
	}
}

/* Prepare for another command. */
int
sdhc_soft_reset(struct sdhc_host *hp, int mask)
{
	int timo;

	DPRINTF(1,("%s: software reset reg=%#x\n", DEVNAME(hp->sc), mask));

	HWRITE1(hp, SDHC_SOFTWARE_RESET, mask);
	for (timo = 10; timo > 0; timo--) {
		if (!ISSET(HREAD1(hp, SDHC_SOFTWARE_RESET), mask))
			break;
		sdmmc_delay(10000);
		HWRITE1(hp, SDHC_SOFTWARE_RESET, 0);
	}
	if (timo == 0) {
		DPRINTF(1,("%s: timeout reg=%#x\n", DEVNAME(hp->sc),
		    HREAD1(hp, SDHC_SOFTWARE_RESET)));
		HWRITE1(hp, SDHC_SOFTWARE_RESET, 0);
		return (ETIMEDOUT);
	}

	return (0);
}

int
sdhc_wait_intr(struct sdhc_host *hp, int mask, int timo)
{
	int status;
	int s;

	mask |= SDHC_ERROR_INTERRUPT;

	s = splsdmmc();
	status = hp->intr_status & mask;
	while (status == 0) {
		if (tsleep(&hp->intr_status, PWAIT, "hcintr", timo)
		    == EWOULDBLOCK) {
			status |= SDHC_ERROR_INTERRUPT;
			break;
		}
		status = hp->intr_status & mask;
	}
	hp->intr_status &= ~status;

	DPRINTF(2,("%s: intr status %#x error %#x\n", DEVNAME(hp->sc), status,
	    hp->intr_error_status));
	
	/* Command timeout has higher priority than command complete. */
	if (ISSET(status, SDHC_ERROR_INTERRUPT)) {
		hp->intr_error_status = 0;
		(void)sdhc_soft_reset(hp, SDHC_RESET_DAT|SDHC_RESET_CMD);
		status = 0;
	}

	splx(s);
	return status;
}

/*
 * Established by attachment driver at interrupt priority IPL_SDMMC.
 */
int
sdhc_intr(void *arg)
{
	struct sdhc_softc *sc = arg;
	int host;
	int done = 0;

	/* We got an interrupt, but we don't know from which slot. */
	for (host = 0; host < sc->sc_nhosts; host++) {
		struct sdhc_host *hp = sc->sc_host[host];
		uint16_t status;
		uint16_t error;

		if (hp == NULL)
			continue;

		if (ISSET(sc->sc_flags, SDHC_F_32BIT_ACCESS)) {
			/* Find out which interrupts are pending. */
			uint32_t xstatus = HREAD4(hp, SDHC_NINTR_STATUS);
			status = xstatus;
			error = xstatus >> 16;
			if (error)
				xstatus |= SDHC_ERROR_INTERRUPT;
			else if (!ISSET(status, SDHC_NINTR_STATUS_MASK))
				continue; /* no interrupt for us */
			/* Acknowledge the interrupts we are about to handle. */
			HWRITE4(hp, SDHC_NINTR_STATUS, xstatus);
		} else {
			/* Find out which interrupts are pending. */
			error = 0;
			status = HREAD2(hp, SDHC_NINTR_STATUS);
			if (!ISSET(status, SDHC_NINTR_STATUS_MASK))
				continue; /* no interrupt for us */
			/* Acknowledge the interrupts we are about to handle. */
			HWRITE2(hp, SDHC_NINTR_STATUS, status);
			if (ISSET(status, SDHC_ERROR_INTERRUPT)) {
				/* Acknowledge error interrupts. */
				error = HREAD2(hp, SDHC_EINTR_STATUS);
				HWRITE2(hp, SDHC_EINTR_STATUS, error);
			}
		}

		DPRINTF(2,("%s: interrupt status=%b error=%b\n", DEVNAME(hp->sc),
		    status, SDHC_NINTR_STATUS_BITS, error, SDHC_EINTR_STATUS_BITS));

		/* Claim this interrupt. */
		done = 1;

		/*
		 * Service error interrupts.
		 */
		if (ISSET(error, SDHC_CMD_TIMEOUT_ERROR|
		    SDHC_DATA_TIMEOUT_ERROR)) {
			hp->intr_error_status |= error;
			hp->intr_status |= status;
			wakeup(&hp->intr_status);
		}

		/*
		 * Wake up the sdmmc event thread to scan for cards.
		 */
		if (ISSET(status, SDHC_CARD_REMOVAL|SDHC_CARD_INSERTION))
			sdmmc_needs_discover(hp->sdmmc);

		/*
		 * Wake up the blocking process to service command
		 * related interrupt(s).
		 */
		if (ISSET(status, SDHC_BUFFER_READ_READY|
		    SDHC_BUFFER_WRITE_READY|SDHC_COMMAND_COMPLETE|
		    SDHC_TRANSFER_COMPLETE|SDHC_DMA_INTERRUPT)) {
			hp->intr_status |= status;
			wakeup(&hp->intr_status);
		}

		/*
		 * Service SD card interrupts.
		 */
		if (ISSET(status, SDHC_CARD_INTERRUPT)) {
			DPRINTF(0,("%s: card interrupt\n", DEVNAME(hp->sc)));
			HCLR2(hp, SDHC_NINTR_STATUS_EN, SDHC_CARD_INTERRUPT);
			sdmmc_card_intr(hp->sdmmc);
		}
	}
	return done;
}

#ifdef SDHC_DEBUG
void
sdhc_dump_regs(struct sdhc_host *hp)
{
	printf("0x%02x PRESENT_STATE:    %b\n", SDHC_PRESENT_STATE,
	    HREAD4(hp, SDHC_PRESENT_STATE), SDHC_PRESENT_STATE_BITS);
	printf("0x%02x POWER_CTL:        %x\n", SDHC_POWER_CTL,
	    HREAD1(hp, SDHC_POWER_CTL));
	printf("0x%02x NINTR_STATUS:     %x\n", SDHC_NINTR_STATUS,
	    HREAD2(hp, SDHC_NINTR_STATUS));
	printf("0x%02x EINTR_STATUS:     %x\n", SDHC_EINTR_STATUS,
	    HREAD2(hp, SDHC_EINTR_STATUS));
	printf("0x%02x NINTR_STATUS_EN:  %x\n", SDHC_NINTR_STATUS_EN,
	    HREAD2(hp, SDHC_NINTR_STATUS_EN));
	printf("0x%02x EINTR_STATUS_EN:  %x\n", SDHC_EINTR_STATUS_EN,
	    HREAD2(hp, SDHC_EINTR_STATUS_EN));
	printf("0x%02x NINTR_SIGNAL_EN:  %x\n", SDHC_NINTR_SIGNAL_EN,
	    HREAD2(hp, SDHC_NINTR_SIGNAL_EN));
	printf("0x%02x EINTR_SIGNAL_EN:  %x\n", SDHC_EINTR_SIGNAL_EN,
	    HREAD2(hp, SDHC_EINTR_SIGNAL_EN));
	printf("0x%02x CAPABILITIES:     %x\n", SDHC_CAPABILITIES,
	    HREAD4(hp, SDHC_CAPABILITIES));
	printf("0x%02x MAX_CAPABILITIES: %x\n", SDHC_MAX_CAPABILITIES,
	    HREAD4(hp, SDHC_MAX_CAPABILITIES));
}
#endif
