/*	$OpenBSD: sdmmc_mem.c,v 1.20 2014/09/23 12:08:13 rapha Exp $	*/

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

/* Routines for SD/MMC memory cards. */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/systm.h>

#include <dev/sdmmc/sdmmcchip.h>
#include <dev/sdmmc/sdmmcreg.h>
#include <dev/sdmmc/sdmmcvar.h>

int	sdmmc_decode_csd(struct sdmmc_softc *, sdmmc_response,
	    struct sdmmc_function *);
int	sdmmc_decode_cid(struct sdmmc_softc *, sdmmc_response,
	    struct sdmmc_function *);
void	sdmmc_print_cid(struct sdmmc_cid *);

int	sdmmc_mem_send_op_cond(struct sdmmc_softc *, u_int32_t, u_int32_t *);
int	sdmmc_mem_set_blocklen(struct sdmmc_softc *, struct sdmmc_function *);

int	sdmmc_mem_send_cxd_data(struct sdmmc_softc *, int, void *, size_t);
int	sdmmc_mem_mmc_switch(struct sdmmc_function *, uint8_t, uint8_t, uint8_t);

int	sdmmc_mem_sd_init(struct sdmmc_softc *, struct sdmmc_function *);
int	sdmmc_mem_mmc_init(struct sdmmc_softc *, struct sdmmc_function *);
int	sdmmc_mem_single_read_block(struct sdmmc_function *, int, u_char *,
	size_t);
int	sdmmc_mem_read_block_subr(struct sdmmc_function *, bus_dmamap_t, int,
	u_char *, size_t);
int	sdmmc_mem_single_segment_dma_read_block(struct sdmmc_function *,
	uint32_t, u_char *, size_t);
int	sdmmc_mem_single_write_block(struct sdmmc_function *, int, u_char *,
	size_t);
int	sdmmc_mem_write_block_subr(struct sdmmc_function *, bus_dmamap_t, int,
	u_char *, size_t);
int	sdmmc_mem_single_segment_dma_write_block(struct sdmmc_function *,
	uint32_t, u_char *, size_t);

#ifdef SDMMC_DEBUG
#define DPRINTF(s)	printf s
#else
#define DPRINTF(s)	/**/
#endif

/*
 * Initialize SD/MMC memory cards and memory in SDIO "combo" cards.
 */
int
sdmmc_mem_enable(struct sdmmc_softc *sc)
{
	u_int32_t host_ocr;
	u_int32_t card_ocr;

	rw_assert_wrlock(&sc->sc_lock);

	/* Set host mode to SD "combo" card or SD memory-only. */
	SET(sc->sc_flags, SMF_SD_MODE|SMF_MEM_MODE);

	/* Reset memory (*must* do that before CMD55 or CMD1). */
	sdmmc_go_idle_state(sc);

	/*
	 * Read the SD/MMC memory OCR value by issuing CMD55 followed
	 * by ACMD41 to read the OCR value from memory-only SD cards.
	 * MMC cards will not respond to CMD55 or ACMD41 and this is
	 * how we distinguish them from SD cards.
	 */
 mmc_mode:
	if (sdmmc_mem_send_op_cond(sc, 0, &card_ocr) != 0) {
		if (ISSET(sc->sc_flags, SMF_SD_MODE) &&
		    !ISSET(sc->sc_flags, SMF_IO_MODE)) {
			/* Not a SD card, switch to MMC mode. */
			CLR(sc->sc_flags, SMF_SD_MODE);
			goto mmc_mode;
		}
		if (!ISSET(sc->sc_flags, SMF_SD_MODE)) {
			DPRINTF(("%s: can't read memory OCR\n",
			    DEVNAME(sc)));
			return 1;
		} else {
			/* Not a "combo" card. */
			CLR(sc->sc_flags, SMF_MEM_MODE);
			return 0;
		}
	}

	/* Set the lowest voltage supported by the card and host. */
	host_ocr = sdmmc_chip_host_ocr(sc->sct, sc->sch);
	if (sdmmc_set_bus_power(sc, host_ocr, card_ocr) != 0) {
		DPRINTF(("%s: can't supply voltage requested by card\n",
		    DEVNAME(sc)));
		return 1;
	}

	/* Tell the card(s) to enter the idle state (again). */
	sdmmc_go_idle_state(sc);

	host_ocr &= card_ocr; /* only allow the common voltages */

	if (sdmmc_send_if_cond(sc, card_ocr) == 0)
		host_ocr |= SD_OCR_SDHC_CAP;

	/* Send the new OCR value until all cards are ready. */
	if (sdmmc_mem_send_op_cond(sc, host_ocr, NULL) != 0) {
		DPRINTF(("%s: can't send memory OCR\n", DEVNAME(sc)));
		return 1;
	}
	return 0;
}

/*
 * Read the CSD and CID from all cards and assign each card a unique
 * relative card address (RCA).  CMD2 is ignored by SDIO-only cards.
 */
