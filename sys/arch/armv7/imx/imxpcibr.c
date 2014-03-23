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
#include <armv7/imx/imxvar.h>
#include <armv7/imx/imxccmvar.h>
#include <armv7/imx/imxiomuxcvar.h>

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

/* bits and bytes */
#define PCIE_RC_COMMAND_IO_SPACE	(1 << 0)
#define PCIE_RC_COMMAND_MEMORY_SPACE	(1 << 1)
#define PCIE_RC_COMMAND_BUS_MASTER	(1 << 2)

#define PCI_CLASS_BRIDGE_PCI		0x0604

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
	bus_dma_tag_t			sc_dma_tag;
	bus_space_handle_t		sc_membus_ioh;
	bus_space_handle_t		sc_iobus_ioh;
	struct extent			*sc_ioex;
	struct extent			*sc_memex;
	char				sc_ioex_name[32];
	char				sc_memex_name[32];
	struct arm32_pci_chipset	sc_pc;
};

struct imxpcibr_softc *imxpcibr_sc;

void imxpcibr_attach(struct device *, struct device *, void *);
void imxpcibr_attach_hook(struct device *, struct device *, struct pcibus_attach_args *);
int imxpcibr_bus_maxdevs(void *, int);
pcitag_t imxpcibr_make_tag(void *, int, int, int);
void imxpcibr_decompose_tag(void *, pcitag_t, int *, int *, int *);
int imxpcibr_conf_size(void *, pcitag_t);
pcireg_t imxpcibr_conf_read(void *, pcitag_t, int);
void imxpcibr_conf_write(void *, pcitag_t, int, pcireg_t);
int imxpcibr_intr_map(struct pci_attach_args *, pci_intr_handle_t *);
const char *imxpcibr_intr_string(void *, pci_intr_handle_t);
void *imxpcibr_intr_establish(void *, pci_intr_handle_t, int, int (*func)(void *), void *, const char *);
void imxpcibr_intr_disestablish(void *, void *);

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

	printf("\n");

	return;

	imxiomuxc_enable_pcie();
	imxiomuxc_pcie_test_powerdown(0);
	clk_enable(clk_get("pcie_axi"));
	clk_enable(clk_get("pcie_ref_125m"));
	imxiomuxc_pcie_test_powerdown(1);

	HSET4(sc, PCIE_RC_COMMAND, PCIE_RC_COMMAND_IO_SPACE |
	    PCIE_RC_COMMAND_MEMORY_SPACE | PCIE_RC_COMMAND_BUS_MASTER);

	HSET4(sc, PCIE_RC_REVID, PCI_CLASS_BRIDGE_PCI << 16);

	snprintf(sc->sc_ioex_name, sizeof(sc->sc_ioex_name),
	    "%s pciio", sc->sc_dev.dv_xname);
	sc->sc_ioex = extent_create(sc->sc_ioex_name, 0x00000000, 0xffffffff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT | EX_FILLED);
	snprintf(sc->sc_memex_name, sizeof(sc->sc_memex_name),
	    "%s pcimem", sc->sc_dev.dv_xname);
	sc->sc_memex = extent_create(sc->sc_memex_name, 0x00000000, 0xffffffff,
	    M_DEVBUF, NULL, 0, EX_NOWAIT | EX_FILLED);

	sc->sc_pc.pc_conf_v = sc;
	sc->sc_pc.pc_attach_hook = imxpcibr_attach_hook;
	sc->sc_pc.pc_bus_maxdevs = imxpcibr_bus_maxdevs;
	sc->sc_pc.pc_make_tag = imxpcibr_make_tag;
	sc->sc_pc.pc_decompose_tag = imxpcibr_decompose_tag;
	sc->sc_pc.pc_conf_size = imxpcibr_conf_size;
	sc->sc_pc.pc_conf_read = imxpcibr_conf_read;
	sc->sc_pc.pc_conf_write = imxpcibr_conf_write;

	sc->sc_pc.pc_intr_v = sc;
	sc->sc_pc.pc_intr_map = imxpcibr_intr_map;
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
imxpcibr_attach_hook(struct device *parent, struct device *self,
    struct pcibus_attach_args *pba)
{
}

int
imxpcibr_bus_maxdevs(void *sc, int busno) {
	return(32);
}

#define BUS_SHIFT 16
#define DEVICE_SHIFT 11
#define FNC_SHIFT 8

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

pcireg_t
imxpcibr_conf_read(void *sc, pcitag_t tag, int offset)
{
	return 0;
}

void
imxpcibr_conf_write(void *sc, pcitag_t tag, int offset, pcireg_t data)
{
}

int
imxpcibr_intr_map(struct pci_attach_args *pa,
    pci_intr_handle_t *ihp)
{
	return 0;
}

const char *
imxpcibr_intr_string(void *sc, pci_intr_handle_t ih)
{
	static char str[16];

	snprintf(str, sizeof str, "irq %ld", ih);
	return(str);
}

void *
imxpcibr_intr_establish(void *sc, pci_intr_handle_t ih, int level,
    int (*func)(void *), void *arg, const char *name)
{
	return NULL;
}

void
imxpcibr_intr_disestablish(void *sc, void *cookie)
{
	/* do something */
}
