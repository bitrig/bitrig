/*	$NetBSD: mvsdio.c,v 1.5 2014/03/15 13:33:48 kiyohara Exp $	*/
/*
 * Copyright (c) 2010 KIYOHARA Takashi
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/mutex.h>

#include <machine/bus.h>
#include <machine/clock.h>
#include <machine/fdt.h>
#include <armv7/armv7/armv7var.h>
#include <armv7/marvell/mvmbusvar.h>
#include <armv7/marvell/mvsdioreg.h>

#include <dev/sdmmc/sdmmcvar.h>
#include <dev/sdmmc/sdmmcchip.h>

//#define MVSDIO_DEBUG 1
#ifdef MVSDIO_DEBUG
#define DPRINTF(n, x)	if (mvsdio_debug >= (n)) printf x
int mvsdio_debug = MVSDIO_DEBUG;
#else
#define DPRINTF(n, x)
#endif

struct mvsdio_softc {
	struct device sc_dev;
	struct device *sc_sdmmc;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	bus_dma_tag_t sc_dmat;

	struct mutex sc_mtx;

	struct sdmmc_command *sc_exec_cmd;
	uint32_t sc_waitintr;
};

static int mvsdio_match(struct device *, void *, void *);
static void mvsdio_attach(struct device *, struct device *, void *);

static int mvsdio_intr(void *);

static int mvsdio_host_reset(sdmmc_chipset_handle_t);
static uint32_t mvsdio_host_ocr(sdmmc_chipset_handle_t);
static int mvsdio_host_maxblklen(sdmmc_chipset_handle_t);
#ifdef MVSDIO_CARD_DETECT
int MVSDIO_CARD_DETECT(sdmmc_chipset_handle_t);
#else
static int mvsdio_card_detect(sdmmc_chipset_handle_t);
#endif
#if 0
#ifdef MVSDIO_WRITE_PROTECT
int MVSDIO_WRITE_PROTECT(sdmmc_chipset_handle_t);
#else
static int mvsdio_write_protect(sdmmc_chipset_handle_t);
#endif
#endif
static int mvsdio_bus_power(sdmmc_chipset_handle_t, uint32_t);
static int mvsdio_bus_clock(sdmmc_chipset_handle_t, int);
static int mvsdio_bus_width(sdmmc_chipset_handle_t, int);
static int mvsdio_bus_rod(sdmmc_chipset_handle_t, int);
static void mvsdio_exec_command(sdmmc_chipset_handle_t, struct sdmmc_command *);
#if 0
static void mvsdio_card_enable_intr(sdmmc_chipset_handle_t, int);
#endif
static void mvsdio_card_intr_ack(sdmmc_chipset_handle_t);

static void mvsdio_wininit(struct mvsdio_softc *);

static struct sdmmc_chip_functions mvsdio_chip_functions = {
	/* host controller reset */
	.host_reset		= mvsdio_host_reset,

	/* host controller capabilities */
	.host_ocr		= mvsdio_host_ocr,
	.host_maxblklen		= mvsdio_host_maxblklen,

	/* card detection */
#ifdef MVSDIO_CARD_DETECT
	.card_detect		= MVSDIO_CARD_DETECT,
#else
	.card_detect		= mvsdio_card_detect,
#endif

	/* write protect */
#if 0
#ifdef MVSDIO_WRITE_PROTECT
	.write_protect		= MVSDIO_WRITE_PROTECT,
#else
	.write_protect		= mvsdio_write_protect,
#endif
#endif

	/* bus power, clock frequency, width, rod */
	.bus_power		= mvsdio_bus_power,
	.bus_clock		= mvsdio_bus_clock,
	.bus_width		= mvsdio_bus_width,
	.bus_rod		= mvsdio_bus_rod,

	/* command execution */
	.exec_command		= mvsdio_exec_command,

	/* card interrupt */
#if 0
	.card_enable_intr	= mvsdio_card_enable_intr,
#endif
	.card_intr_ack		= mvsdio_card_intr_ack,
};

struct cfdriver mvsdio_cd = {
	NULL, "mvsdio", DV_DULL
};