void
sdmmc_mem_scan(struct sdmmc_softc *sc)
{
	struct sdmmc_command cmd;
	struct sdmmc_function *sf;
	u_int16_t next_rca;
	int error;
	int i;

	rw_assert_wrlock(&sc->sc_lock);

	/*
	 * CMD2 is a broadcast command understood by SD cards and MMC
	 * cards.  All cards begin to respond to the command, but back
	 * off if another card drives the CMD line to a different level.
	 * Only one card will get its entire response through.  That
	 * card remains silent once it has been assigned a RCA.
	 */
	for (i = 0; i < 100; i++) {
		bzero(&cmd, sizeof cmd);
		cmd.c_opcode = MMC_ALL_SEND_CID;
		cmd.c_flags = SCF_CMD_BCR | SCF_RSP_R2;

		error = sdmmc_mmc_command(sc, &cmd);
		if (error == ETIMEDOUT) {
			/* No more cards there. */
			break;
		} else if (error != 0) {
			DPRINTF(("%s: can't read CID\n", DEVNAME(sc)));
			break;
		}

		/* In MMC mode, find the next available RCA. */
		next_rca = 1;
		if (!ISSET(sc->sc_flags, SMF_SD_MODE))
			SIMPLEQ_FOREACH(sf, &sc->sf_head, sf_list)
				next_rca++;

		/* Allocate a sdmmc_function structure. */
		sf = sdmmc_function_alloc(sc);
		sf->rca = next_rca;

		/*
		 * Remember the CID returned in the CMD2 response for
		 * later decoding.
		 */
		bcopy(cmd.c_resp, sf->raw_cid, sizeof sf->raw_cid);

		/*
		 * Silence the card by assigning it a unique RCA, or
		 * querying it for its RCA in the case of SD.
		 */
		if (sdmmc_set_relative_addr(sc, sf) != 0) {
			printf("%s: can't set mem RCA\n", DEVNAME(sc));
			sdmmc_function_free(sf);
			break;
		}

#if 0
		/* Verify that the RCA has been set by selecting the card. */
		if (sdmmc_select_card(sc, sf) != 0) {
			printf("%s: can't select mem RCA %d\n",
			    DEVNAME(sc), sf->rca);
			sdmmc_function_free(sf);
			break;
		}

		/* Deselect. */
		(void)sdmmc_select_card(sc, NULL);
#endif

		/*
		 * If this is a memory-only card, the card responding
		 * first becomes an alias for SDIO function 0.
		 */
		if (sc->sc_fn0 == NULL)
			sc->sc_fn0 = sf;

		SIMPLEQ_INSERT_TAIL(&sc->sf_head, sf, sf_list);
	}

	/* Go to Data Transfer Mode, if possible. */
	sdmmc_chip_bus_rod(sc->sct, sc->sch, 0);

	/*
	 * All cards are either inactive or awaiting further commands.
	 * Read the CSDs and decode the raw CID for each card.
	 */
	SIMPLEQ_FOREACH(sf, &sc->sf_head, sf_list) {
		bzero(&cmd, sizeof cmd);
		cmd.c_opcode = MMC_SEND_CSD;
		cmd.c_arg = MMC_ARG_RCA(sf->rca);
		cmd.c_flags = SCF_CMD_AC | SCF_RSP_R2;

		if (sdmmc_mmc_command(sc, &cmd) != 0) {
			SET(sf->flags, SFF_ERROR);
			continue;
		}

		if (sdmmc_decode_csd(sc, cmd.c_resp, sf) != 0 ||
		    sdmmc_decode_cid(sc, sf->raw_cid, sf) != 0) {
			SET(sf->flags, SFF_ERROR);
			continue;
		}

#ifdef SDMMC_DEBUG
		printf("%s: CID: ", DEVNAME(sc));
		sdmmc_print_cid(&sf->cid);
#endif
	}
}

int
sdmmc_decode_csd(struct sdmmc_softc *sc, sdmmc_response resp,
    struct sdmmc_function *sf)
{
	/* TRAN_SPEED(2:0): transfer rate exponent */
	static const int speed_exponent[8] = {
		100 *    1,	/* 100 Kbits/s */
		  1 * 1000,	/*   1 Mbits/s */
		 10 * 1000,	/*  10 Mbits/s */
		100 * 1000,	/* 100 Mbits/s */
			 0,
			 0,
			 0,
			 0,
	};
	/* TRAN_SPEED(6:3): time mantissa */
	static const int speed_mantissa[16] = {
		0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80,
	};
	struct sdmmc_csd *csd = &sf->csd;
	int e, m;

