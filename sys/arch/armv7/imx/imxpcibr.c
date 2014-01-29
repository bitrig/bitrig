/*
 * Copyright (c) 2013 Patrick Wildt <patrick@blueri.se>
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
#include <sys/extent.h>
#include <sys/device.h>
#include <sys/evcount.h>
#include <sys/socket.h>
#include <sys/timeout.h>
#include <machine/intr.h>
#include <machine/bus.h>
#include <machine/clock.h>
#include <dev/pci/pcivar.h>

#include <armv7/armv7/armv7var.h>
#include <armv7/imx/imxccmvar.h>
#include <armv7/imx/imxiomuxcvar.h>
#include <armv7/imx/imxgpiovar.h>

/* EP mode */
// FIXME

/* RC mode */
#define PCIE_RC_DEVICEID		0x000
#define PCIE_RC_COMMAND			0x004
#define PCIE_RC_REVID			0x008
#define PCIE_RC_BIST			0x00C
#define PCIE_RC_BAR0			0x010
#define PCIE_RC_BAR1			0x014
#define PCIE_RC_BNR			0x018
#define PCIE_RC_IOBLSSR			0x01C
#define PCIE_RC_MEM_BLR			0x020
#define PCIE_RC_PREF_MEM_BLR		0x024
#define PCIE_RC_PREF_BASE_U32		0x028
#define PCIE_RC_PREF_LIM_U32		0x02C
#define PCIE_RC_IO_BASE_LIM_U16		0x030
#define PCIE_RC_CAPPR			0x034
#define PCIE_RC_EROMBAR			0x038
#define PCIE_RC_EROMMASK		0x03C
#define PCIE_RC_INTERRUPT_LINE		0x03C
#define PCIE_RC_PMCR			0x040
#define PCIE_RC_PMCSR			0x044
#define PCIE_RC_CIDR			0x070
#define PCIE_RC_DCR			0x074
#define PCIE_RC_DCONR			0x078
#define PCIE_RC_LCR			0x07C
#define PCIE_RC_LCSR			0x080
#define PCIE_RC_SCR			0x084
#define PCIE_RC_SCSR			0x088
#define PCIE_RC_RCCR			0x08C
#define PCIE_RC_RSR			0x090
#define PCIE_RC_DCR2			0x094
#define PCIE_RC_DCSR2			0x098
#define PCIE_RC_LCR2			0x09C
#define PCIE_RC_LCSR2			0x0A0
#define PCIE_RC_AER			0x100
#define PCIE_RC_UESR			0x104
#define PCIE_RC_UEMR			0x108
#define PCIE_RC_UESEVR			0x10C
#define PCIE_RC_CESR			0x110
#define PCIE_RC_CEMR			0x114
#define PCIE_RC_ACCR			0x118
#define PCIE_RC_HLR			0x11C
#define PCIE_RC_RECR			0x12C
#define PCIE_RC_RESR			0x130
#define PCIE_RC_ESIR			0x134
#define PCIE_RC_VCECHR			0x140
#define PCIE_RC_PVCCR1			0x144
#define PCIE_RC_PVCCR2			0x148
#define PCIE_RC_PVCCSR			0x14C
#define PCIE_RC_VCRCR			0x150
#define PCIE_RC_VCRCONR			0x154
#define PCIE_RC_VCRSR			0x158

/* Port Logic */
#define PCIE_PL_ALTRTR			0x700
#define PCIE_PL_VSDR			0x704
#define PCIE_PL_PFLR			0x708
#define PCIE_PL_AFLACR			0x70C
#define PCIE_PL_PLCR			0x710
#define PCIE_PL_LSR			0x714
#define PCIE_PL_SNR			0x718
#define PCIE_PL_STRFM1			0x71C
#define PCIE_PL_STRFM2			0x720
#define PCIE_PL_AMODNPSR		0x724
#define PCIE_PL_DEBUG0			0x728
#define PCIE_PL_DEBUG1			0x72C
#define PCIE_PL_TPFCSR			0x730
#define PCIE_PL_TNSCSR			0x734
#define PCIE_PL_TCFCSR			0x738
#define PCIE_PL_QSR			0x73C
#define PCIE_PL_VCTAR1			0x740
#define PCIE_PL_VCTAR2			0x744
#define PCIE_PL_VC0PRQC			0x748
#define PCIE_PL_VC0NRQC			0x74C
#define PCIE_PL_VC0CRQC			0x750
#define PCIE_PL_VCnPRQC			0x754
#define PCIE_PL_VCnNRQC			0x758
#define PCIE_PL_VCnCRQC			0x75C
#define PCIE_PL_VC0PBD			0x7A8
#define PCIE_PL_VC0NPBD			0x7AC
#define PCIE_PL_VC0CBD			0x7B0
#define PCIE_PL_VC1PBD			0x7B4
#define PCIE_PL_VC1NPBD			0x7B8
#define PCIE_PL_VC1CBD			0x7BC
#define PCIE_PL_G2CR			0x80C
#define PCIE_PL_PHY_STATUS		0x810
#define PCIE_PL_PHY_CTRL		0x814
#define PCIE_PL_MRCCR0			0x818
#define PCIE_PL_MRCCR1			0x81C
#define PCIE_PL_MSICA			0x820
#define PCIE_PL_MSICUA			0x824
#define PCIE_PL_MSICIn_ENB		0x828
#define PCIE_PL_MSICIn_MASK		0x82C
#define PCIE_PL_MSICIn_STATUS		0x830
#define PCIE_PL_MSICGPIO		0x888
#define PCIE_PL_iATUVR			0x900
#define PCIE_PL_iATURC1			0x904
#define PCIE_PL_iATURC2			0x908
#define PCIE_PL_iATURLBA		0x90C
#define PCIE_PL_iATURUBA		0x910
#define PCIE_PL_iATURLA			0x914
#define PCIE_PL_iATURLTA		0x918
#define PCIE_PL_iATURUTA		0x91C

