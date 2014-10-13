/* $OpenBSD: imxccm.c,v 1.3 2013/11/26 20:33:11 deraadt Exp $ */
/*
 * Copyright (c) 2012-2014 Patrick Wildt <patrick@blueri.se>
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
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/sysctl.h>
#include <sys/device.h>
#include <sys/evcount.h>
#include <sys/socket.h>
#include <sys/timeout.h>
#include <sys/sensors.h>
#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/clock.h>
#include <armv7/armv7/armv7var.h>
#include <armv7/imx/imxccmvar.h>

/* registers */
#define CCM_CCR		0x00
#define CCM_CCDR	0x04
#define CCM_CSR		0x08
#define CCM_CCSR	0x0c
#define CCM_CACRR	0x10
#define CCM_CBCDR	0x14
#define CCM_CBCMR	0x18
#define CCM_CSCMR1	0x1c
#define CCM_CSCMR2	0x20
#define CCM_CSCDR1	0x24
#define CCM_CS1CDR	0x28
#define CCM_CS2CDR	0x2c
#define CCM_CDCDR	0x30
#define CCM_CHSCCDR	0x34
#define CCM_CSCDR2	0x38
#define CCM_CSCDR3	0x3c
#define CCM_CSCDR4	0x40
#define CCM_CDHIPR	0x48
#define CCM_CDCR	0x4c
#define CCM_CTOR	0x50
#define CCM_CLPCR	0x54
#define CCM_CISR	0x58
#define CCM_CIMR	0x5c
#define CCM_CCOSR	0x60
#define CCM_CGPR	0x64
#define CCM_CCGR0	0x68
#define CCM_CCGR1	0x6c
#define CCM_CCGR2	0x70
#define CCM_CCGR3	0x74
#define CCM_CCGR4	0x78
#define CCM_CCGR5	0x7c
#define CCM_CCGR6	0x80
#define CCM_CCGR7	0x84
#define CCM_CMEOR	0x88

/* ANALOG */
#define CCM_ANALOG_SET(x)			((x) + 4)
#define CCM_ANALOG_CLR(x)			((x) + 8)
#define CCM_ANALOG_PLL_ARM			0x4000
#define CCM_ANALOG_PLL_ARM_SET			0x4004
#define CCM_ANALOG_PLL_ARM_CLR			0x4008
#define CCM_ANALOG_PLL_USB1			0x4010
#define CCM_ANALOG_PLL_USB1_SET			0x4014
#define CCM_ANALOG_PLL_USB1_CLR			0x4018
#define CCM_ANALOG_PLL_USB2			0x4020
#define CCM_ANALOG_PLL_USB2_SET			0x4024
#define CCM_ANALOG_PLL_USB2_CLR			0x4028
#define CCM_ANALOG_PLL_SYS			0x4030
#define CCM_ANALOG_PLL_AUDIO			0x4070
#define CCM_ANALOG_PLL_VIDEO			0x40a0
#define CCM_ANALOG_PLL_ENET			0x40e0
#define CCM_ANALOG_PLL_ENET_SET			0x40e4
#define CCM_ANALOG_PLL_ENET_CLR			0x40e8
#define CCM_ANALOG_PFD_480			0x40f0
#define CCM_ANALOG_PFD_480_SET			0x40f4
#define CCM_ANALOG_PFD_480_CLR			0x40f8
#define CCM_ANALOG_PFD_528			0x4100
#define CCM_ANALOG_PFD_528_SET			0x4104
#define CCM_ANALOG_PFD_528_CLR			0x4108
#define CCM_PMU_MISC1				0x4160
#define CCM_ANALOG_USB1_CHRG_DETECT		0x41b0
#define CCM_ANALOG_USB1_CHRG_DETECT_SET		0x41b4
#define CCM_ANALOG_USB1_CHRG_DETECT_CLR		0x41b8
#define CCM_ANALOG_USB2_CHRG_DETECT		0x4210
#define CCM_ANALOG_USB2_CHRG_DETECT_SET		0x4214
#define CCM_ANALOG_USB2_CHRG_DETECT_CLR		0x4218
#define CCM_ANALOG_DIGPROG			0x4260
#define CCM_ANALOG_DIGPROG_MX6SL		0x4280