	if (ISSET(sc->sc_flags, SMF_SD_MODE)) {
		/*
		 * CSD version 1.0 corresponds to SD system
		 * specification version 1.0 - 1.10. (SanDisk, 3.5.3)
		 */
		csd->csdver = SD_CSD_CSDVER(resp);
		switch (csd->csdver) {
		case SD_CSD_CSDVER_2_0:
			sf->flags |= SFF_SDHC;
			csd->capacity = SD_CSD_V2_CAPACITY(resp);
			csd->read_bl_len = SD_CSD_V2_BL_LEN;
			break;
		case SD_CSD_CSDVER_1_0:
			csd->capacity = SD_CSD_CAPACITY(resp);
			csd->read_bl_len = SD_CSD_READ_BL_LEN(resp);
			break;
		default:
			printf("%s: unknown SD CSD structure version 0x%x\n",
			    DEVNAME(sc), csd->csdver);
			return 1;
			break;
		}

		e = SD_CSD_SPEED_EXP(resp);
		m = SD_CSD_SPEED_MANT(resp);
		csd->tran_speed = speed_exponent[e] * speed_mantissa[m] / 10;
	} else {
		csd->csdver = MMC_CSD_CSDVER(resp);
		if (csd->csdver == MMC_CSD_CSDVER_1_0 ||
		    csd->csdver == MMC_CSD_CSDVER_2_0 ||
		    csd->csdver == MMC_CSD_CSDVER_EXT_CSD) {
			csd->mmcver = MMC_CSD_MMCVER(resp);
			csd->capacity = MMC_CSD_CAPACITY(resp);
			csd->read_bl_len = MMC_CSD_READ_BL_LEN(resp);
		} else {
			printf("%s: unknown MMC CSD structure version 0x%x\n",
			    DEVNAME(sc), csd->csdver);
			return 1;
		}

		e = MMC_CSD_TRAN_SPEED_EXP(resp);
		m = MMC_CSD_TRAN_SPEED_MANT(resp);
		csd->tran_speed = speed_exponent[e] * speed_mantissa[m] / 10;
	}
	csd->sector_size = MIN(1 << csd->read_bl_len,
	    sdmmc_chip_host_maxblklen(sc->sct, sc->sch));
	if (csd->sector_size < (1<<csd->read_bl_len))
		csd->capacity *= (1<<csd->read_bl_len) /
		    csd->sector_size;

	return 0;
}

int
sdmmc_decode_cid(struct sdmmc_softc *sc, sdmmc_response resp,
    struct sdmmc_function *sf)
{
	struct sdmmc_cid *cid = &sf->cid;

	if (ISSET(sc->sc_flags, SMF_SD_MODE)) {
		cid->mid = SD_CID_MID(resp);
		cid->oid = SD_CID_OID(resp);
		SD_CID_PNM_CPY(resp, cid->pnm);
		cid->rev = SD_CID_REV(resp);
		cid->psn = SD_CID_PSN(resp);
		cid->mdt = SD_CID_MDT(resp);
	} else {
		switch(sf->csd.mmcver) {
		case MMC_CSD_MMCVER_1_0:
		case MMC_CSD_MMCVER_1_4:
			cid->mid = MMC_CID_MID_V1(resp);
			MMC_CID_PNM_V1_CPY(resp, cid->pnm);
			cid->rev = MMC_CID_REV_V1(resp);
			cid->psn = MMC_CID_PSN_V1(resp);
			cid->mdt = MMC_CID_MDT_V1(resp);
			break;
		case MMC_CSD_MMCVER_2_0:
		case MMC_CSD_MMCVER_3_1:
		case MMC_CSD_MMCVER_4_0:
			cid->mid = MMC_CID_MID_V2(resp);
			cid->oid = MMC_CID_OID_V2(resp);
			MMC_CID_PNM_V2_CPY(resp, cid->pnm);
			cid->psn = MMC_CID_PSN_V2(resp);
			break;
		default:
			printf("%s: unknown MMC version %d\n",
			    DEVNAME(sc), sf->csd.mmcver);
			return 1;
		}
	}
	return 0;
}

#ifdef SDMMC_DEBUG
void
sdmmc_print_cid(struct sdmmc_cid *cid)
{
	printf("mid=0x%02x oid=0x%04x pnm=\"%s\" rev=0x%02x psn=0x%08x"
	    " mdt=%03x\n", cid->mid, cid->oid, cid->pnm, cid->rev, cid->psn,
	    cid->mdt);
}
#endif