struct cfattach mvsdio_ca = {
	sizeof (struct mvsdio_softc), mvsdio_match, mvsdio_attach,
};

/* ARGSUSED */
static int
mvsdio_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	if (fdt_node_compatible("marvell,orion-sdio", aa->aa_node))
		return (1);

	return 0;
}

/* ARGSUSED */
static void
mvsdio_attach(struct device *parent, struct device *self, void *aux)
{
	struct mvsdio_softc *sc = (struct mvsdio_softc *)self;
	struct armv7_attach_args *aa = aux;
	struct sdmmcbus_attach_args saa;
	struct fdt_memory mem;
	uint32_t nis, eis;

	printf("\n");

	clk_enable(clk_fdt_get(aa->aa_node, 0));

	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: cannot extract memory", sc->sc_dev.dv_xname);

	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(aa->aa_iot, mem.addr, mem.size, 0, &sc->sc_ioh)) {
		printf("%s: cannot map registers\n", sc->sc_dev.dv_xname);
		return;
	}
	sc->sc_dmat = aa->aa_dmat;

	mtx_init(&sc->sc_mtx, IPL_SDMMC);

	sc->sc_exec_cmd = NULL;
	sc->sc_waitintr = 0;

	arm_intr_establish_fdt(aa->aa_node, IPL_SDMMC, mvsdio_intr, sc,
	    sc->sc_dev.dv_xname);

	mvsdio_wininit(sc);

#if BYTE_ORDER == LITTLE_ENDIAN
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC, HC_BIGENDIAN);
#else
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC, HC_LSBFIRST);
#endif
	nis =
	    NIS_CMDCOMPLETE	/* Command Complete */		|
	    NIS_XFERCOMPLETE	/* Transfer Complete */		|
	    NIS_BLOCKGAPEV	/* Block gap event */		|
	    NIS_DMAINT		/* DMA interrupt */		|
	    NIS_CARDINT		/* Card interrupt */		|
	    NIS_READWAITON	/* Read Wait state is on */	|
	    NIS_SUSPENSEON					|
	    NIS_AUTOCMD12COMPLETE	/* Auto_cmd12 is comp */|
	    NIS_UNEXPECTEDRESPDET				|
	    NIS_ERRINT;			/* Error interrupt */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NIS, nis);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NISE, nis);

#define NIC_DYNAMIC_CONFIG_INTRS	(NIS_CMDCOMPLETE	| \
					 NIS_XFERCOMPLETE	| \
					 NIS_DMAINT		| \
					 NIS_CARDINT		| \
					 NIS_AUTOCMD12COMPLETE)

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NISIE,
	    nis & ~NIC_DYNAMIC_CONFIG_INTRS);

	eis =
	    EIS_CMDTIMEOUTERR	/*Command timeout err*/		|
	    EIS_CMDCRCERR	/* Command CRC Error */		|
	    EIS_CMDENDBITERR	/*Command end bit err*/		|
	    EIS_CMDINDEXERR	/*Command Index Error*/		|
	    EIS_DATATIMEOUTERR	/* Data timeout error */	|
	    EIS_RDDATACRCERR	/* Read data CRC err */		|
	    EIS_RDDATAENDBITERR	/*Rd data end bit err*/		|
	    EIS_AUTOCMD12ERR	/* Auto CMD12 error */		|
	    EIS_CMDSTARTBITERR	/*Cmd start bit error*/		|
	    EIS_XFERSIZEERR	/*Tx size mismatched err*/	|
	    EIS_RESPTBITERR	/* Response T bit err */	|
	    EIS_CRCENDBITERR	/* CRC end bit error */		|
	    EIS_CRCSTARTBITERR	/* CRC start bit err */		|
	    EIS_CRCSTATERR;	/* CRC status error */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_EIS, eis);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_EISE, eis);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_EISIE, eis);

        /*
	 * Attach the generic SD/MMC bus driver.  (The bus driver must
	 * not invoke any chipset functions before it is attached.)
	 */
	memset(&saa, 0, sizeof(saa));
	saa.saa_busname = "sdmmc";
	saa.sct = &mvsdio_chip_functions;
	saa.sch = sc;
	saa.dmat = sc->sc_dmat;
	saa.clkmin = 100;		/* XXXX: 100 kHz from SheevaPlug LSP */
	saa.clkmax = MVSDIO_MAX_CLOCK;
	saa.caps = SMC_CAPS_AUTO_STOP | SMC_CAPS_4BIT_MODE | SMC_CAPS_DMA |
	    SMC_CAPS_SD_HIGHSPEED | SMC_CAPS_MMC_HIGHSPEED;