/* bits and bytes */
#define CCM_ANALOG_PLL_NUM			0x10
#define CCM_ANALOG_PLL_DENOM			0x20
#define CCM_ANALOG_PLL_POWER			(1 << 12)
#define CCM_ANALOG_PLL_ENABLE			(1 << 13)
#define CCM_ANALOG_PLL_BYPASS			(1 << 16)
#define CCM_ANALOG_PLL_LOCK			(1U << 31)
#define CCM_ANALOG_USB1_CHRG_DETECT_CHK_CHRG_B	(1 << 19)
#define CCM_ANALOG_USB1_CHRG_DETECT_EN_B	(1 << 20)
#define CCM_ANALOG_USB2_CHRG_DETECT_CHK_CHRG_B	(1 << 19)
#define CCM_ANALOG_USB2_CHRG_DETECT_EN_B	(1 << 20)
#define CCM_ANALOG_DIGPROG_MINOR_MASK		0xff

#define CCM_SENSORS				13

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

struct imxccm_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	struct ksensor sc_freq_sensor[CCM_SENSORS];
	struct ksensordev sc_sensordev;

	int			cpu_type;
};

enum clocks {
	/* OSC */
	OSC,		/* 24 MHz OSC */

	/* PLLs */
	ARM_PLL1,	/* ARM core PLL */
	SYS_PLL2,	/* System PLL: 528 MHz */
	USB1_PLL3,	/* OTG USB PLL: 480 MHz */
	USB2_PLL,	/* Host USB PLL: 480 MHz */
	AUD_PLL4,	/* Audio PLL */
	VID_PLL5,	/* Video PLL */
	ENET_PLL6,	/* ENET PLL */
	MLB_PLL,	/* MLB PLL */

	/* SYS_PLL2 PFDs */
	SYS_PLL2_PFD0,	/* 352 MHz */
	SYS_PLL2_PFD1,	/* 594 MHz */
	SYS_PLL2_PFD2,	/* 396 MHz */
};

static const char *axi_sels[]		= { "periph", "pll2_pfd2_396m", "periph", "pll3_pfd1_540m", };
static const char *lvds_sels[]		= { "dummy", "dummy", "dummy", "dummy", "dummy", "dummy",
					    "pll4_audio", "pll5_video", "pll8_mlb", "enet_ref",
					    "pcie_ref", "sata_ref", };
static const char *periph_pre_sels[]	= { "pll2_bus", "pll2_pfd2_396m", "pll2_pfd0_352m", "pll2_198m", };
static const char *periph_clk2_sels[]	= { "pll3_usb_otg", "osc", "osc", "dummy", };
static const char *periph2_clk2_sels[]	= { "pll3_usb_otg", "pll2_bus", };
static const char *periph_sels[]	= { "periph_pre", "periph_clk2", };
static const char *periph2_sels[]	= { "periph2_pre", "periph2_clk2", };
static const char *pcie_axi_sels[]	= { "axi", "ahb" };
static const char *pll1_sw_sels[]	= { "pll1_sys", "step", };
static const char *step_sels[]		= { "osc", "pll2_pfd2_396m", };
static const char *usdhc_sels[]		= { "pll2_pfd2_396m", "pll2_pfd0_352m", };

struct clk_div_table {
	uint32_t val;
	uint32_t div;
};

static struct clk_div_table clk_enet_ref_table[] = {
	{ .val = 0, .div = 20 },
	{ .val = 1, .div = 10 },
	{ .val = 2, .div = 5 },
	{ .val = 3, .div = 4 },
	{ },
};

struct imxccm_softc *imxccm_sc;

void imxccm_attach(struct device *parent, struct device *self, void *args);
int imxccm_cpuspeed(int *);
void imxccm_register_clocks(struct imxccm_softc *);
void imxccm_armclk_set_freq(unsigned int);
void imxccm_disable_usb1_chrg_detect(void);
void imxccm_disable_usb2_chrg_detect(void);
void imxccm_refresh_sensors(void *);

struct clk *imxccm_pll(enum clocks, char *, char *, uint32_t, uint32_t);
uint32_t imxccm_pll_get_rate(struct clk *);
int imxccm_pll_enable(struct clk *);
struct clk *imxccm_pfd(char *, char *, uint32_t, uint32_t);
uint32_t imxccm_pfd_get_rate(struct clk *);
struct clk *imxccm_div(char *, char *, uint32_t, uint32_t, uint32_t);
struct clk *imxccm_div_table(char *, char *, uint32_t, uint32_t, uint32_t, struct clk_div_table *);
uint32_t imxccm_div_get_rate(struct clk *);
uint32_t imxccm_div_table_get_rate(struct clk *);
uint32_t imxccm_fixed_clock_get_rate(struct clk *);
struct clk *imxccm_fixed_clock(char *, uint32_t);
uint32_t imxccm_fixed_factor_get_rate(struct clk *);
struct clk *imxccm_fixed_factor(char *, char *, uint32_t, uint32_t);
struct clk *imxccm_var_clock(char *, uint32_t (*) (struct clk *));
struct clk *imxccm_gate(char *, char *, uint32_t, uint32_t);
struct clk *imxccm_gate2(char *, char *, uint32_t, uint32_t);
struct clk *imxccm_mux(char *, uint32_t, uint32_t, uint32_t, const char **, int);
struct clk *imxccm_mux_get_parent(struct clk *);
int imxccm_mux_set_parent(struct clk *, struct clk *);
int imxccm_gate_enable(struct clk *);
void imxccm_gate_disable(struct clk *);