int
sdmmc_mem_send_cxd_data(struct sdmmc_softc *sc, int opcode, void *data,
    size_t datalen)
{
	struct sdmmc_command cmd;
	bus_dma_segment_t ds[1];
	void *ptr = NULL;
	int rseg;
	int error = 0;

	if (datalen > INT_MAX) {
		error = EINVAL;
		goto out;
	}

	if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
		error = bus_dmamem_alloc(sc->sc_dmat, datalen, PAGE_SIZE, 0, ds,
		    1, &rseg, BUS_DMA_NOWAIT);
		if (error)
			goto out;
		error = bus_dmamem_map(sc->sc_dmat, ds, 1, datalen, (caddr_t *)&ptr,
		    BUS_DMA_NOWAIT);
		if (error)
			goto dmamem_free;
		error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, ptr, datalen,
		    NULL, BUS_DMA_NOWAIT|BUS_DMA_STREAMING|BUS_DMA_READ);
		if (error)
			goto dmamem_unmap;

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
		    BUS_DMASYNC_PREREAD);
	} else {
		ptr = malloc(datalen, M_DEVBUF, M_NOWAIT | M_ZERO);
		if (ptr == NULL) {
			error = ENOMEM;
			goto out;
		}
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_data = ptr;
	cmd.c_datalen = (int)datalen;
	cmd.c_blklen = (int)datalen;
	cmd.c_opcode = opcode;
	cmd.c_arg = 0;
	cmd.c_flags = SCF_CMD_ADTC | SCF_CMD_READ;
	if (opcode == MMC_SEND_EXT_CSD)
		SET(cmd.c_flags, SCF_RSP_R1);
	else
		SET(cmd.c_flags, SCF_RSP_R2);
	if (ISSET(sc->sc_caps, SMC_CAPS_DMA))
		cmd.c_dmamap = sc->sc_dmap;

	error = sdmmc_mmc_command(sc, &cmd);
	if (error == 0) {
		if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
			    BUS_DMASYNC_POSTREAD);
		}
		memcpy(data, ptr, datalen);
	}

out:
	if (ptr != NULL) {
		if (ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
			bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);
dmamem_unmap:
			bus_dmamem_unmap(sc->sc_dmat, ptr, datalen);
dmamem_free:
			bus_dmamem_free(sc->sc_dmat, ds, rseg);
		} else {
			free(ptr, M_DEVBUF, 0);
		}
	}

	return error;
}

int
sdmmc_mem_mmc_switch(struct sdmmc_function *sf, uint8_t set, uint8_t index,
    uint8_t value)
{
	struct sdmmc_softc *sc = sf->sc;
	struct sdmmc_command cmd;

	memset(&cmd, 0, sizeof(cmd));
	cmd.c_opcode = MMC_SWITCH;
	cmd.c_arg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
	    (index << 16) | (value << 8) | set;
	cmd.c_flags = SCF_RSP_R1B | SCF_CMD_AC;

	return sdmmc_mmc_command(sc, &cmd);
}

/*
 * Initialize a SD/MMC memory card.
 */
int
sdmmc_mem_init(struct sdmmc_softc *sc, struct sdmmc_function *sf)
{
	int error = 0;

	rw_assert_wrlock(&sc->sc_lock);

	if (sdmmc_select_card(sc, sf) != 0 ||
	    sdmmc_mem_set_blocklen(sc, sf) != 0)
		error = 1;

	if (ISSET(sc->sc_flags, SMF_SD_MODE))
		error = sdmmc_mem_sd_init(sc, sf);
	else
		error = sdmmc_mem_mmc_init(sc, sf);

	return error;
}

int
sdmmc_mem_sd_init(struct sdmmc_softc *sc, struct sdmmc_function *sf)
{
	/* XXX */

	return 0;
}