#ifndef MVSDIO_CARD_DETECT
	saa.caps |= SMC_CAPS_POLL_CARD_DET;
#endif
	sc->sc_sdmmc = config_found(&sc->sc_dev, &saa, NULL);
}

static int
mvsdio_intr(void *arg)
{
	struct mvsdio_softc *sc = (struct mvsdio_softc *)arg;
	struct sdmmc_command *cmd = sc->sc_exec_cmd;
	uint32_t nis, eis;
	int handled = 0, error;

	nis = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NIS);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NIS, nis);

	DPRINTF(3, ("%s: intr: NIS=0x%x, NISE=0x%x, NISIE=0x%x\n",
	    __func__, nis,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NISE),
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NISIE)));

	if (__predict_false(nis & NIS_ERRINT)) {
		sc->sc_exec_cmd = NULL;
		eis = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_EIS);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_EIS, eis);

		DPRINTF(3, ("    EIS=0x%x, EISE=0x%x, EISIE=0x%x\n",
		    eis,
		    bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_EISE),
		    bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_EISIE)));

		if (eis & (EIS_CMDTIMEOUTERR | EIS_DATATIMEOUTERR)) {
			error = ETIMEDOUT;	/* Timeouts */
			DPRINTF(2, ("    Command/Data Timeout (0x%x)\n",
			    eis & (EIS_CMDTIMEOUTERR | EIS_DATATIMEOUTERR)));
		} else {

#define CRC_ERROR	(EIS_CMDCRCERR		| \
			 EIS_RDDATACRCERR	| \
			 EIS_CRCENDBITERR	| \
			 EIS_CRCSTARTBITERR	| \
			 EIS_CRCSTATERR)
			if (eis & CRC_ERROR) {
				error = EIO;		/* CRC errors */
				printf("%s: CRC Error (0x%x)\n",
				  sc->sc_dev.dv_xname, eis & CRC_ERROR);
			}

#define COMMAND_ERROR	(EIS_CMDENDBITERR	| \
			 EIS_CMDINDEXERR	| \
			 EIS_CMDSTARTBITERR)
			if (eis & COMMAND_ERROR) {
				error = EIO;		/*Other command errors*/
				printf("%s: Command Error (0x%x)\n",
				    sc->sc_dev.dv_xname, eis & COMMAND_ERROR);
			}

#define MISC_ERROR	(EIS_RDDATAENDBITERR	| \
			 EIS_AUTOCMD12ERR	| \
			 EIS_XFERSIZEERR	| \
			 EIS_RESPTBITERR)
			if (eis & MISC_ERROR) {
				error = EIO;		/* Misc error */
				printf("%s: Misc Error (0x%x)\n",
				    sc->sc_dev.dv_xname, eis & MISC_ERROR);
			}
		}

		if (cmd != NULL) {
			cmd->c_error = error;
			wakeup(sc);
		}
		handled = 1;
	} else if (cmd != NULL &&
	    ((nis & sc->sc_waitintr) || (nis & NIS_UNEXPECTEDRESPDET))) {
		sc->sc_exec_cmd = NULL;
		sc->sc_waitintr = 0;
		if (cmd->c_flags & SCF_RSP_PRESENT) {
			uint16_t rh[MVSDIO_NRH + 1];
			int i, j;

			if (cmd->c_flags & SCF_RSP_136) {
				for (i = 0; i < MVSDIO_NRH; i++)
					rh[i + 1] = bus_space_read_4(sc->sc_iot,
					    sc->sc_ioh, MVSDIO_RH(i));
				rh[0] = 0;
				for (j = 3, i = 1; j >= 0; j--, i += 2) {
					cmd->c_resp[j] =
					    rh[i - 1] << 30 |
					    rh[i + 0] << 14 |
					    rh[i + 1] >> 2;
				}
				cmd->c_resp[3] &= 0x00ffffff;
			} else {
				for (i = 0; i < 3; i++)
					rh[i] = bus_space_read_4(sc->sc_iot,
					    sc->sc_ioh, MVSDIO_RH(i));
				cmd->c_resp[0] =
				    ((rh[0] & 0x03ff) << 22) |
				    ((rh[1]         ) <<  6) |
				    ((rh[2] & 0x003f) <<  0);
				cmd->c_resp[1] = (rh[0] & 0xfc00) >> 10;
				cmd->c_resp[2] = 0;
				cmd->c_resp[3] = 0;
			}
		}
		if (nis & NIS_UNEXPECTEDRESPDET)
			cmd->c_error = EIO;
		wakeup(sc);
	}

	if (nis & NIS_CARDINT)
		if (bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NISIE) &
		    NIS_CARDINT) {
			sdmmc_card_intr(sc->sc_sdmmc);
			handled = 1;
		}

	return handled;
}