struct cfattach	imxccm_ca = {
	sizeof (struct imxccm_softc), NULL, imxccm_attach
};

struct cfdriver imxccm_cd = {
	NULL, "imxccm", DV_DULL
};

void
imxccm_attach(struct device *parent, struct device *self, void *args)
{
	struct armv7_attach_args *aa = args;
	struct imxccm_softc *sc = (struct imxccm_softc *) self;
	int cpu_type, i;
	char *board;

	imxccm_sc = sc;
	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(sc->sc_iot, aa->aa_dev->mem[0].addr,
	    aa->aa_dev->mem[0].size, 0, &sc->sc_ioh))
		panic("imxccm_attach: bus_space_map failed!");

	strlcpy(sc->sc_sensordev.xname, sc->sc_dev.dv_xname,
	    sizeof(sc->sc_sensordev.xname));
	for (i = 0; i < CCM_SENSORS; i++) {
		sc->sc_freq_sensor[i].type = SENSOR_FREQ;
		sensor_attach(&sc->sc_sensordev, &sc->sc_freq_sensor[i]);
	}
	sensordev_install(&sc->sc_sensordev);
	sensor_task_register(sc, imxccm_refresh_sensors, 60);

	/* main clocks */
	imxccm_fixed_clock("dummy", 0);
	imxccm_fixed_clock("osc", 24000);

	/* pll */
	imxccm_pll(ARM_PLL1, "pll1_sys", "osc", CCM_ANALOG_PLL_ARM, 0x7f);
	imxccm_pll(SYS_PLL2, "pll2_bus", "osc", CCM_ANALOG_PLL_SYS, 0x1);
	imxccm_pll(USB1_PLL3, "pll3_usb_otg", "osc", CCM_ANALOG_PLL_USB1, 0x1);
	imxccm_pll(AUD_PLL4, "pll4_audio", "osc", CCM_ANALOG_PLL_AUDIO, 0x7f);
	imxccm_pll(VID_PLL5, "pll5_video", "osc", CCM_ANALOG_PLL_VIDEO, 0x7f);
	imxccm_pll(ENET_PLL6, "pll6_enet", "osc", CCM_ANALOG_PLL_ENET, 0x3);
	imxccm_pll(USB2_PLL, "pll7_usb_host", "osc", CCM_ANALOG_PLL_USB2, 0x1);
	imxccm_pfd("pll2_pfd0_352m", "pll2_bus", CCM_ANALOG_PFD_528, 0);
	imxccm_pfd("pll2_pfd1_594m", "pll2_bus", CCM_ANALOG_PFD_528, 1);
	imxccm_pfd("pll2_pfd2_396m", "pll2_bus", CCM_ANALOG_PFD_528, 2);
	imxccm_pfd("pll3_pfd0_720m", "pll3_usb_otg", CCM_ANALOG_PFD_480, 0);
	imxccm_pfd("pll3_pfd1_540m", "pll3_usb_otg", CCM_ANALOG_PFD_480, 1);
	imxccm_pfd("pll3_pfd2_508m", "pll3_usb_otg", CCM_ANALOG_PFD_480, 2);
	imxccm_pfd("pll3_pfd3_454m", "pll3_usb_otg", CCM_ANALOG_PFD_480, 3);
	imxccm_fixed_factor("pll2_198m", "pll2_pfd2_396m", 1, 2);
	imxccm_fixed_factor("pll3_120m", "pll3_usb_otg", 1, 4);
	imxccm_fixed_factor("pll3_80m", "pll3_usb_otg", 1, 6);
	imxccm_fixed_factor("pll3_60m", "pll3_usb_otg", 1, 8);

	/* arm core clocks */
	imxccm_mux("step", CCM_CCSR, 8, 1, step_sels, nitems(step_sels));
	imxccm_mux("pll1_sw", CCM_CCSR, 2, 1, pll1_sw_sels, nitems(pll1_sw_sels));
	imxccm_div("arm", "pll1_sw", CCM_CACRR, 0, 3);

	/* periph */
	imxccm_mux("periph_pre", CCM_CBCMR, 18, 2, periph_pre_sels, nitems(periph_pre_sels));
	imxccm_mux("periph2_pre", CCM_CBCMR, 21, 2, periph_pre_sels, nitems(periph_pre_sels));
	imxccm_mux("periph_clk2_sel", CCM_CBCMR, 12, 2, periph_clk2_sels, nitems(periph_clk2_sels));
	imxccm_mux("periph2_clk2_sel", CCM_CBCMR, 20, 1, periph2_clk2_sels, nitems(periph2_clk2_sels));
	imxccm_div("periph_clk2", "periph_clk2_sel", CCM_CBCDR, 27, 3);
	imxccm_div("periph2_clk2", "periph2_clk2_sel", CCM_CBCDR, 0, 3);
	imxccm_mux("periph", CCM_CBCDR, 25, 1, periph_sels, nitems(periph_sels));
	imxccm_mux("periph2", CCM_CBCDR, 26, 1, periph2_sels, nitems(periph2_sels));
	imxccm_div("ahb", "periph", CCM_CBCDR, 10, 3);
	imxccm_div("ipg", "ahb", CCM_CBCDR, 8, 2);
	imxccm_div("ipg_per", "ipg", CCM_CSCMR1, 0, 6);

	/* axi */
	imxccm_mux("axi_sel", CCM_CBCDR, 6, 2, axi_sels, nitems(axi_sels));
	imxccm_div("axi", "axi_sel", CCM_CBCDR, 16, 3);

	/* uart */
	imxccm_div("uart_clk_podf", "pll3_80m", CCM_CSCDR1, 0, 6);
	imxccm_gate2("uart_ipg", "ipg", CCM_CCGR5, 24);
	imxccm_gate2("uart_serial", "uart_clk_podf", CCM_CCGR5, 26);

	/* usdhc */
	imxccm_mux("usdhc1_sel", CCM_CSCMR1, 16, 1, usdhc_sels, nitems(usdhc_sels));
	imxccm_mux("usdhc2_sel", CCM_CSCMR1, 17, 1, usdhc_sels, nitems(usdhc_sels));
	imxccm_mux("usdhc3_sel", CCM_CSCMR1, 18, 1, usdhc_sels, nitems(usdhc_sels));
	imxccm_mux("usdhc4_sel", CCM_CSCMR1, 19, 1, usdhc_sels, nitems(usdhc_sels));
	imxccm_div("usdhc1_podf", "usdhc1_sel", CCM_CSCDR1, 11, 3);
	imxccm_div("usdhc2_podf", "usdhc2_sel", CCM_CSCDR1, 16, 3);
	imxccm_div("usdhc3_podf", "usdhc3_sel", CCM_CSCDR1, 19, 3);
	imxccm_div("usdhc4_podf", "usdhc4_sel", CCM_CSCDR1, 22, 3);
	imxccm_gate2("usdhc1", "usdhc1_podf", CCM_CCGR6, 2);
	imxccm_gate2("usdhc2", "usdhc2_podf", CCM_CCGR6, 4);
	imxccm_gate2("usdhc3", "usdhc3_podf", CCM_CCGR6, 6);
	imxccm_gate2("usdhc4", "usdhc4_podf", CCM_CCGR6, 8);

	/* i2c */
	imxccm_gate2("i2c1", "ipg_per", CCM_CCGR2, 6);
	imxccm_gate2("i2c2", "ipg_per", CCM_CCGR2, 8);
	imxccm_gate2("i2c3", "ipg_per", CCM_CCGR2, 10);

	/* enet */
	imxccm_div_table("enet_ref", "pll6_enet", CCM_ANALOG_PLL_ENET, 0, 2, clk_enet_ref_table);
	imxccm_gate2("enet", "ipg", CCM_CCGR1, 10);

	/* usb */
	imxccm_gate2("usboh3", "ipg", CCM_CCGR6, 0);
	imxccm_gate("usbphy1_gate", "dummy", CCM_ANALOG_PLL_USB1, 6);
	imxccm_gate("usbphy2_gate", "dummy", CCM_ANALOG_PLL_USB2, 6);

	/* sata */
	imxccm_gate2("sata", "ipg", CCM_CCGR5, 4);
	imxccm_fixed_factor("sata_ref", "pll6_enet", 1, 5);
	imxccm_gate("sata_ref_100m", "sata_ref", CCM_ANALOG_PLL_ENET, 20);

	/* pcie */
	imxccm_mux("pcie_axi_sel", CCM_CBCMR, 10, 1, pcie_axi_sels, nitems(pcie_axi_sels));
	imxccm_gate2("pcie_axi", "pcie_axi_sel", CCM_CCGR4, 0);
	imxccm_fixed_factor("pcie_ref", "pll6_enet", 1, 4);
	imxccm_gate("pcie_ref_125m", "pcie_ref", CCM_ANALOG_PLL_ENET, 19);

	/* lvds */
	imxccm_mux("lvds1_sel", CCM_PMU_MISC1, 0, 5, lvds_sels, nitems(lvds_sels));
	imxccm_mux("lvds2_sel", CCM_PMU_MISC1, 5, 5, lvds_sels, nitems(lvds_sels));
	imxccm_gate("lvds1_gate", "dummy", CCM_PMU_MISC1, 10);
	imxccm_gate("lvds2_gate", "dummy", CCM_PMU_MISC1, 11);
	clk_set_parent(clk_get("lvds1_sel"), clk_get("sata_ref")); /* PCIe */

	cpu_type = HREAD4(sc, CCM_ANALOG_DIGPROG_MX6SL);
	if ((cpu_type >> 16) == MX6SL) {
		board = "i.MX6SoloLite";
	} else {
		cpu_type = HREAD4(sc, CCM_ANALOG_DIGPROG);
		if ((cpu_type >> 16) == MX6DL) {
			board = "i.MX6DL/SOLO";
		} else if ((cpu_type >> 16) == MX6Q) {
			board = "i.MX6Q";
		} else {
			cpu_type = 0;
			board = "unknown";
		}
	}

	sc->cpu_type = cpu_type >> 16;
	printf(": %s rev 1.%d CPU freq: %d MHz", board, cpu_type & 0xff,
	    clk_get_rate(clk_get("arm")) / 1000);

	printf("\n");

	cpu_cpuspeed = imxccm_cpuspeed;
}