/* bits and bytes */
#define PCIE_RC_COMMAND_IO_SPACE		(1 << 0)
#define PCIE_RC_COMMAND_MEMORY_SPACE		(1 << 1)
#define PCIE_RC_COMMAND_BUS_MASTER		(1 << 2)
#define PCIE_RC_COMMAND_SERR			(0x100)
#define PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN1	(0x1 << 0)
#define PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN2	(0x2 << 0)
#define PCIE_RC_LCR_MAX_LINK_SPEEDS_MASK	(0xf << 0)
#define PCIE_PL_PFLR_FORCE_LINK			(0x1 << 15)
#define PCIE_PL_PFLR_LINK_STATE_MASK		(0x3f << 16)
#define PCIE_PL_PLCR_LINK_MODE_ENABLE_MASK	(0x3f << 16)
#define PCIE_PL_PLCR_LINK_MODE_ENABLE_1_LANES	(0x01 << 16)
#define PCIE_PL_PLCR_LINK_MODE_ENABLE_2_LANES	(0x03 << 16)
#define PCIE_PL_PLCR_LINK_MODE_ENABLE_4_LANES	(0x07 << 16)
#define PCIE_PL_PLCR_LINK_MODE_ENABLE_8_LANES	(0x0f << 16)
#define PCIE_PL_PLCR_LINK_MODE_ENABLE_16_LANES	(0x1f << 16)
#define PCIE_PL_G2CR_PREDETER_SPEED_CHANGE	(0x1 << 17)
#define PCIE_PL_G2CR_PREDETER_LANES_MASK	(0x1ff << 8)
#define PCIE_PL_G2CR_PREDETER_LANES_1		(0x1 << 8)
#define PCIE_PL_G2CR_PREDETER_LANES_2		(0x2 << 8)
#define PCIE_PL_G2CR_PREDETER_LANES_4		(0x4 << 8)
#define PCIE_PL_iATUVR_REGION_INDEX0		(0 << 0)
#define PCIE_PL_iATUVR_REGION_INDEX1		(1 << 0)
#define PCIE_PL_iATUVR_REGION_OUTBOUND		(0 << 31)
#define PCIE_PL_iATUVR_REGION_INBOUND		(1U << 31)
#define PCIE_PL_iATURC1_TYPE_MEM_RD_WR		(0)
#define PCIE_PL_iATURC1_TYPE_MEM_RD_LK		(1)
#define PCIE_PL_iATURC1_TYPE_IO_RD_WR		(2)
#define PCIE_PL_iATURC1_TYPE_CONF_WR_0		(4)
#define PCIE_PL_iATURC1_TYPE_CONF_WR_1		(5)
#define PCIE_PL_iATURC2_REGION_ENABLE		(1U << 31)

/* FU */
/* control bus bit definition */
#define PCIE_CR_CTL_DATA_LOC 0
#define PCIE_CR_CTL_CAP_ADR_LOC 16
#define PCIE_CR_CTL_CAP_DAT_LOC 17
#define PCIE_CR_CTL_WR_LOC 18
#define PCIE_CR_CTL_RD_LOC 19
#define PCIE_CR_STAT_DATA_LOC 0
#define PCIE_CR_STAT_ACK_LOC 16

#define  PCIE_CONF_BUS(b)               (((b) & 0xFF) << 20)
#define  PCIE_CONF_DEV(d)               (((d) & 0x1F) << 15)
#define  PCIE_CONF_FUNC(f)              (((f) & 0x7) << 12)
#define  PCIE_CONF_REG(r)               ((r) & ~0x3)

#define SSP_CR_LANE0_DIG_RX_ASIC_OUT 0x100D
#define SSP_CR_LANE0_DIG_RX_OVRD_IN_LO 0x1005
#define SSP_CR_LANE0_DIG_RX_OVRD_IN_LO_RX_DATA_EN (1 << 5)
#define SSP_CR_LANE0_DIG_RX_OVRD_IN_LO_RX_PLL_EN (1 << 3)