int
sdmmc_mem_mmc_init(struct sdmmc_softc *sc, struct sdmmc_function *sf)
{
	int error = 0;
	u_int8_t ext_csd[512];
	int hs_timing = 0;
	int bus_clock = 0;
	int width, value;
	u_int32_t sectors = 0;

	/* change bus clock */
	bus_clock = min(sc->sc_busclk, sf->csd.tran_speed);
	error = sdmmc_chip_bus_clock(sc->sct, sc->sch, bus_clock);
	if (error != 0) {
		printf("%s: can't change bus clock\n", DEVNAME(sc));
		return error;
	}

	if (sf->csd.mmcver >= MMC_CSD_MMCVER_4_0) {
		/* read EXT_CSD */
		error = sdmmc_mem_send_cxd_data(sc,
		    MMC_SEND_EXT_CSD, ext_csd, sizeof(ext_csd));
		if (error != 0) {
			SET(sf->flags, SFF_ERROR);
			printf("%s: can't read EXT_CSD\n", DEVNAME(sc));
			return error;
		}

		if (ext_csd[EXT_CSD_CARD_TYPE] & EXT_CSD_CARD_TYPE_F_52M) {
			sf->csd.tran_speed = 52000;	/* 52MHz */
			hs_timing = 1;
		} else if (ext_csd[EXT_CSD_CARD_TYPE] & EXT_CSD_CARD_TYPE_F_26M) {
			sf->csd.tran_speed = 26000;	/* 26MHz */
		} else {
			printf("%s: unknown CARD_TYPE 0x%x\n", DEVNAME(sc),
			    ext_csd[EXT_CSD_CARD_TYPE]);
		}
		if (!ISSET(sc->sc_caps, SMC_CAPS_MMC_HIGHSPEED))
			hs_timing = 0;

		if (hs_timing) {
			/* switch to high speed timing */
			error = sdmmc_mem_mmc_switch(sf, EXT_CSD_CMD_SET_NORMAL,
			    EXT_CSD_HS_TIMING, hs_timing);
			if (error != 0) {
				printf("%s: can't change high speed\n",
				    DEVNAME(sc));
				return error;
			}
		}

		if (sc->sc_busclk > sf->csd.tran_speed)
			sc->sc_busclk = sf->csd.tran_speed;
		if (sc->sc_busclk != bus_clock) {
			error = sdmmc_chip_bus_clock(sc->sct, sc->sch,
			    sc->sc_busclk);
			if (error != 0) {
				printf("%s: can't change bus clock\n",
				    DEVNAME(sc));
				return error;
			}
		}

		if (hs_timing) {
			/* read EXT_CSD again */
			error = sdmmc_mem_send_cxd_data(sc,
			    MMC_SEND_EXT_CSD, ext_csd, sizeof(ext_csd));
			if (error != 0) {
				printf("%s: can't re-read EXT_CSD\n", DEVNAME(sc));
				return error;
			}
			if (ext_csd[EXT_CSD_HS_TIMING] != 1) {
				printf("%s, HS_TIMING set failed\n", DEVNAME(sc));
				return EINVAL;
			}
		}

		sectors = ext_csd[EXT_CSD_SEC_COUNT + 0] << 0 |
		    ext_csd[EXT_CSD_SEC_COUNT + 1] << 8  |
		    ext_csd[EXT_CSD_SEC_COUNT + 2] << 16 |
		    ext_csd[EXT_CSD_SEC_COUNT + 3] << 24;

		if (sectors > (2u * 1024 * 1024 * 1024) / 512) {
			sf->flags |= SFF_SDHC;
			sf->csd.capacity = sectors;
		}

		if (ISSET(sc->sc_caps, SMC_CAPS_8BIT_MODE)) {
			width = 8;
			value = EXT_CSD_BUS_WIDTH_8;
		} else if (ISSET(sc->sc_caps, SMC_CAPS_4BIT_MODE)) {
			width = 4;
			value = EXT_CSD_BUS_WIDTH_4;
		} else {
			width = 1;
			value = EXT_CSD_BUS_WIDTH_1;
		}

		if (width != 1) {
			error = sdmmc_mem_mmc_switch(sf, EXT_CSD_CMD_SET_NORMAL,
			    EXT_CSD_BUS_WIDTH, value);
			if (error == 0)
				error = sdmmc_chip_bus_width(sc->sct,
				    sc->sch, width);
			else {
				DPRINTF(("%s: can't change bus width"
				    " (%d bit)\n", DEVNAME(sc), width));
				return error;
			}

			/* XXXX: need bus test? (using by CMD14 & CMD19) */
		}
		sf->width = width;
	} else {
		if (sc->sc_busclk > sf->csd.tran_speed)
			sc->sc_busclk = sf->csd.tran_speed;
		if (sc->sc_busclk != bus_clock) {
			error = sdmmc_chip_bus_clock(sc->sct, sc->sch,
			    sc->sc_busclk);
			if (error != 0) {
				printf("%s: can't change bus clock\n",
				    DEVNAME(sc));
				return error;
			}
		}
	}

	return error;
}

/*
 * Get or set the card's memory OCR value (SD or MMC).
 */
int
sdmmc_mem_send_op_cond(struct sdmmc_softc *sc, u_int32_t ocr,
    u_int32_t *ocrp)
{
	struct sdmmc_command cmd;
	int error;
	int i;

	rw_assert_wrlock(&sc->sc_lock);

	/*
	 * If we change the OCR value, retry the command until the OCR
	 * we receive in response has the "CARD BUSY" bit set, meaning
	 * that all cards are ready for identification.
	 */
	for (i = 0; i < 100; i++) {
		bzero(&cmd, sizeof cmd);
		cmd.c_arg = ocr;
		cmd.c_flags = SCF_CMD_BCR | SCF_RSP_R3;

		if (ISSET(sc->sc_flags, SMF_SD_MODE)) {
			cmd.c_opcode = SD_APP_OP_COND;
			error = sdmmc_app_command(sc, &cmd);
		} else {
			cmd.c_opcode = MMC_SEND_OP_COND;
			error = sdmmc_mmc_command(sc, &cmd);
		}
		if (error != 0)
			break;
		if (ISSET(MMC_R3(cmd.c_resp), MMC_OCR_MEM_READY) ||
		    ocr == 0)
			break;
		error = ETIMEDOUT;
		sdmmc_delay(10000);
	}
	if (error == 0 && ocrp != NULL)
		*ocrp = MMC_R3(cmd.c_resp);

	return error;
}

/*
 * Set the read block length appropriately for this card, according to
 * the card CSD register value.
 */