int
imxccm_is(enum imx_cpu type)
{
	struct imxccm_softc *sc = imxccm_sc;
	return sc->cpu_type == type;
}

struct clktable {
	char *		name;
	char *		parent;
	char *		secondary;
	struct clk	clk;
};

int
imxccm_cpuspeed(int *freq)
{
	*freq = clk_get_rate(clk_get("arm")) / 1000;
	return (0);
}

struct clk_pll {
	enum clocks clock;
	uint32_t reg;
	uint32_t mask;
	uint32_t powerup;
};

struct clk *
imxccm_pll(enum clocks clock, char *name, char *parent, uint32_t reg, uint32_t mask)
{
	struct clk *clk, *p;
	struct clk_pll *pll;

	clk = malloc(sizeof(struct clk), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (clk == NULL)
		return NULL;

	p = clk_get(parent);
	if (p == NULL)
		goto err;

	pll = malloc(sizeof(struct clk_pll), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (pll == NULL)
		goto err;

	if (clock == USB1_PLL3 || clock == USB2_PLL)
		pll->powerup = 1;

	pll->clock = clock;
	pll->reg = reg;
	pll->mask = mask;
	clk->data = pll;
	clk->enable = imxccm_pll_enable;
	clk->get_rate = imxccm_pll_get_rate;
	clk->parent = p;

	if (!clk_register(clk, name))
		return clk;

err:
	free(clk, M_DEVBUF, sizeof(struct clk));
	return NULL;
}

uint32_t
imxccm_pll_get_rate(struct clk *clk)
{
	struct imxccm_softc *sc = imxccm_sc;
	struct clk_pll *pll = clk->data;
	uint32_t div, freq = clk_get_rate_parent(clk);
	uint32_t num, denom;

	switch (pll->clock) {
	case ARM_PLL1:
		if (HREAD4(sc, pll->reg) & CCM_ANALOG_PLL_BYPASS)
			return freq;
		div = HREAD4(sc, pll->reg) & pll->mask;
		return (freq * div) / 2;
	case SYS_PLL2:
	case USB1_PLL3:
	case USB2_PLL:
		div = HREAD4(sc, pll->reg) & pll->mask;
		return freq * (20 + (div << 1));
	case AUD_PLL4:
	case VID_PLL5:
		div = HREAD4(sc, pll->reg) & pll->mask;
		num = HREAD4(sc, pll->reg + CCM_ANALOG_PLL_NUM);
		denom = HREAD4(sc, pll->reg + CCM_ANALOG_PLL_DENOM);
		return (freq * div) + ((freq / denom) * num);
	case ENET_PLL6:
		return 500000;
	default:
		return 0;
	}
}

static void
imxccm_pll_wait_lock(struct clk *clk)
{
	struct imxccm_softc *sc = imxccm_sc;
	struct clk_pll *pll = clk->data;
	int i;

	for (i = 0; i < 100; i++) {
		if (HREAD4(sc, pll->reg) & CCM_ANALOG_PLL_LOCK)
			return;
		delay(100);
	}

	return;
}

int
imxccm_pll_enable(struct clk *clk)
{
	struct imxccm_softc *sc = imxccm_sc;
	struct clk_pll *pll = clk->data;

	if (pll->powerup)
		HSET4(sc, pll->reg, CCM_ANALOG_PLL_POWER);
	else
		HCLR4(sc, pll->reg, CCM_ANALOG_PLL_POWER);

	imxccm_pll_wait_lock(clk);

	HCLR4(sc, pll->reg, CCM_ANALOG_PLL_BYPASS);

	HSET4(sc, pll->reg, CCM_ANALOG_PLL_ENABLE);

	return 0;
}

struct clk_pfd {
	uint32_t reg;
	uint32_t idx;
};

struct clk *
imxccm_pfd(char *name, char *parent, uint32_t reg, uint32_t idx)
{
	struct clk *clk, *p;
	struct clk_pfd *pfd;

	clk = malloc(sizeof(struct clk), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (clk == NULL)
		return NULL;

	p = clk_get(parent);
	if (p == NULL)
		goto err;

	pfd = malloc(sizeof(struct clk_pfd), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (pfd == NULL)
		goto err;

	pfd->reg = reg;
	pfd->idx = idx;
	clk->data = pfd;
	clk->get_rate = imxccm_pfd_get_rate;
	clk->parent = p;

	if (!clk_register(clk, name))
		return clk;

err:
	free(clk, M_DEVBUF, sizeof(struct clk));
	return NULL;
}

uint32_t
imxccm_pfd_get_rate(struct clk *clk)
{
	struct imxccm_softc *sc = imxccm_sc;
	struct clk_pfd *pfd = clk->data;
	uint32_t frac;

	frac = (HREAD4(sc, pfd->reg) >> (pfd->idx * 8)) & 0x3f;

	return (clk_get_rate(clk_get_parent(clk)) * 18) / frac;
}

struct clk_div {
	uint32_t reg;
	uint32_t shift;
	uint32_t mask;
	struct clk_div_table *table;
};

uint32_t
imxccm_div_get_rate(struct clk *clk)
{
	struct imxccm_softc *sc = imxccm_sc;
	struct clk_div *div = clk->data;
	uint32_t podf, reg = HREAD4(sc, div->reg);

	/* Fixup inverted aclk podf. */
	if (div->reg == CCM_CSCMR1)
		reg ^= 0x00600000;

	podf = (reg >> div->shift) & div->mask;

	return clk_get_rate(clk_get_parent(clk)) / (podf + 1);
}

uint32_t
imxccm_div_table_get_rate(struct clk *clk)
{
	struct imxccm_softc *sc = imxccm_sc;
	struct clk_div *div = clk->data;
	struct clk_div_table *table;
	uint32_t divisor = 0, val, reg = HREAD4(sc, div->reg);
	uint32_t rate = clk_get_rate(clk_get_parent(clk));

	/* Fixup inverted aclk podf. */
	if (div->reg == CCM_CSCMR1)
		reg ^= 0x00600000;

	val = (reg >> div->shift) & div->mask;

	for (table = div->table; table->div; table++)
		if (table->val == val)
			divisor = table->div;

	if (divisor)
		rate /= divisor;

	return rate;
}

struct clk *
imxccm_div(char *name, char *parent, uint32_t reg, uint32_t shift, uint32_t width)
{
	return imxccm_div_table(name, parent, reg, shift, width, NULL);
}

struct clk *
imxccm_div_table(char *name, char *parent, uint32_t reg, uint32_t shift, uint32_t width, struct clk_div_table *table)
{
	struct clk *clk, *p;
	struct clk_div *div;

	clk = malloc(sizeof(struct clk), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (clk == NULL)
		return NULL;

	p = clk_get(parent);
	if (p == NULL)
		goto err;

	div = malloc(sizeof(struct clk_div), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (div == NULL)
		goto err;

	div->reg = reg;
	div->shift = shift;
	div->mask = (1UL << width) - 1;
	div->table = table;
	clk->data = div;
	clk->get_rate = imxccm_div_get_rate;
	clk->parent = p;

	if (div->table != NULL)
		clk->get_rate = imxccm_div_table_get_rate;

	if (!clk_register(clk, name))
		return clk;

err:
	free(clk, M_DEVBUF, sizeof(struct clk));
	return NULL;
}

struct clk_fixed
{
	uint32_t val;
};

uint32_t
imxccm_fixed_clock_get_rate(struct clk *clk)
{
	struct clk_fixed *fix = clk->data;
	return fix->val;
}

struct clk *
imxccm_fixed_clock(char *name, uint32_t val)
{
	struct clk *clk;
	struct clk_fixed *fix;

	clk = malloc(sizeof(struct clk), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (clk == NULL)
		return NULL;

	fix = malloc(sizeof(struct clk_fixed), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (fix == NULL)
		goto err;

	fix->val = val;
	clk->data = fix;
	clk->get_rate = imxccm_fixed_clock_get_rate;
	if (!clk_register(clk, name))
		return clk;

err:
	free(clk, M_DEVBUF, sizeof(struct clk));
	return NULL;
}

struct clk_fixed_factor
{
	uint32_t mul;
	uint32_t div;
};

uint32_t
imxccm_fixed_factor_get_rate(struct clk *clk)
{
	struct clk_fixed_factor *fix = clk->data;
	return (clk_get_rate_parent(clk) * fix->mul) / fix->div;
}

struct clk *
imxccm_fixed_factor(char *name, char *parent, uint32_t mul, uint32_t div)
{
	struct clk *clk, *p;
	struct clk_fixed_factor *fix;

	clk = malloc(sizeof(struct clk), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (clk == NULL)
		return NULL;

	p = clk_get(parent);
	if (p == NULL)
		goto err;

	fix = malloc(sizeof(struct clk_fixed_factor), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (fix == NULL)
		goto err;

	fix->mul = mul;
	fix->div = div;
	clk->data = fix;
	clk->get_rate = imxccm_fixed_factor_get_rate;
	clk->parent = p;

	if (!clk_register(clk, name))
		return clk;

err:
	free(clk, M_DEVBUF, sizeof(struct clk));
	return NULL;
}

struct clk *
imxccm_var_clock(char *name, uint32_t (*get_rate) (struct clk *))
{
	struct clk *clk;

	clk = malloc(sizeof(struct clk), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (clk == NULL)
		return NULL;

	clk->get_rate = get_rate;
	if (!clk_register(clk, name))
		return clk;

	return NULL;
}

struct clk_gate {
	uint32_t reg;
	uint32_t val;
	uint32_t shift;
	uint32_t mask;
};

/*
 * CGR values:
 * 00: Clock is off during all modes. Stop enter hardware handshake is disabled.
 * 01: Clock is on in run mode, but off in WAIt and STOP modes.
 * 10: Not applicable (Reserved).
 * 11: Clock is on during all modes, except STOP mode.
 */
struct clk *
imxccm_gate(char *name, char *parent, uint32_t reg, uint32_t shift)
{
	struct clk *clk, *p;
	struct clk_gate *gate;

	clk = malloc(sizeof(struct clk), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (clk == NULL)
		return NULL;

	p = clk_get(parent);
	if (p == NULL)
		goto err;

	gate = malloc(sizeof(struct clk_gate), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (gate == NULL)
		goto err;

	gate->reg = reg;
	gate->val = 1;
	gate->mask = 0x1;
	gate->shift = shift;
	clk->data = gate;
	clk->enable = imxccm_gate_enable;
	clk->disable = imxccm_gate_disable;
	clk->get_rate = clk_get_rate_parent;
	clk->parent = p;

	if (!clk_register(clk, name))
		return clk;

err:
	free(clk, M_DEVBUF, sizeof(struct clk));
	return NULL;
}

/*
 * 2-bit wide clock gate
 */

struct clk *
imxccm_gate2(char *name, char *parent, uint32_t reg, uint32_t shift)
{
	struct clk *clk = imxccm_gate(name, parent, reg, shift);
	struct clk_gate *gate;

	if (clk == NULL)
		return NULL;

	gate = clk->data;
	gate->val = 3;
	gate->mask = 0x3;

	return clk;
}

int
imxccm_gate_enable(struct clk *clk)
{
	struct imxccm_softc *sc = imxccm_sc;
	struct clk_gate *gate = clk->data;
	HWRITE4(sc, gate->reg, (HREAD4(sc, gate->reg)
	    & ~(gate->mask << gate->shift)) | (gate->val << gate->shift));
	return 0;
}

void
imxccm_gate_disable(struct clk *clk)
{
	struct imxccm_softc *sc = imxccm_sc;
	struct clk_gate *gate = clk->data;
	HCLR4(sc, gate->reg, gate->mask << gate->shift);
}

struct clk_mux {
	uint32_t reg;
	uint32_t shift;
	uint32_t mask;
	struct clk **parents;
	int num_parents;
};

struct clk *
imxccm_mux(char *name, uint32_t reg, uint32_t shift, uint32_t width, const char **parents, int num_parents)
{
	struct clk *clk;
	struct clk_mux *mux;
	int i;

	clk = malloc(sizeof(struct clk), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (clk == NULL)
		return NULL;

	mux = malloc(sizeof(struct clk_mux), M_DEVBUF, M_NOWAIT|M_ZERO);
	if (mux == NULL)
		goto err;

	mux->parents = malloc(sizeof(struct clk *) * num_parents, M_DEVBUF, M_NOWAIT|M_ZERO);
	if (mux->parents == NULL)
		goto mux;

	/* Resolve parents now, so that it's faster later. */
	for (i = 0; i < num_parents; i++)
		mux->parents[i] = clk_get(parents[i]);

	mux->reg = reg;
	mux->shift = shift;
	mux->mask = (1UL << width) - 1;
	mux->num_parents = num_parents;
	clk->data = mux;
	clk->set_parent = imxccm_mux_set_parent;
	clk->get_rate = clk_get_rate_parent;
	clk->parent = imxccm_mux_get_parent(clk);

	if (!clk_register(clk, name))
		return clk;

mux:
	free(mux, M_DEVBUF, sizeof(struct clk_mux));
err:
	free(clk, M_DEVBUF, sizeof(struct clk));
	return NULL;
}

struct clk *
imxccm_mux_get_parent(struct clk *clk)
{
	struct imxccm_softc *sc = imxccm_sc;
	struct clk_mux *mux;
	uint32_t i;

	if (clk == NULL || clk->data == NULL)
		return NULL;

	mux = clk->data;
	i = (HREAD4(sc, mux->reg) >> mux->shift) & mux->mask;

	if (i >= mux->num_parents)
		return NULL;

	return mux->parents[i];
}

int
imxccm_mux_set_parent(struct clk *clk, struct clk *parent)
{
	struct imxccm_softc *sc = imxccm_sc;
	struct clk_mux *mux;
	int i;

	if (clk == NULL || parent == NULL || clk->data == NULL)
		return 1;

	mux = clk->data;

	for (i = 0; i < mux->num_parents; i++)
		if (parent == mux->parents[i])
			break;

	if (i >= mux->num_parents)
		return 1;

	HWRITE4(sc, mux->reg, (HREAD4(sc, mux->reg) & ~(mux->mask << mux->shift)) | (i << mux->shift));

	return 0;
}

void
imxccm_disable_usb1_chrg_detect(void)
{
	struct imxccm_softc *sc = imxccm_sc;

	HWRITE4(sc, CCM_ANALOG_USB1_CHRG_DETECT_SET,
	      CCM_ANALOG_USB1_CHRG_DETECT_CHK_CHRG_B
	    | CCM_ANALOG_USB1_CHRG_DETECT_EN_B);
}

void
imxccm_disable_usb2_chrg_detect(void)
{
	struct imxccm_softc *sc = imxccm_sc;

	HWRITE4(sc, CCM_ANALOG_USB2_CHRG_DETECT_SET,
	      CCM_ANALOG_USB2_CHRG_DETECT_CHK_CHRG_B
	    | CCM_ANALOG_USB2_CHRG_DETECT_EN_B);
}

void
imxccm_refresh_sensors(void *arg)
{
	struct imxccm_softc *sc = (struct imxccm_softc *)arg;
	int i = 0;

	strlcpy(sc->sc_freq_sensor[i].desc, "pll arm",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("pll1_sys")) * 1000 * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "pll sys",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("pll2_bus")) * 1000 * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "pll otg",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("pll3_usb_otg")) * 1000 * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "pll usb",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("pll7_usb_host")) * 1000 * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "pll enet",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("pll6_enet")) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "per",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("periph")) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "ipg",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("ipg")) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "ipg per",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("ipg_per")) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "uart",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("uart_serial")) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "usdhc1",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("usdhc1")) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "usdhc2",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("usdhc2")) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "usdhc3",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("usdhc3")) * 1000LL * 1000LL;

	strlcpy(sc->sc_freq_sensor[i].desc, "usdhc4",
	    sizeof(sc->sc_freq_sensor[i].desc));
	sc->sc_freq_sensor[i++].value =
	    clk_get_rate(clk_get("usdhc4")) * 1000LL * 1000LL;
}