static int
mvsdio_host_reset(sdmmc_chipset_handle_t sch)
{
	struct mvsdio_softc *sc = (struct mvsdio_softc *)sch;

	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_SR, SR_SWRESET);
	return 0;
}

static uint32_t
mvsdio_host_ocr(sdmmc_chipset_handle_t sch)
{

	return MMC_OCR_3_3V_3_4V | MMC_OCR_3_2V_3_3V;
}

static int
mvsdio_host_maxblklen(sdmmc_chipset_handle_t sch)
{

	return DBS_BLOCKSIZE_MAX;
}

#ifndef MVSDIO_CARD_DETECT
static int
mvsdio_card_detect(sdmmc_chipset_handle_t sch)
{
	struct mvsdio_softc *sc __unused = (struct mvsdio_softc *)sch;

	DPRINTF(2, ("%s: driver lacks card_detect() function.\n",
	    sc->sc_dev.dv_xname));
	return 1;	/* always detect */
}
#endif

#if 0
#ifndef MVSDIO_WRITE_PROTECT
static int
mvsdio_write_protect(sdmmc_chipset_handle_t sch)
{

	/* Nothing */

	return 0;
}
#endif
#endif

static int
mvsdio_bus_power(sdmmc_chipset_handle_t sch, uint32_t ocr)
{
	struct mvsdio_softc *sc = (struct mvsdio_softc *)sch;
	uint32_t reg;

	/* Initial state is Open Drain on CMD line. */
	mtx_enter(&sc->sc_mtx);
	reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC);
	reg &= ~HC_PUSHPULLEN;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC, reg);
	mtx_leave(&sc->sc_mtx);

	return 0;
}

static int
mvsdio_bus_clock(sdmmc_chipset_handle_t sch, int freq)
{
	struct mvsdio_softc *sc = (struct mvsdio_softc *)sch;
	uint32_t reg;
	int m;

	mtx_enter(&sc->sc_mtx);
	reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_TM);

	/* Just stop the clock. */
	if (freq == 0) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_TM,
		    reg | TM_STOPCLKEN);
		goto out;
	}

#define FREQ_TO_M(f)	(100000 / (f) - 1)

	m = FREQ_TO_M(freq);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_CDV,
	    m & CDV_CLKDVDRMVALUE_MASK);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_TM,
	    reg & ~TM_STOPCLKEN);

	reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC);
	if (freq > 25000)
		reg |= HC_HISPEEDEN;
	else
		reg &= ~HC_HISPEEDEN;	/* up to 25 MHz */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC, reg);

out:
	mtx_leave(&sc->sc_mtx);

	return 0;
}

static int
mvsdio_bus_width(sdmmc_chipset_handle_t sch, int width)
{
	struct mvsdio_softc *sc = (struct mvsdio_softc *)sch;
	uint32_t reg, v;

	switch (width) {
	case 1:
		v = 0;
		break;

	case 4:
		v = HC_DATAWIDTH;
		break;

	default:
		DPRINTF(0, ("%s: unsupported bus width (%d)\n",
		    sc->sc_dev.dv_xname, width));
		return EINVAL;
	}

	mtx_enter(&sc->sc_mtx);
	reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC);
	reg &= ~HC_DATAWIDTH;
	reg |= v;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC, reg);
	mtx_leave(&sc->sc_mtx);

	return 0;
}