#define PCI_CLASS_BRIDGE_PCI		0x0604

//#define PCIE_UTILITE_PWR		(7*32+4)
#define PCIE_UTILITE_ETH_RST		(0*32+26)

#define HREAD4(sc, reg)							\
	(bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, (reg)))
#define HWRITE4(sc, reg, val)						\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, (reg), (val))
#define HSET4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) | (bits))
#define HCLR4(sc, reg, bits)						\
	HWRITE4((sc), (reg), HREAD4((sc), (reg)) & ~(bits))

struct imxpcibr_softc {
	struct device			sc_dev;
	bus_space_tag_t			sc_iot;
	bus_space_handle_t		sc_ioh;
	bus_space_handle_t		sc_cfg0;
	bus_space_handle_t		sc_cfg1;
	bus_dma_tag_t			sc_dma_tag;
	uint32_t			sc_iobase;
	uint32_t			sc_iosize;
	uint32_t			sc_membase;
	uint32_t			sc_memsize;
	uint32_t			sc_cfg0base;
	uint32_t			sc_cfg1base;
	uint32_t			sc_cfg0size;
	uint32_t			sc_cfg1size;
	struct extent			*sc_ioex;
	struct extent			*sc_memex;
	char				sc_ioex_name[32];
	char				sc_memex_name[32];
	struct arm32_pci_chipset	sc_pc;
	int				sc_int;
};

struct imxpcibr_softc *imxpcibr_sc;

void imxpcibr_attach(struct device *, struct device *, void *);
void imxpcibr_regions_setup(struct imxpcibr_softc *);
void imxpcibr_reset_card(struct imxpcibr_softc *);
int imxpcibr_link_up(struct imxpcibr_softc *);
int imxpcibr_wait_for_link(struct imxpcibr_softc *);
int imxpcibr_phy_ack_polling(struct imxpcibr_softc *, int, int);
int imxpcibr_phy_cap_addr(struct imxpcibr_softc *, int);
uint32_t imxpcibr_phy_read(struct imxpcibr_softc *, int);
void imxpcibr_phy_write(struct imxpcibr_softc *, int, uint32_t);
void imxpcibr_attach_hook(struct device *, struct device *, struct pcibus_attach_args *);
int imxpcibr_bus_maxdevs(void *, int);
pcitag_t imxpcibr_make_tag(void *, int, int, int);
void imxpcibr_decompose_tag(void *, pcitag_t, int *, int *, int *);
int imxpcibr_conf_size(void *, pcitag_t);
void imxpcibr_conf_prog_cfg0(struct imxpcibr_softc *, pcitag_t);
void imxpcibr_conf_prog_cfg1(struct imxpcibr_softc *, pcitag_t);
void imxpcibr_conf_prog_mem_outbound(struct imxpcibr_softc *, pcitag_t);
void imxpcibr_conf_prog_io_outbound(struct imxpcibr_softc *, pcitag_t);
pcireg_t imxpcibr_conf_read(void *, pcitag_t, int);
void imxpcibr_conf_write(void *, pcitag_t, int, pcireg_t);
int imxpcibr_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
int imxpcibr_intr_map_msi(struct pci_attach_args *, pci_intr_handle_t *);
const char *imxpcibr_intr_string(void *, pci_intr_handle_t);
void *imxpcibr_intr_establish(void *, pci_intr_handle_t, int, int (*func)(void *), void *, const char *);
void imxpcibr_intr_disestablish(void *, void *);

/* XXX */
struct sigdata;
extern void data_fault_hook(int code, int (*func)(trapframe_t *, u_int, u_int,
    struct proc *, struct sigdata *));
int imxpcibr_abort_handler(trapframe_t *, u_int, u_int, struct proc *,
    struct sigdata *);

struct cfattach imxpcibr_ca = {
	sizeof (struct imxpcibr_softc), NULL, imxpcibr_attach
};

struct cfdriver imxpcibr_cd = {
	NULL, "imxpcibr", DV_DULL
};