int
sdmmc_mem_set_blocklen(struct sdmmc_softc *sc, struct sdmmc_function *sf)
{
	struct sdmmc_command cmd;

	rw_assert_wrlock(&sc->sc_lock);

	bzero(&cmd, sizeof cmd);
	cmd.c_opcode = MMC_SET_BLOCKLEN;
	cmd.c_arg = sf->csd.sector_size;
	cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1;
	DPRINTF(("%s: read_bl_len=%d sector_size=%d\n", DEVNAME(sc),
	    1 << sf->csd.read_bl_len, sf->csd.sector_size));

	return sdmmc_mmc_command(sc, &cmd);
}

int
sdmmc_mem_read_block_subr(struct sdmmc_function *sf, bus_dmamap_t dmap,
    int blkno, u_char *data, size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	struct sdmmc_command cmd;
	int error;

	if (datalen > INT_MAX) {
		error = EINVAL;
		goto err;
	}

	if ((error = sdmmc_select_card(sc, sf)) != 0)
		goto err;

	bzero(&cmd, sizeof cmd);
	cmd.c_data = data;
	cmd.c_datalen = (int)datalen;
	cmd.c_blklen = sf->csd.sector_size;
	cmd.c_opcode = (datalen / cmd.c_blklen) > 1 ?
	    MMC_READ_BLOCK_MULTIPLE : MMC_READ_BLOCK_SINGLE;
	if (sf->flags & SFF_SDHC)
		cmd.c_arg = blkno;
	else
		cmd.c_arg = blkno << 9;
	cmd.c_flags = SCF_CMD_ADTC | SCF_CMD_READ | SCF_RSP_R1;
	if (ISSET(sc->sc_caps, SMC_CAPS_DMA))
		cmd.c_dmamap = dmap;

	error = sdmmc_mmc_command(sc, &cmd);
	if (error != 0)
		goto err;

	if (ISSET(sc->sc_flags, SMF_STOP_AFTER_MULTIPLE) &&
	    cmd.c_opcode == MMC_READ_BLOCK_MULTIPLE) {
		bzero(&cmd, sizeof cmd);
		cmd.c_opcode = MMC_STOP_TRANSMISSION;
		cmd.c_arg = MMC_ARG_RCA(sf->rca);
		cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1B;
		error = sdmmc_mmc_command(sc, &cmd);
		if (error != 0)
			goto err;
	}

	do {
		bzero(&cmd, sizeof cmd);
		cmd.c_opcode = MMC_SEND_STATUS;
		cmd.c_arg = MMC_ARG_RCA(sf->rca);
		cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1;
		error = sdmmc_mmc_command(sc, &cmd);
		if (error != 0)
			break;
		/* XXX time out */
	} while (!ISSET(MMC_R1(cmd.c_resp), MMC_R1_READY_FOR_DATA));

err:
	return (error);
}

int
sdmmc_mem_single_read_block(struct sdmmc_function *sf, int blkno, u_char *data,
    size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	int error;
	int i;

	KASSERT((datalen % SDMMC_SECTOR_SIZE) == 0);
	KASSERT(!ISSET(sc->sc_caps, SMC_CAPS_DMA));

	for (i = 0; i < datalen / sf->csd.sector_size; i++) {
		error = sdmmc_mem_read_block_subr(sf, sc->sc_dmap, blkno + i,
		    data + i * sf->csd.sector_size, sf->csd.sector_size);
		if (error)
			break;
	}

	return (error);
}

int
sdmmc_mem_read_block(struct sdmmc_function *sf, int blkno, u_char *data,
    size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	int error;

	rw_enter_write(&sc->sc_lock);

	if (ISSET(sc->sc_caps, SMC_CAPS_SINGLE_ONLY)) {
		error = sdmmc_mem_single_read_block(sf, blkno, data, datalen);
		goto out;
	}

	if (!ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
		error = sdmmc_mem_read_block_subr(sf, sc->sc_dmap, blkno, data,
		    datalen);
		goto out;
	}

	/* DMA transfer */
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, data, datalen, NULL,
	    BUS_DMA_NOWAIT|BUS_DMA_READ);
	if (error)
		goto out;

#ifdef SDMMC_DEBUG
	printf("%s: data=%p, datalen=%zu\n", __func__, data, datalen);
	for (int i = 0; i < sc->sc_dmap->dm_nsegs; i++) {
		printf("%s: seg#%d: addr=%#lx, size=%#lx\n", __func__, i,
		    (u_long)sc->sc_dmap->dm_segs[i].ds_addr,
		    (u_long)sc->sc_dmap->dm_segs[i].ds_len);
	}
#endif

	if (sc->sc_dmap->dm_nsegs > 1
	    && !ISSET(sc->sc_caps, SMC_CAPS_MULTI_SEG_DMA)) {
		error = sdmmc_mem_single_segment_dma_read_block(sf, blkno,
		    data, datalen);
		goto unload;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
	    BUS_DMASYNC_PREREAD);

	error = sdmmc_mem_read_block_subr(sf, sc->sc_dmap, blkno, data,
	    datalen);
	if (error)
		goto unload;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
	    BUS_DMASYNC_POSTREAD);