static int
mvsdio_bus_rod(sdmmc_chipset_handle_t sch, int on)
{
	struct mvsdio_softc *sc = (struct mvsdio_softc *)sch;
	uint32_t reg;

	/* Change Open-drain/Push-pull. */
	mtx_enter(&sc->sc_mtx);
	reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC);
	if (on)
		reg &= ~HC_PUSHPULLEN;
	else
		reg |= HC_PUSHPULLEN;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC, reg);
	mtx_leave(&sc->sc_mtx);

	return 0;
}

static void
mvsdio_exec_command(sdmmc_chipset_handle_t sch, struct sdmmc_command *cmd)
{
	struct mvsdio_softc *sc = (struct mvsdio_softc *)sch;
	uint32_t tm, c, hc, aacc, nisie, wait;
	int blklen;

	DPRINTF(1, ("%s: start cmd %d arg=%#x data=%p dlen=%d flags=%#x\n",
	    sc->sc_dev.dv_xname, cmd->c_opcode, cmd->c_arg, cmd->c_data,
	    cmd->c_datalen, cmd->c_flags));

	mtx_enter(&sc->sc_mtx);

	tm = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_TM);

	if (cmd->c_datalen > 0) {
		bus_dma_segment_t *dm_seg =
		    &cmd->c_dmamap->dm_segs[cmd->c_dmaseg];
		bus_addr_t ds_addr = dm_seg->ds_addr + cmd->c_dmaoff;

		blklen = MIN(cmd->c_datalen, cmd->c_blklen);

		if (cmd->c_datalen % blklen > 0) {
			printf("%s: data not a multiple of %u bytes\n",
			    sc->sc_dev.dv_xname, blklen);
			cmd->c_error = EINVAL;
			goto out;
		}
		if ((uint32_t)cmd->c_data & 0x3) {
			printf("%s: data not 4byte aligned\n",
			    sc->sc_dev.dv_xname);
			cmd->c_error = EINVAL;
			goto out;
		}

		/* Set DMA Buffer Address */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_DMABA16LSB,
		    ds_addr & 0xffff);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_DMABA16MSB,
		    (ds_addr >> 16) & 0xffff);

		/* Set Data Block Size and Count */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_DBS,
		    DBS_BLOCKSIZE(blklen));
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_DBC,
		    DBC_BLOCKCOUNT(cmd->c_datalen / blklen));

		tm &= ~TM_HOSTXFERMODE;			/* Always DMA */
		if (cmd->c_flags & SCF_CMD_READ)
			tm |= TM_DATAXFERTOWARDHOST;
		else
			tm &= ~TM_DATAXFERTOWARDHOST;
		tm |= TM_HWWRDATAEN;
		wait = NIS_XFERCOMPLETE;
	} else {
		tm &= ~TM_HWWRDATAEN;
		wait = NIS_CMDCOMPLETE;
	}

	/* Set Argument in Command */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_AC16LSB,
	    cmd->c_arg & 0xffff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_AC16MSB,
	    (cmd->c_arg >> 16) & 0xffff);

	/* Set Host Control, exclude PushPullEn, DataWidth, HiSpeedEn. */
	hc = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC);
	hc |= (HC_TIMEOUTVALUE_MAX | HC_TIMEOUTEN);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_HC, hc);

	/* Data Block Gap Control: Resume */

	/* Clock Control: SclkMasterEn */

	if (cmd->c_opcode == MMC_READ_BLOCK_MULTIPLE ||
	    cmd->c_opcode == MMC_WRITE_BLOCK_MULTIPLE) {
		aacc = 0;
#if 1	/* XXXX: need? */
		if (cmd->c_opcode == MMC_READ_BLOCK_MULTIPLE) {
			struct sdmmc_softc *sdmmc =
			    (struct sdmmc_softc *)sc->sc_sdmmc;
			struct sdmmc_function *sf = sdmmc->sc_card;

			aacc = MMC_ARG_RCA(sf->rca);
		}
#endif

		/* Set Argument in Auto Cmd12 Command */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_AACC16LSBT,
		    aacc & 0xffff);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_AACC16MSBT,
		    (aacc >> 16) & 0xffff);

		bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_IACCT,
		    IACCT_AUTOCMD12BUSYCHKEN	|
		    IACCT_AUTOCMD12INDEXCHKEN	|
		    IACCT_AUTOCMD12INDEX);

		tm |= TM_AUTOCMD12EN;
		wait = NIS_AUTOCMD12COMPLETE;
	} else
		tm &= ~TM_AUTOCMD12EN;

	tm |= TM_INTCHKEN;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_TM, tm);

	c = C_CMDINDEX(cmd->c_opcode);
	if (cmd->c_flags & SCF_RSP_PRESENT) {
		if (cmd->c_flags & SCF_RSP_136)
			c |= C_RESPTYPE_136BR;
		else if (!(cmd->c_flags & SCF_RSP_BSY))
			c |= C_RESPTYPE_48BR;
		else
			c |= C_RESPTYPE_48BRCB;
		c |= C_UNEXPECTEDRESPEN;
	} else
		c |= C_RESPTYPE_NR;
	if (cmd->c_flags & SCF_RSP_CRC)
		c |= C_CMDCRCCHKEN;
	if (cmd->c_flags & SCF_RSP_IDX)
		c |= C_CMDINDEXCHKEN;
	if (cmd->c_datalen > 0)
		c |= (C_DATAPRESENT | C_DATACRC16CHKEN);

	DPRINTF(2, ("%s: TM=0x%x, C=0x%x, HC=0x%x\n", __func__, tm, c, hc));

	nisie = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NISIE);
	nisie &= ~(NIS_CMDCOMPLETE | NIS_XFERCOMPLETE | NIS_AUTOCMD12COMPLETE);
	nisie |= wait;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NISIE, nisie);

	/* Execute command */
	sc->sc_exec_cmd = cmd;
	sc->sc_waitintr = wait;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_C, c);

	/* Wait interrupt for complete or error or timeout */
	while (sc->sc_exec_cmd == cmd)
		msleep(sc, &sc->sc_mtx, PWAIT, "hcintr", 100000);