void
imxpcibr_attach(struct device *parent, struct device *self, void *args)
{
	struct armv7_attach_args *aa = args;
	struct imxpcibr_softc *sc = (struct imxpcibr_softc *) self;
	struct pcibus_attach_args pba;

	sc->sc_iot = aa->aa_iot;
	sc->sc_dma_tag = aa->aa_dmat;
	if (bus_space_map(sc->sc_iot, aa->aa_dev->mem[0].addr,
	    aa->aa_dev->mem[0].size, 0, &sc->sc_ioh))
		panic("imxpcibr_attach: bus_space_map failed!");

	sc->sc_cfg0base = 0x01F00000;
	sc->sc_cfg1base = 0x01F40000;
	sc->sc_cfg0size = sc->sc_cfg1size = 0x40000;

	if (bus_space_map(sc->sc_iot, sc->sc_cfg0base,
	    sc->sc_cfg0size, 0, &sc->sc_cfg0))
		panic("imxpcibr_attach: bus_space_map failed!");

	if (bus_space_map(sc->sc_iot, sc->sc_cfg1base,
	    sc->sc_cfg1size, 0, &sc->sc_cfg1))
		panic("imxpcibr_attach: bus_space_map failed!");

	printf("\n");

	sc->sc_iobase = aa->aa_dev->mem[1].addr;
	sc->sc_iosize = aa->aa_dev->mem[1].size;
	sc->sc_membase = aa->aa_dev->mem[2].addr;
	sc->sc_memsize = aa->aa_dev->mem[2].size;
	sc->sc_int = aa->aa_dev->irq[0];

	/*
	 * FIXME: PCI abort handling.
	 */
	data_fault_hook(16+6, imxpcibr_abort_handler);

	/*
	 * Map PCIe address space.
	 */
	snprintf(sc->sc_ioex_name, sizeof(sc->sc_ioex_name),
	    "%s pciio", sc->sc_dev.dv_xname);
	sc->sc_ioex = extent_create(sc->sc_ioex_name, 0x00000000, 0xffffffff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT | EX_FILLED);
	extent_free(sc->sc_ioex, 0, sc->sc_iosize, EX_NOWAIT);

	snprintf(sc->sc_memex_name, sizeof(sc->sc_memex_name),
	    "%s pcimem", sc->sc_dev.dv_xname);
	sc->sc_memex = extent_create(sc->sc_memex_name, 0x00000000, 0xffffffff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT | EX_FILLED);
	extent_free(sc->sc_memex, sc->sc_membase, sc->sc_memsize, EX_NOWAIT);

	// FIXME: make sure clocks are all right
	clk_enable(clk_get("sata_ref_100m"));
	clk_enable(clk_get("pcie_ref_125m"));
	clk_enable(clk_get("lvds1_out"));
	clk_enable(clk_get("pcie_axi"));
	delay(1000);

	/*
	 * Reset PCIe before turning it on. We need to get it into a
	 * safe mode before we're doing anything else.
	 */
	if (imxiomuxc_pcie_get_refclk() && imxiomuxc_pcie_get_ltssm()) {
		HWRITE4(sc, PCIE_PL_PFLR, (HREAD4(sc, PCIE_PL_PFLR) &
		    ~PCIE_PL_PFLR_LINK_STATE_MASK) | PCIE_PL_PFLR_FORCE_LINK);
		imxiomuxc_pcie_ltssm(0);
	}

	imxiomuxc_pcie_test_powerdown(1);
	imxiomuxc_pcie_refclk(0);

	/*
	 * Set up IOMUXC registers.
	 */
	imxiomuxc_enable_pcie();

	//imxgpio_set_dir(PCIE_UTILITE_PWR, IMXGPIO_DIR_OUT);
	//imxgpio_set_bit(PCIE_UTILITE_PWR);

	/*
	 * Enable power.
	 */
	imxiomuxc_pcie_test_powerdown(0);
	delay(10);
	imxiomuxc_pcie_refclk(1);
	delay(500);

	/*
	 * Reset external cards via GPIO.
	 */
	imxpcibr_reset_card(sc);

	/*
	 * Setup regions, BAR0 and bridge mode.
	 */
	delay(3000);
	imxpcibr_regions_setup(sc);
	delay(3000);

	HWRITE4(sc, PCIE_RC_BAR0, 0x00000000);
	HWRITE4(sc, PCIE_RC_REVID, PCI_CLASS_BRIDGE_PCI << 16);

	/*
	 * Force Gen1.  Otherwise devices that are started in Gen2 mode
	 * might not be detected at all.
	 */
	HWRITE4(sc, PCIE_RC_LCR, (HREAD4(sc, PCIE_RC_LCR) &
	    ~PCIE_RC_LCR_MAX_LINK_SPEEDS_MASK) |
	    PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN1);

	/*
	 * Start link up.
	 */
	imxiomuxc_pcie_ltssm(1);

	if (!imxpcibr_wait_for_link(sc))
		return;

	/*
	 * Now that we are running, go to Gen2.
	 */
	HWRITE4(sc, PCIE_RC_LCR, (HREAD4(sc, PCIE_RC_LCR) &
	    ~PCIE_RC_LCR_MAX_LINK_SPEEDS_MASK) |
	    PCIE_RC_LCR_MAX_LINK_SPEEDS_GEN2);

	/*
	 * Negotiate speed.
	 */
	HSET4(sc, PCIE_PL_G2CR, PCIE_PL_G2CR_PREDETER_SPEED_CHANGE);

	for (int i = 0; i < 200; i++) {
		if (!(HREAD4(sc, PCIE_PL_G2CR) &
		    PCIE_PL_G2CR_PREDETER_SPEED_CHANGE))
			break;
		if (i >= 200) {
			printf("%s: speed not negotiated\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		delay(1000);
	}

	if (!imxpcibr_wait_for_link(sc))
		return;

	sc->sc_pc.pc_conf_v = sc;
	sc->sc_pc.pc_attach_hook = imxpcibr_attach_hook;
	sc->sc_pc.pc_bus_maxdevs = imxpcibr_bus_maxdevs;
	sc->sc_pc.pc_make_tag = imxpcibr_make_tag;
	sc->sc_pc.pc_decompose_tag = imxpcibr_decompose_tag;
	sc->sc_pc.pc_conf_size = imxpcibr_conf_size;
	sc->sc_pc.pc_conf_read = imxpcibr_conf_read;
	sc->sc_pc.pc_conf_write = imxpcibr_conf_write;

	// FIXME: interrupts
	sc->sc_pc.pc_intr_v = sc;
	sc->sc_pc.pc_intr_map = imxpcibr_intr_map;
	sc->sc_pc.pc_intr_map_msi = imxpcibr_intr_map_msi;
	sc->sc_pc.pc_intr_string = imxpcibr_intr_string;
	sc->sc_pc.pc_intr_establish = imxpcibr_intr_establish;
	sc->sc_pc.pc_intr_disestablish = imxpcibr_intr_disestablish;

	bzero(&pba, sizeof(pba));
	pba.pba_dmat = sc->sc_dma_tag;

	pba.pba_busname = "pci";
	pba.pba_iot = sc->sc_iot;
	pba.pba_memt = sc->sc_iot;
	pba.pba_ioex = sc->sc_ioex;
	pba.pba_memex = sc->sc_memex;
	pba.pba_pc = &sc->sc_pc;
	pba.pba_domain = pci_ndomains++;
	pba.pba_bus = 0;

	imxpcibr_sc = sc;

	config_found(self, &pba, NULL);
}

void
imxpcibr_regions_setup(struct imxpcibr_softc *sc)
{
#ifdef notyet
	int i;
#endif

	/* FIXME: dynamic num of lanes */
	HWRITE4(sc, PCIE_PL_PLCR, (HREAD4(sc, PCIE_PL_PLCR)
	    & ~PCIE_PL_PLCR_LINK_MODE_ENABLE_MASK)
	    | PCIE_PL_PLCR_LINK_MODE_ENABLE_1_LANES);

	/* FIXME: dynamic num of lanes */
	HWRITE4(sc, PCIE_PL_G2CR, (HREAD4(sc, PCIE_PL_G2CR)
	    & ~PCIE_PL_G2CR_PREDETER_LANES_MASK)
	    | PCIE_PL_G2CR_PREDETER_LANES_1);

	HWRITE4(sc, PCIE_RC_BAR0, 0x00000004);
	HWRITE4(sc, PCIE_RC_BAR1, 0x00000000);

	HWRITE4(sc, PCIE_RC_INTERRUPT_LINE, (HREAD4(sc, PCIE_RC_INTERRUPT_LINE)
	    & 0xffff00ff) | 0x00000100);

	HWRITE4(sc, PCIE_RC_BNR, (HREAD4(sc, PCIE_RC_BNR)
	    & 0xff000000) | 0x00010100);

	HWRITE4(sc, PCIE_RC_MEM_BLR, ((sc->sc_membase & 0xfff00000) >> 16)
	    | ((sc->sc_membase + sc->sc_memsize - 1) & 0xfff00000));

	HWRITE4(sc, PCIE_RC_COMMAND, (HREAD4(sc, PCIE_RC_COMMAND)
	    & 0xFFFF0000) | PCIE_RC_COMMAND_IO_SPACE
	    | PCIE_RC_COMMAND_MEMORY_SPACE | PCIE_RC_COMMAND_BUS_MASTER
	    | PCIE_RC_COMMAND_SERR);

	HSET4(sc, PCIE_RC_REVID, PCI_CLASS_BRIDGE_PCI << 16);

	// TODO: MSI
#ifdef notyet
#define MSI_MATCH_ADDR  0x01FF8000
	HWRITE4(sc, PCIE_PL_MSICA, MSI_MATCH_ADDR);
	HWRITE4(sc, PCIE_PL_MSICUA, 0);
	for (i = 0; i < 8; i++) {
		HWRITE4(sc, PCIE_PL_MSICIn_ENB + 0xC * i, 0);
		HWRITE4(sc, PCIE_PL_MSICIn_MASK + 0xC * i, ~0);
		HWRITE4(sc, PCIE_PL_MSICIn_STATUS + 0xC * i, ~0);
	}
#endif
}

void
imxpcibr_reset_card(struct imxpcibr_softc *sc)
{
	switch (board_id)
	{
	case BOARD_ID_IMX6_UTILITE:
		imxgpio_clear_bit(PCIE_UTILITE_ETH_RST);
		imxgpio_set_dir(PCIE_UTILITE_ETH_RST, IMXGPIO_DIR_OUT);
		delay(100 * 1000);
		imxgpio_set_bit(PCIE_UTILITE_ETH_RST);
		break;
	}
}

int
imxpcibr_wait_for_link(struct imxpcibr_softc *sc)
{
	for (int i = 0; i < 200; i++) {
		if (imxpcibr_link_up(sc))
			return 1;
		delay(1000);
	}

	printf("%s: link did not come up\n", sc->sc_dev.dv_xname);

	return 0;
}

int
imxpcibr_link_up(struct imxpcibr_softc *sc)
{
	int link = 0;
	uint32_t ltssm, rx_valid, val;
	// DEBUG1, bit 36, xmlh_link_up LTSSM reports PHY link up
	link = HREAD4(sc, PCIE_PL_DEBUG1) & (1 << (36-32));
	if (link)
		return 1;

	rx_valid = imxpcibr_phy_read(sc, SSP_CR_LANE0_DIG_RX_ASIC_OUT);
	ltssm = HREAD4(sc, PCIE_PL_DEBUG0) & 0x3F;
	if (rx_valid & 0x01)
		return 0;
	if (ltssm != 0x0D)
		return 0;

	printf("Transition to gen2 is stuck, reset PHY!\n");
	val = imxpcibr_phy_read(sc, SSP_CR_LANE0_DIG_RX_OVRD_IN_LO);
	val |= (SSP_CR_LANE0_DIG_RX_OVRD_IN_LO_RX_DATA_EN
	    | SSP_CR_LANE0_DIG_RX_OVRD_IN_LO_RX_PLL_EN);
	imxpcibr_phy_write(sc, SSP_CR_LANE0_DIG_RX_OVRD_IN_LO,
	    val);
	delay(2000);
	val = imxpcibr_phy_read(sc, SSP_CR_LANE0_DIG_RX_OVRD_IN_LO);
	val &= ~(SSP_CR_LANE0_DIG_RX_OVRD_IN_LO_RX_DATA_EN
	    | SSP_CR_LANE0_DIG_RX_OVRD_IN_LO_RX_PLL_EN);
	imxpcibr_phy_write(sc, SSP_CR_LANE0_DIG_RX_OVRD_IN_LO,
	    val);

	return 0;
}

int
imxpcibr_phy_ack_polling(struct imxpcibr_softc *sc, int loop, int expected)
{
	int val = 0;

	do {
		val = HREAD4(sc, PCIE_PL_PHY_STATUS);
		val = (val >> PCIE_CR_STAT_ACK_LOC) & 0x1;
	} while ((--loop) && (val != expected));

	return (val == expected);
}

int
imxpcibr_phy_cap_addr(struct imxpcibr_softc *sc, int addr)
{
	/* write addr */
	HWRITE4(sc, PCIE_PL_PHY_CTRL, addr << PCIE_CR_CTL_DATA_LOC);

	/* capture addr */
	HWRITE4(sc, PCIE_PL_PHY_CTRL, (addr << PCIE_CR_CTL_DATA_LOC)
	    | (0x1 << PCIE_CR_CTL_CAP_ADR_LOC));

	/* wait for ack */
	if (!imxpcibr_phy_ack_polling(sc, 100, 1))
		return 0;

	/* deassert cap addr */
	HWRITE4(sc, PCIE_PL_PHY_CTRL, addr << PCIE_CR_CTL_DATA_LOC);

	/* wait for ack */
	if (!imxpcibr_phy_ack_polling(sc, 100, 0))
		return 0;

	return 1;
}

uint32_t
imxpcibr_phy_read(struct imxpcibr_softc *sc, int addr)
{
	uint32_t val;

	/* write addr */
	if (!imxpcibr_phy_cap_addr(sc, addr))
		return 0;

	HWRITE4(sc, PCIE_PL_PHY_CTRL, 1 << PCIE_CR_CTL_RD_LOC);

	/* wait for ack */
	if (!imxpcibr_phy_ack_polling(sc, 100, 1))
		return 0;

	/* data */
	val = HREAD4(sc, PCIE_PL_PHY_STATUS)
	    & (0xFFFF << PCIE_CR_STAT_DATA_LOC);

	/* deassert */
	HWRITE4(sc, PCIE_PL_PHY_CTRL, 0);

	/* wait for ack deassertion */
	if (!imxpcibr_phy_ack_polling(sc, 100, 0))
		return 0;

	return val;
}

void
imxpcibr_phy_write(struct imxpcibr_softc *sc, int addr, uint32_t val)
{
	/* write addr */
	if (!imxpcibr_phy_cap_addr(sc, addr))
		return;

	HWRITE4(sc, PCIE_PL_PHY_CTRL, val << PCIE_CR_CTL_DATA_LOC);

	/* capture data */
	HWRITE4(sc, PCIE_PL_PHY_CTRL, (val << PCIE_CR_CTL_DATA_LOC)
	    | (1 << PCIE_CR_CTL_CAP_DAT_LOC));

	/* wait for ack */
	if (!imxpcibr_phy_ack_polling(sc, 100, 1))
		return;

	/* deassert */
	HWRITE4(sc, PCIE_PL_PHY_CTRL, val << PCIE_CR_CTL_DATA_LOC);

	/* wait for ack deassertion */
	if (!imxpcibr_phy_ack_polling(sc, 100, 0))
		return;

	/* assert write */
	HWRITE4(sc, PCIE_PL_PHY_CTRL, 1 << PCIE_CR_CTL_WR_LOC);

	/* wait for ack */
	if (!imxpcibr_phy_ack_polling(sc, 100, 1))
		return;

	/* deassert */
	HWRITE4(sc, PCIE_PL_PHY_CTRL, val << PCIE_CR_CTL_DATA_LOC);

	/* wait for ack deassertion */
	if (!imxpcibr_phy_ack_polling(sc, 100, 0))
		return;

	HWRITE4(sc, PCIE_PL_PHY_CTRL, 0);
}

void
imxpcibr_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{
}

int
imxpcibr_bus_maxdevs(void *sc, int busno) {
	return(1);
}

#define BUS_SHIFT 24
#define DEVICE_SHIFT 19
#define FNC_SHIFT 16

pcitag_t
imxpcibr_make_tag(void *sc, int bus, int dev, int fnc)
{
	return (bus << BUS_SHIFT) | (dev << DEVICE_SHIFT) | (fnc << FNC_SHIFT);
}

void
imxpcibr_decompose_tag(void *sc, pcitag_t tag, int *busp, int *devp, int *fncp)
{
	if (busp != NULL)
		*busp = (tag >> BUS_SHIFT) & 0xff;
	if (devp != NULL)
		*devp = (tag >> DEVICE_SHIFT) & 0x1f;
	if (fncp != NULL)
		*fncp = (tag >> FNC_SHIFT) & 0x7;
}

int
imxpcibr_conf_size(void *sc, pcitag_t tag)
{
	return PCI_CONFIG_SPACE_SIZE;
}

void
imxpcibr_conf_prog_cfg0(struct imxpcibr_softc *sc, pcitag_t tag)
{
	HWRITE4(sc, PCIE_PL_iATUVR,
	    PCIE_PL_iATUVR_REGION_OUTBOUND | PCIE_PL_iATUVR_REGION_INDEX0);
	HWRITE4(sc, PCIE_PL_iATURLBA, sc->sc_cfg0base);
	HWRITE4(sc, PCIE_PL_iATURUBA, 0); /* upper 32-bits of cfg0base */
	HWRITE4(sc, PCIE_PL_iATURLA, sc->sc_cfg0base + sc->sc_cfg0size - 1);
	HWRITE4(sc, PCIE_PL_iATURLTA, tag);
	HWRITE4(sc, PCIE_PL_iATURUTA, 0);
	HWRITE4(sc, PCIE_PL_iATURC1, PCIE_PL_iATURC1_TYPE_CONF_WR_0);
	HWRITE4(sc, PCIE_PL_iATURC2, PCIE_PL_iATURC2_REGION_ENABLE);
}

void
imxpcibr_conf_prog_cfg1(struct imxpcibr_softc *sc, pcitag_t tag)
{
	HWRITE4(sc, PCIE_PL_iATUVR,
	    PCIE_PL_iATUVR_REGION_OUTBOUND | PCIE_PL_iATUVR_REGION_INDEX1);
	HWRITE4(sc, PCIE_PL_iATURC1, PCIE_PL_iATURC1_TYPE_CONF_WR_1);
	HWRITE4(sc, PCIE_PL_iATURLBA, sc->sc_cfg1base);
	HWRITE4(sc, PCIE_PL_iATURUBA, 0); /* upper 32-bits of cfg1base */
	HWRITE4(sc, PCIE_PL_iATURLA, sc->sc_cfg1base + sc->sc_cfg1size - 1);
	HWRITE4(sc, PCIE_PL_iATURLTA, tag);
	HWRITE4(sc, PCIE_PL_iATURUTA, 0);
	HWRITE4(sc, PCIE_PL_iATURC2, PCIE_PL_iATURC2_REGION_ENABLE);
}

void
imxpcibr_conf_prog_mem_outbound(struct imxpcibr_softc *sc, pcitag_t tag)
{
	HWRITE4(sc, PCIE_PL_iATUVR,
	    PCIE_PL_iATUVR_REGION_OUTBOUND | PCIE_PL_iATUVR_REGION_INDEX0);
	HWRITE4(sc, PCIE_PL_iATURC1, PCIE_PL_iATURC1_TYPE_MEM_RD_WR);
	HWRITE4(sc, PCIE_PL_iATURLBA, sc->sc_membase);
	HWRITE4(sc, PCIE_PL_iATURUBA, 0); /* upper 32-bits of membase */
	HWRITE4(sc, PCIE_PL_iATURLA, sc->sc_membase + sc->sc_memsize - 1);
	HWRITE4(sc, PCIE_PL_iATURLTA, sc->sc_membase);
	HWRITE4(sc, PCIE_PL_iATURUTA, 0); /* upper 32-bits */
	HWRITE4(sc, PCIE_PL_iATURC2, PCIE_PL_iATURC2_REGION_ENABLE);
}

void
imxpcibr_conf_prog_io_outbound(struct imxpcibr_softc *sc, pcitag_t tag)
{
	HWRITE4(sc, PCIE_PL_iATUVR,
	    PCIE_PL_iATUVR_REGION_OUTBOUND | PCIE_PL_iATUVR_REGION_INDEX1);
	HWRITE4(sc, PCIE_PL_iATURC1, PCIE_PL_iATURC1_TYPE_IO_RD_WR);
	HWRITE4(sc, PCIE_PL_iATURLBA, sc->sc_iobase);
	HWRITE4(sc, PCIE_PL_iATURUBA, 0); /* upper 32-bits of iobase */
	HWRITE4(sc, PCIE_PL_iATURLA, sc->sc_iobase + sc->sc_iosize - 1);
	HWRITE4(sc, PCIE_PL_iATURLTA, 0);
	HWRITE4(sc, PCIE_PL_iATURUTA, 0);
	HWRITE4(sc, PCIE_PL_iATURC2, PCIE_PL_iATURC2_REGION_ENABLE);
}

pcireg_t
imxpcibr_conf_read(void *v, pcitag_t tag, int offset)
{
	struct imxpcibr_softc *sc = (struct imxpcibr_softc *)v;
	int bus, dev, fn;
	pcireg_t data = 0;

	imxpcibr_decompose_tag(sc, tag, &bus, &dev, &fn);

	if (!bus)
		data = HREAD4(sc, offset & ~0x3);
	else {
		if (bus == 1) {
			imxpcibr_conf_prog_cfg0(sc, tag);
			data = bus_space_read_4(sc->sc_iot, sc->sc_cfg0, offset & ~0x3);
			imxpcibr_conf_prog_mem_outbound(sc, tag);
		} else {
			imxpcibr_conf_prog_cfg1(sc, tag);
			data = bus_space_read_4(sc->sc_iot, sc->sc_cfg1, offset & ~0x3);
			imxpcibr_conf_prog_io_outbound(sc, tag);
		}
	}

	return data;
}

void
imxpcibr_conf_write(void *v, pcitag_t tag, int offset, pcireg_t data)
{
	struct imxpcibr_softc *sc = (struct imxpcibr_softc *)v;
	int bus, dev, fn;

	imxpcibr_decompose_tag(sc, tag, &bus, &dev, &fn);

	if (!bus)
		HWRITE4(sc, offset & ~0x3, data);
	else {
		if (bus == 1) {
			imxpcibr_conf_prog_cfg0(sc, tag);
			bus_space_write_4(sc->sc_iot, sc->sc_cfg0, offset & ~0x3, data);
			imxpcibr_conf_prog_mem_outbound(sc, tag);
		} else {
			imxpcibr_conf_prog_cfg1(sc, tag);
			bus_space_write_4(sc->sc_iot, sc->sc_cfg1, offset & ~0x3, data);
			imxpcibr_conf_prog_io_outbound(sc, tag);
		}
	}
}

int
imxpcibr_intr_map(struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	*ihp = (pci_intr_handle_t) pa->pa_pc;
	return 0;
}

int
imxpcibr_intr_map_msi(struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	*ihp = (pci_intr_handle_t) pa->pa_pc;
	return -1;
}

const char *
imxpcibr_intr_string(void *sc, pci_intr_handle_t ih)
{
	pci_chipset_tag_t pc = (pci_chipset_tag_t) ih;
	struct imxpcibr_softc *brsc = pc->pc_conf_v;
	static char str[16];

	snprintf(str, sizeof str, "irq %ld", brsc->sc_int);
	return(str);
}

void *
imxpcibr_intr_establish(void *sc, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg, const char *name)
{
	pci_chipset_tag_t pc = (pci_chipset_tag_t) ih;
	struct imxpcibr_softc *brsc = pc->pc_conf_v;
	return arm_intr_establish(brsc->sc_int + 3, level,
	    func, arg, (void *)name);
}

void
imxpcibr_intr_disestablish(void *sc, void *cookie)
{
	/* do something */
}

int imxpcibr_abort_handler(trapframe_t *arg1, u_int arg2, u_int arg3,
    struct proc *arg4, struct sigdata *arg5)
{
	return 0;
}