unload:
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);

out:
	rw_exit(&sc->sc_lock);
	return (error);
}

/*
 * Simulate multi-segment dma transfer.
 */
int
sdmmc_mem_single_segment_dma_read_block(struct sdmmc_function *sf,
    uint32_t blkno, u_char *data, size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	bool use_bbuf = false;
	int error = 0;
	int i;

	for (i = 0; i < sc->sc_dmap->dm_nsegs; i++) {
		size_t len = sc->sc_dmap->dm_segs[i].ds_len;
		if ((len % SDMMC_SECTOR_SIZE) != 0) {
			use_bbuf = true;
			break;
		}
	}
	if (use_bbuf) {
		bus_dmamap_sync(sc->sc_dmat, sf->bbuf_dmap, 0, datalen,
		    BUS_DMASYNC_PREREAD);

		error = sdmmc_mem_read_block_subr(sf, sf->bbuf_dmap,
		    blkno, data, datalen);
		if (error) {
			bus_dmamap_unload(sc->sc_dmat, sf->bbuf_dmap);
			return error;
		}

		bus_dmamap_sync(sc->sc_dmat, sf->bbuf_dmap, 0, datalen,
		    BUS_DMASYNC_POSTREAD);

		/* Copy from bounce buffer */
		memcpy(data, sf->bbuf, datalen);

		return 0;
	}

	for (i = 0; i < sc->sc_dmap->dm_nsegs; i++) {
		size_t len = sc->sc_dmap->dm_segs[i].ds_len;

		error = bus_dmamap_load(sc->sc_dmat, sf->sseg_dmap,
		    data, len, NULL, BUS_DMA_NOWAIT|BUS_DMA_READ);
		if (error)
		bus_dmamap_sync(sc->sc_dmat, sf->sseg_dmap, 0, len,
		    BUS_DMASYNC_PREREAD);

		error = sdmmc_mem_read_block_subr(sf, sf->sseg_dmap,
		    blkno, data, len);
		if (error) {
			bus_dmamap_unload(sc->sc_dmat, sf->sseg_dmap);
			return error;
		}

		bus_dmamap_sync(sc->sc_dmat, sf->sseg_dmap, 0, len,
		    BUS_DMASYNC_POSTREAD);

		bus_dmamap_unload(sc->sc_dmat, sf->sseg_dmap);

		blkno += len / SDMMC_SECTOR_SIZE;
		data += len;
	}
	return 0;
}

int
sdmmc_mem_write_block_subr(struct sdmmc_function *sf, bus_dmamap_t dmap,
    int blkno, u_char *data, size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	struct sdmmc_command cmd;
	int error;

	if (datalen > INT_MAX) {
		error = EINVAL;
		goto err;
	}

	if ((error = sdmmc_select_card(sc, sf)) != 0)
		goto err;

	bzero(&cmd, sizeof cmd);
	cmd.c_data = data;
	cmd.c_datalen = (int)datalen;
	cmd.c_blklen = sf->csd.sector_size;
	cmd.c_opcode = (datalen / cmd.c_blklen) > 1 ?
	    MMC_WRITE_BLOCK_MULTIPLE : MMC_WRITE_BLOCK_SINGLE;
	if (sf->flags & SFF_SDHC)
		cmd.c_arg = blkno;
	else
		cmd.c_arg = blkno << 9;
	cmd.c_flags = SCF_CMD_ADTC | SCF_RSP_R1;
	if (ISSET(sc->sc_caps, SMC_CAPS_DMA))
		cmd.c_dmamap = dmap;

	error = sdmmc_mmc_command(sc, &cmd);
	if (error != 0)
		goto err;

	if (ISSET(sc->sc_flags, SMF_STOP_AFTER_MULTIPLE) &&
	    cmd.c_opcode == MMC_WRITE_BLOCK_MULTIPLE) {
		bzero(&cmd, sizeof cmd);
		cmd.c_opcode = MMC_STOP_TRANSMISSION;
		cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1B;
		error = sdmmc_mmc_command(sc, &cmd);
		if (error != 0)
			goto err;
	}

	do {
		bzero(&cmd, sizeof cmd);
		cmd.c_opcode = MMC_SEND_STATUS;
		cmd.c_arg = MMC_ARG_RCA(sf->rca);
		cmd.c_flags = SCF_CMD_AC | SCF_RSP_R1;
		error = sdmmc_mmc_command(sc, &cmd);
		if (error != 0)
			break;
		/* XXX time out */
	} while (!ISSET(MMC_R1(cmd.c_resp), MMC_R1_READY_FOR_DATA));

err:
	return (error);
}