out:
	mtx_leave(&sc->sc_mtx);

	DPRINTF(1, ("%s: cmd %d done (flags=%08x error=%d)\n",
	    sc->sc_dev.dv_xname,
	    cmd->c_opcode, cmd->c_flags, cmd->c_error));
}

#if 0
static void
mvsdio_card_enable_intr(sdmmc_chipset_handle_t sch, int enable)
{
	struct mvsdio_softc *sc = (struct mvsdio_softc *)sch;
	uint32_t reg;

	mtx_enter(&sc->sc_mtx);
	reg = bus_space_read_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NISIE);
	reg |= NIS_CARDINT;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, MVSDIO_NISIE, reg);
	mtx_leave(&sc->sc_mtx);
}
#endif

static void
mvsdio_card_intr_ack(sdmmc_chipset_handle_t sch)
{

	/* Nothing */
}


static void
mvsdio_wininit(struct mvsdio_softc *sc)
{
	if (mvmbus_dram_info == NULL)
		panic("%s: mbus dram information not set up",
		    sc->sc_dev.dv_xname);

	for (int i = 0; i < MVSDIO_NWINDOW; i++) {
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    MVSDIO_WC(i), 0);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    MVSDIO_WB(i), 0);
	}

	for (int i = 0; i < mvmbus_dram_info->numcs; i++) {
		struct mbus_dram_window *win = &mvmbus_dram_info->cs[i];

		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    MVSDIO_WC(i),
		    WC_WINEN |
		    WC_TARGET(mvmbus_dram_info->targetid) |
		    WC_ATTR(win->attr) |
		    WC_SIZE(win->size));
		bus_space_write_4(sc->sc_iot, sc->sc_ioh,
		    MVSDIO_WB(i), win->base);
	}
}