int
sdmmc_mem_single_write_block(struct sdmmc_function *sf, int blkno, u_char *data,
    size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	int error;
	int i;

	KASSERT((datalen % SDMMC_SECTOR_SIZE) == 0);
	KASSERT(!ISSET(sc->sc_caps, SMC_CAPS_DMA));

	for (i = 0; i < datalen / sf->csd.sector_size; i++) {
		error = sdmmc_mem_write_block_subr(sf, sc->sc_dmap, blkno + i,
		    data + i * sf->csd.sector_size, sf->csd.sector_size);
		if (error)
			break;
	}

	return (error);
}

int
sdmmc_mem_write_block(struct sdmmc_function *sf, int blkno, u_char *data,
    size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	int error;

	rw_enter_write(&sc->sc_lock);

	if (ISSET(sc->sc_caps, SMC_CAPS_SINGLE_ONLY)) {
		error = sdmmc_mem_single_write_block(sf, blkno, data, datalen);
		goto out;
	}

	if (!ISSET(sc->sc_caps, SMC_CAPS_DMA)) {
		error = sdmmc_mem_write_block_subr(sf, sc->sc_dmap, blkno, data,
		    datalen);
		goto out;
	}

	/* DMA transfer */
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmap, data, datalen, NULL,
	    BUS_DMA_NOWAIT|BUS_DMA_WRITE);
	if (error)
		goto out;

#ifdef SDMMC_DEBUG
	printf("%s: data=%p, datalen=%zu\n", __func__, data, datalen);
	for (int i = 0; i < sc->sc_dmap->dm_nsegs; i++) {
		printf("%s: seg#%d: addr=%#lx, size=%#lx\n", __func__, i,
		    (u_long)sc->sc_dmap->dm_segs[i].ds_addr,
		    (u_long)sc->sc_dmap->dm_segs[i].ds_len);
	}
#endif

	if (sc->sc_dmap->dm_nsegs > 1
	    && !ISSET(sc->sc_caps, SMC_CAPS_MULTI_SEG_DMA)) {
		error = sdmmc_mem_single_segment_dma_write_block(sf, blkno,
		    data, datalen);
		goto unload;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
	    BUS_DMASYNC_PREWRITE);

	error = sdmmc_mem_write_block_subr(sf, sc->sc_dmap, blkno, data,
	    datalen);
	if (error)
		goto unload;

	bus_dmamap_sync(sc->sc_dmat, sc->sc_dmap, 0, datalen,
	    BUS_DMASYNC_POSTWRITE);
unload:
	bus_dmamap_unload(sc->sc_dmat, sc->sc_dmap);

out:
	rw_exit(&sc->sc_lock);
	return (error);
}

/*
 * Simulate multi-segment dma transfer.
 */
int
sdmmc_mem_single_segment_dma_write_block(struct sdmmc_function *sf,
    uint32_t blkno, u_char *data, size_t datalen)
{
	struct sdmmc_softc *sc = sf->sc;
	bool use_bbuf = false;
	int error = 0;
	int i;

	for (i = 0; i < sc->sc_dmap->dm_nsegs; i++) {
		size_t len = sc->sc_dmap->dm_segs[i].ds_len;
		if ((len % SDMMC_SECTOR_SIZE) != 0) {
			use_bbuf = true;
			break;
		}
	}
	if (use_bbuf) {
		/* Copy to bounce buffer */
		memcpy(sf->bbuf, data, datalen);

		bus_dmamap_sync(sc->sc_dmat, sf->bbuf_dmap, 0, datalen,
		    BUS_DMASYNC_PREWRITE);

		error = sdmmc_mem_write_block_subr(sf, sf->bbuf_dmap,
		    blkno, data, datalen);
		if (error) {
			bus_dmamap_unload(sc->sc_dmat, sf->bbuf_dmap);
			return error;
		}

		bus_dmamap_sync(sc->sc_dmat, sf->bbuf_dmap, 0, datalen,
		    BUS_DMASYNC_POSTWRITE);

		return 0;
	}

	for (i = 0; i < sc->sc_dmap->dm_nsegs; i++) {
		size_t len = sc->sc_dmap->dm_segs[i].ds_len;

		error = bus_dmamap_load(sc->sc_dmat, sf->sseg_dmap,
		    data, len, NULL, BUS_DMA_NOWAIT|BUS_DMA_WRITE);
		if (error)
			return error;

		bus_dmamap_sync(sc->sc_dmat, sf->sseg_dmap, 0, len,
		    BUS_DMASYNC_PREWRITE);

		error = sdmmc_mem_write_block_subr(sf, sf->sseg_dmap,
		    blkno, data, len);
		if (error) {
			bus_dmamap_unload(sc->sc_dmat, sf->sseg_dmap);
			return error;
		}

		bus_dmamap_sync(sc->sc_dmat, sf->sseg_dmap, 0, len,
		    BUS_DMASYNC_POSTWRITE);

		bus_dmamap_unload(sc->sc_dmat, sf->sseg_dmap);

		blkno += len / SDMMC_SECTOR_SIZE;
		data += len;
	}

	return error;
}
