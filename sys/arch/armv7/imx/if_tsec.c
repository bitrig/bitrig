/*	$OpenBSD: if_tsec.c,v 1.34 2014/12/22 02:26:53 tedu Exp $	*/

/*
 * Copyright (c) 2008 Mark Kettenis
 * Copyright (c) 2015 Patrick Wildt <patrick@blueri.se>
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
 * Driver for the TSEC interface on the MPC8349E processors.
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/timeout.h>

#include <machine/bus.h>
#include <machine/fdt.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_types.h>

#include <arm/cpufunc.h>
#include <armv7/armv7/armv7var.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <netinet/in.h>
#include <netinet/if_ether.h>

/*
 * TSEC registers.
 */

#define TSEC_IEVENT		0x010
#define  TSEC_IEVENT_BABR	0x80000000
#define  TSEC_IEVENT_RXC	0x40000000
#define  TSEC_IEVENT_BSY	0x20000000
#define  TSEC_IEVENT_EBERR	0x10000000
#define  TSEC_IEVENT_MSRO	0x04000000
#define  TSEC_IEVENT_GTSC	0x02000000
#define  TSEC_IEVENT_BABT	0x01000000
#define  TSEC_IEVENT_TXC	0x00800000
#define  TSEC_IEVENT_TXE	0x00400000
#define  TSEC_IEVENT_TXB	0x00200000
#define  TSEC_IEVENT_TXF	0x00100000
#define  TSEC_IEVENT_LC		0x00040000
#define  TSEC_IEVENT_CRL	0x00020000
#define  TSEC_IEVENT_DXA	TSEC_IEVENT_CRL
#define  TSEC_IEVENT_XFUN	0x00010000
#define  TSEC_IEVENT_RXB	0x00008000
#define  TSEC_IEVENT_MMRD	0x00000400
#define  TSEC_IEVENT_MMWR	0x00000200
#define  TSEC_IEVENT_GRSC	0x00000100
#define  TSEC_IEVENT_RXF	0x00000080
#define  TSEC_IEVENT_FMT	"\020" "\040BABR" "\037RXC" "\036BSY" \
	"\035EBERR" "\033MSRO" "\032GTSC" "\031BABT" "\030TXC" "\027TXE" \
	"\026TXB" "\025TXF" "\023LC" "\022CRL/XDA" "\021XFUN" "\020RXB" \
	"\013MMRD" "\012MMRW" "\011GRSC" "\010RXF"
#define TSEC_IMASK		0x014
#define  TSEC_IMASK_BREN	0x80000000
#define  TSEC_IMASK_RXCEN	0x40000000
#define  TSEC_IMASK_BSYEN	0x20000000
#define  TSEC_IMASK_EBERREN	0x10000000
#define  TSEC_IMASK_MSROEN	0x04000000
#define  TSEC_IMASK_GTSCEN	0x02000000
#define  TSEC_IMASK_BTEN	0x01000000
#define  TSEC_IMASK_TXCEN	0x00800000
#define  TSEC_IMASK_TXEEN	0x00400000
#define  TSEC_IMASK_TXBEN	0x00200000
#define  TSEC_IMASK_TXFEN	0x00100000
#define  TSEC_IMASK_LCEN	0x00040000
#define  TSEC_IMASK_CRLEN	0x00020000
#define  TSEC_IMASK_DXAEN	TSEC_IMASK_CRLEN
#define  TSEC_IMASK_XFUNEN	0x00010000
#define  TSEC_IMASK_RXBEN	0x00008000
#define  TSEC_IMASK_MAGEN	0x00000800
#define  TSEC_IMASK_MMRD	0x00000400
#define  TSEC_IMASK_MMWR	0x00000200
#define  TSEC_IMASK_GRSCEN	0x00000100
#define  TSEC_IMASK_RXFEN	0x00000080
#define  TSEC_IMASK_FGPIEN	0x00000010
#define  TSEC_IMASK_FIREN	0x00000008
#define  TSEC_IMASK_FIQEN	0x00000004
#define  TSEC_IMASK_DPEEN	0x00000002
#define  TSEC_IMASK_PERREN	0x00000001
#define TSEC_EDIS		0x018
#define TSEC_ECNTRL		0x020
#define  TSEC_ECNTRL_FIFM	0x00008000
#define  TSEC_ECNTRL_INIT_SETTINGS	0x00001000
#define  TSEC_ECNTRL_TBI_MODE	0x00000020
#define  TSEC_ECNTRL_REDUCED_MODE	0x00000010
#define  TSEC_ECNTRL_R100M	0x00000008 /* RGMII 100 mode */
#define  TSEC_ECNTRL_REDUCED_MII_MODE	0x00000004
#define  TSEC_ECNTRL_SGMII_MODE	0x00000002

#define TSEC_MINFLR		0x024
#define TSEC_PTV		0x028
#define TSEC_DMACTRL		0x02c
#define  TSEC_DMACTRL_LE	0x00008000 /* Little-Endian */
#define  TSEC_DMACTRL_TDSEN	0x00000080
#define  TSEC_DMACTRL_TBDSEN	0x00000040
#define  TSEC_DMACTRL_GRS	0x00000010 /* Graceful receive stop */
#define  TSEC_DMACTRL_GTS	0x00000008 /* Graceful transmit stop */
#define  TSEC_DMACTRL_WWR	0x00000002
#define  TSEC_DMACTRL_WOP	0x00000001
#define TSEC_TBIPA		0x030

#define TSEC_FIFO_TX_THR	0x08c
#define TSEC_FIFO_TX_STARVE	0x098
#define TSEC_FIFO_TX_STARVE_SHUTOFF	0x09c

#define TSEC_TCTRL		0x100
#define TSEC_TSTAT		0x104
#define  TSEC_TSTAT_THLT	0x80000000
#define TSEC_TXIC		0x110
#define  TSEC_TXIC_ICEN		0x80000000
#define  TSEC_TXIC_ICCS		0x40000000
#define  TSEC_TXIC_ICFT(x)	(((x) & 0xff) << 21)
#define  TSEC_TXIC_ICTT(x)	((x) & 0xffff)
#define TSEC_TBPTR		0x184
#define TSEC_TBASE		0x204

#define TSEC_RCTRL		0x300
#define  TSEC_RCTRL_PAL(x)	(((x) & 0x1f) << 16)
#define  TSEC_RCTRL_PROM	0x00000008
#define TSEC_RSTAT		0x304
#define  TSEC_RSTAT_QHLT	0x00800000
#define TSEC_RXIC		0x310
#define TSEC_MRBLR		0x340
#define TSEC_RBPTR		0x384
#define TSEC_RBASE		0x404

#define TSEC_MACCFG1		0x500
#define  TSEC_MACCFG1_TXEN	0x00000001
#define  TSEC_MACCFG1_RXEN	0x00000004
#define  TSEC_MACCFG1_RESET	0x80000000
#define TSEC_MACCFG2		0x504
#define  TSEC_MACCFG2_IF_MODE	0x00000300 /* I/F mode */
#define  TSEC_MACCFG2_IF_MII	0x00000100 /* I/F mode */
#define  TSEC_MACCFG2_IF_GMII	0x00000200 /* I/F mode */
#define  TSEC_MACCFG2_PAD	0x00000004
#define  TSEC_MACCFG2_CRC	0x00000002
#define  TSEC_MACCFG2_FDX	0x00000001 /* Full duplex */
#define TSEC_MAXFRM		0x510
#define TSEC_MIIMCFG		0x520
#define  TSEC_MIIMCFG_RESET	0x80000000 /* Reset */
#define TSEC_MIIMCOM		0x524
#define  TSEC_MIIMCOM_READ	0x00000001 /* Read cycle */
#define  TSEC_MIIMCOM_SCAN	0x00000002 /* Scan cycle */
#define TSEC_MIIMADD		0x528
#define TSEC_MIIMCON		0x52c
#define TSEC_MIIMSTAT		0x530
#define TSEC_MIIMIND		0x534
#define  TSEC_MIIMIND_BUSY	0x00000001 /* Busy */
#define  TSEC_MIIMIND_SCAN	0x00000002 /* Scan in progress */
#define  TSEC_MIIMIND_NOTVALID	0x00000004 /* Not valid */
#define TSEC_MACSTNADDR1 	0x540
#define TSEC_MACSTNADDR2 	0x544
#define TSEC_IADDR0		0x800
#define TSEC_IADDR1		0x804
#define TSEC_IADDR2		0x818
#define TSEC_IADDR3		0x81c
#define TSEC_IADDR4		0x810
#define TSEC_IADDR5		0x814
#define TSEC_IADDR6		0x818
#define TSEC_IADDR7		0x81c
#define TSEC_GADDR0		0x880
#define TSEC_GADDR1		0x884
#define TSEC_GADDR2		0x888
#define TSEC_GADDR3		0x88c
#define TSEC_GADDR4		0x890
#define TSEC_GADDR5		0x894
#define TSEC_GADDR6		0x898
#define TSEC_GADDR7		0x89c

#define TSEC_ATTR		0xbf8
#define  TSEC_ATTR_RDSEN	0x00000080
#define  TSEC_ATTR_RBDSEN	0x00000040
#define TSEC_ATTRELI		0xbfc

#define TSEC_RQPRM0		0xc00
#define TSEC_RQPRM1		0xc04
#define TSEC_RQPRM2		0xc08
#define TSEC_RQPRM3		0xc0c
#define TSEC_RQPRM4		0xc10
#define TSEC_RQPRM5		0xc14
#define TSEC_RQPRM6		0xc18
#define TSEC_RQPRM7		0xc1c
#define TSEC_RFBPTR0		0xc44
#define TSEC_RFBPTR1		0xc4c
#define TSEC_RFBPTR2		0xc54
#define TSEC_RFBPTR3		0xc5c
#define TSEC_RFBPTR4		0xc64
#define TSEC_RFBPTR5		0xc6c
#define TSEC_RFBPTR6		0xc74
#define TSEC_RFBPTR7		0xc7c

#define TSEC_TMR_PEVENT		0xe0c
#define TSEC_TMR_PEMASK		0xe10
#define TSEC_TMR_PESTAT		0xe14

#define TSEC_ISRG0		0xeb0
#define TSEC_ISRG1		0xeb4

#define TSEC_RXIC0		0xed0
#define TSEC_RXIC1		0xed4
#define TSEC_RXIC2		0xed8
#define TSEC_RXIC3		0xedc
#define TSEC_RXIC4		0xee0
#define TSEC_RXIC5		0xee4
#define TSEC_RXIC6		0xee8
#define TSEC_RXIC7		0xeec
#define TSEC_TXIC0		0xf10
#define TSEC_TXIC1		0xf14
#define TSEC_TXIC2		0xf18
#define TSEC_TXIC3		0xf1c
#define TSEC_TXIC4		0xf20
#define TSEC_TXIC5		0xf24
#define TSEC_TXIC6		0xf28
#define TSEC_TXIC7		0xf2c

#define TSEC_IEVENTG1		0x4010

/*
 * TSEC descriptors.
 */

struct tsec_desc {
	uint16_t td_status;
	uint16_t td_len;
	uint32_t td_addr;
};

/* Tx status bits. */
#define TSEC_TX_TXTRUNC		0x0001 /* TX truncation */
#define TSEC_TX_UN		0x0002 /* Underrun */
#define TSEC_TX_RC		0x003c /* Retry count */
#define TSEC_TX_RL		0x0040 /* Retransmission limit */
#define TSEC_TX_HFE		0x0080 /* Huge frame enable/late collision */
#define TSEC_TX_LC		TSEC_TX_HFE
#define TSEC_TX_TO1		0x0100 /* Transmit software ownership bit */
#define TSEC_TX_DEF		0x0200 /* Defer indication */
#define TSEC_TX_TC		0x0400 /* Tx CRC */
#define TSEC_TX_L		0x0800 /* Last in frame */
#define TSEC_TX_I		0x1000 /* Interrupt */
#define TSEC_TX_W		0x2000 /* Wrap */
#define TSEC_TX_PAD		0x4000 /* PAD/CRC */
#define TSEC_TX_R		0x8000 /* Ready */

/* Rx status bits */
#define TSEC_RX_TR		0x0001 /* Truncation */
#define TSEC_RX_OV		0x0002 /* Overrun */
#define TSEC_RX_CR		0x0004 /* Rx CRC error */
#define TSEC_RX_SH		0x0008 /* Short frame */
#define TSEC_RX_NO		0x0010 /* Rx non-octet aligned frame */
#define TSEC_RX_LG		0x0020 /* Rx framelength violation */
#define TSEC_RX_MC		0x0040 /* Multicast */
#define TSEC_RX_BC		0x0080 /* Broadcast */
#define TSEC_RX_M		0x0100 /* Miss */
#define TSEC_RX_F		0x0400 /* First in frame */
#define TSEC_RX_L		0x0800 /* Last in frame */
#define TSEC_RX_I		TSEC_TX_I
#define TSEC_RX_W		TSEC_TX_W
#define TSEC_RX_RO1		0x4000 /* Receive software ownership bit */
#define TSEC_RX_E		0x8000 /* Empty */

struct tsec_buf {
	bus_dmamap_t	tb_map;
	struct mbuf	*tb_m;
};

#define TSEC_NTXDESC	256
#define TSEC_NTXSEGS	16

#define TSEC_NRXDESC	256

struct tsec_dmamem {
	bus_dmamap_t		tdm_map;
	bus_dma_segment_t	tdm_seg;
	size_t			tdm_size;
	caddr_t			tdm_kva;
};
#define TSEC_DMA_MAP(_tdm)	((_tdm)->tdm_map)
#define TSEC_DMA_LEN(_tdm)	((_tdm)->tdm_size)
#define TSEC_DMA_DVA(_tdm)	((_tdm)->tdm_map->dm_segs[0].ds_addr)
#define TSEC_DMA_KVA(_tdm)	((void *)(_tdm)->tdm_kva)

struct tsec_softc {
	struct device		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_space_handle_t	sc_mii_ioh;
	bus_dma_tag_t		sc_dmat;
	void			*sc_node;
	uint32_t		sc_tbi;

	struct arpcom		sc_ac;
#define sc_lladdr	sc_ac.ac_enaddr
	struct mii_data		sc_mii;
#define sc_media	sc_mii.mii_media

	struct tsec_dmamem	*sc_txring;
	struct tsec_buf		*sc_txbuf;
	struct tsec_desc	*sc_txdesc;
	int			sc_tx_prod;
	int			sc_tx_cnt;
	int			sc_tx_cons;

	struct tsec_dmamem	*sc_rxring;
	struct tsec_buf		*sc_rxbuf;
	struct tsec_desc	*sc_rxdesc;
	int			sc_rx_prod;
	struct if_rxring	sc_rx_ring;
	int			sc_rx_cons;

	struct timeout		sc_tick;
};

#define DEVNAME(_s)	((_s)->sc_dev.dv_xname)

int	tsec_match(struct device *, void *, void *);
void	tsec_attach(struct device *, struct device *, void *);

struct cfattach tsec_ca = {
	sizeof(struct tsec_softc), tsec_match, tsec_attach
};

struct cfdriver tsec_cd = {
	NULL, "tsec", DV_IFNET
};

int	tsec_find_phy(int, int);

void tsec_get_hwaddr(struct tsec_softc *, uint8_t *);

uint32_t tsec_read(struct tsec_softc *, bus_addr_t);
void	tsec_write(struct tsec_softc *, bus_addr_t, uint32_t);
uint32_t tsec_mii_read(struct tsec_softc *, bus_addr_t);
void	tsec_mii_write(struct tsec_softc *, bus_addr_t, uint32_t);

int	tsec_ioctl(struct ifnet *, u_long, caddr_t);
void	tsec_start(struct ifnet *);
void	tsec_watchdog(struct ifnet *);

int	tsec_media_change(struct ifnet *);
void	tsec_media_status(struct ifnet *, struct ifmediareq *);

int	tsec_mii_readreg(struct device *, int, int);
void	tsec_mii_writereg(struct device *, int, int, int);
void	tsec_mii_statchg(struct device *);

void	tsec_lladdr_write(struct tsec_softc *);

void	tsec_tick(void *);

int	tsec_txintr(void *);
int	tsec_rxintr(void *);
int	tsec_errintr(void *);

void	tsec_tx_proc(struct tsec_softc *);
void	tsec_rx_proc(struct tsec_softc *);

void	tsec_up(struct tsec_softc *);
void	tsec_down(struct tsec_softc *);
void	tsec_iff(struct tsec_softc *);
int	tsec_encap(struct tsec_softc *, struct mbuf *, int *);

void	tsec_reset(struct tsec_softc *);
void	tsec_stop_dma(struct tsec_softc *);

struct tsec_dmamem *
	tsec_dmamem_alloc(struct tsec_softc *, bus_size_t, bus_size_t);
void	tsec_dmamem_free(struct tsec_softc *, struct tsec_dmamem *);
struct mbuf *tsec_alloc_mbuf(struct tsec_softc *, bus_dmamap_t);
void	tsec_fill_rx_ring(struct tsec_softc *);

/*
 * The MPC8349E processor has two TSECs but only one external
 * management interface to control external PHYs.  The registers
 * controlling the management interface are part of TSEC1. So to
 * control a PHY attached to TSEC2, one needs to access TSEC1's
 * registers.  To deal with this, the first TSEC that attaches maps
 * the register space for both TSEC1 and TSEC2 and stores the bus
 * space tag and bus space handle in these global variables.  We use
 * these to create subregions for each individual interface and the
 * management interface.
 */
bus_space_tag_t		tsec_iot;
bus_space_handle_t	tsec_ioh;

int
tsec_match(struct device *parent, void *cfdata, void *aux)
{
	struct armv7_attach_args *aa = aux;

	//if (OF_getprop(oa->oa_node, "device_type", buf, sizeof(buf)) <= 0 ||
	//    strcmp(buf, "network") != 0)
	//	return (0);

	//if (OF_getprop(oa->oa_node, "compatible", buf, sizeof(buf)) <= 0 ||
	//    strcmp(buf, "gianfar") != 0)
	//	return (0);

	if (fdt_node_compatible("fsl,etsec2", aa->aa_node))
		return 1;

	return 0;
}

void
tsec_attach(struct device *parent, struct device *self, void *aux)
{
	struct tsec_softc *sc = (void *)self;
	struct armv7_attach_args *aa = aux;
	struct fdt_memory mem;
	struct ifnet *ifp;
	void *node;
	int phy = 0, tbi = 0, n;

	if (aa->aa_node == NULL)
		panic("%s: no fdt node", __func__);

	sc->sc_dmat = aa->aa_dmat;
	sc->sc_node = aa->aa_node;
	sc->sc_iot = aa->aa_iot;
	node = fdt_child_node(sc->sc_node);
	if (fdt_get_memory_address(node, 0, &mem))
		panic("%s: could not extract memory data from FDT", __func__);

	//* Ethernet Controller registers. */
	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh)) {
		printf("%s: bus_space_map failed\n", __func__);
		return;
	}

	if (!fdt_node_property_int(sc->sc_node, "phy-handle", &phy))
		panic("%s: could not extract phy handle from FDT", __func__);

	node = fdt_find_node_by_phandle(NULL, phy);
	if (node == NULL)
		panic("%s: could not get phy node\n");

	if (!fdt_node_property_int(node, "reg", &phy))
		panic("%s: could not extract phy addr from FDT", __func__);

	if (fdt_node_property_int(sc->sc_node, "tbi-handle", &tbi) > 0) {
		node = fdt_find_node_by_phandle(NULL, tbi);
		if (node == NULL)
			panic("%s: could not get tbi node\n");

		if (!fdt_node_property_int(node, "reg", &sc->sc_tbi))
			panic("%s: could not extract tbi addr from FDT", __func__);
	}

#if 0
	if (OF_getprop(oa->oa_node, "phy-handle", &phy,
	    sizeof(phy)) == sizeof(phy)) {
		int node, reg;

		node = tsec_find_phy(OF_peer(0), phy);
		if (node == -1 || OF_getprop(node, "reg", &reg,
		    sizeof(reg)) != sizeof(reg)) {
			printf(": can't find PHY\n");
			return;
		}

		oa->oa_phy = reg;
	}
#endif

	/* MII Management registers. */
	if (fdt_get_memory_address(fdt_parent_node(node), 0, &mem))
		panic("%s: could not extract phy memory data from FDT", __func__);

	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_mii_ioh)) {
		printf("%s: bus_space_map failed\n", __func__);
		return;
	}

	/* Init timer */
	//callout_init(&sc->tsec_callout, 1);

	node = fdt_child_node(sc->sc_node);
	if (node == NULL)
	    panic("%s: could not extract interrupt data from FDT", __func__);

	tsec_get_hwaddr(sc, sc->sc_lladdr);
	printf(": address %s\n", ether_sprintf(sc->sc_lladdr));

	timeout_set(&sc->sc_tick, tsec_tick, sc);

	ifp = &sc->sc_ac.ac_if;
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = tsec_ioctl;
	ifp->if_start = tsec_start;
	ifp->if_watchdog = tsec_watchdog;
	IFQ_SET_MAXLEN(&ifp->if_snd, TSEC_NTXDESC - 1);
	IFQ_SET_READY(&ifp->if_snd);
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);

	ifp->if_capabilities = IFCAP_VLAN_MTU;

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = tsec_mii_readreg;
	sc->sc_mii.mii_writereg = tsec_mii_writereg;
	sc->sc_mii.mii_statchg = tsec_mii_statchg;

	ifmedia_init(&sc->sc_media, 0, tsec_media_change, tsec_media_status);

	tsec_reset(sc);

	/* HW init */
	tsec_write(sc, TSEC_ECNTRL, TSEC_ECNTRL_INIT_SETTINGS);
	tsec_write(sc, TSEC_ATTRELI, 0);
	tsec_write(sc, TSEC_ATTR, 0);
	tsec_write(sc, TSEC_FIFO_TX_THR, 0x100);
	tsec_write(sc, TSEC_FIFO_TX_STARVE, 0x40);
	tsec_write(sc, TSEC_FIFO_TX_STARVE_SHUTOFF, 0x80);

	/* Reset management. */
	tsec_write(sc, TSEC_MIIMCFG, TSEC_MIIMCFG_RESET);
	tsec_write(sc, TSEC_MIIMCFG, 0x00000003);
	for (n = 0; n < 100; n++) {
		if ((tsec_read(sc, TSEC_MIIMIND) & TSEC_MIIMIND_BUSY) == 0)
			break;
	}

	mii_attach(self, &sc->sc_mii, 0xffffffff, phy,
	    MII_OFFSET_ANY, 0);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		printf("%s: no PHY found!\n", sc->sc_dev.dv_xname);
		ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_MANUAL, 0, NULL);
		ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_MANUAL);
	} else
		ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_AUTO);

	if_attach(ifp);
	ether_ifattach(ifp);

	arm_intr_establish_fdt_idx(node, 0, IPL_NET, tsec_txintr,
	    sc, sc->sc_dev.dv_xname);
	arm_intr_establish_fdt_idx(node, 1, IPL_NET, tsec_rxintr,
	    sc, sc->sc_dev.dv_xname);
	arm_intr_establish_fdt_idx(node, 2, IPL_NET, tsec_errintr,
	    sc, sc->sc_dev.dv_xname);
}

#if 0
int
tsec_find_phy(int node, int phy)
{
	int child, handle;

	if (OF_getprop(node, "linux,phandle", &handle,
	    sizeof(handle)) == sizeof(handle) && phy == handle)
		return (node);

	for (child = OF_child(node); child != 0; child = OF_peer(child)) {
		node = tsec_find_phy(child, phy);
		if (node != -1)
			return node;
	}

	return (-1);
}
#endif

uint32_t
tsec_read(struct tsec_softc *sc, bus_addr_t addr)
{
	return (betoh32(bus_space_read_4(sc->sc_iot, sc->sc_ioh, addr)));
}

void
tsec_write(struct tsec_softc *sc, bus_addr_t addr, uint32_t data)
{
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, addr, htobe32(data));
}

uint32_t
tsec_mii_read(struct tsec_softc *sc, bus_addr_t addr)
{
	return (betoh32(bus_space_read_4(sc->sc_iot, sc->sc_mii_ioh, addr)));
}

void
tsec_mii_write(struct tsec_softc *sc, bus_addr_t addr, uint32_t data)
{
	bus_space_write_4(sc->sc_iot, sc->sc_mii_ioh, addr, htobe32(data));
}

void
tsec_lladdr_write(struct tsec_softc *sc)
{
	uint32_t addr1, addr2;

	addr1 = sc->sc_lladdr[5] << 24 | sc->sc_lladdr[4] << 16 |
	    sc->sc_lladdr[3] << 8 | sc->sc_lladdr[2];
	addr2 = sc->sc_lladdr[1] << 24 | sc->sc_lladdr[0] << 16;
	tsec_write(sc, TSEC_MACSTNADDR1, addr1);
	tsec_write(sc, TSEC_MACSTNADDR2, addr2);
}

void
tsec_start(struct ifnet *ifp)
{
	struct tsec_softc *sc = ifp->if_softc;
	struct mbuf *m;
	int idx;

	if (!(ifp->if_flags & IFF_RUNNING))
		return;
	if (ifp->if_flags & IFF_OACTIVE)
		return;
	if (IFQ_IS_EMPTY(&ifp->if_snd))
		return;

	idx = sc->sc_tx_prod;
	while ((betoh16(sc->sc_txdesc[idx].td_status) & TSEC_TX_TO1) == 0) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		if (tsec_encap(sc, m, &idx)) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		/* Now we are committed to transmit the packet. */
		IFQ_DEQUEUE(&ifp->if_snd, m);

#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m, BPF_DIRECTION_OUT);
#endif
	}

	if (sc->sc_tx_prod != idx) {
		sc->sc_tx_prod = idx;

		/* Set a timeout in case the chip goes out to lunch. */
		ifp->if_timer = 5;
	}
}

int
tsec_ioctl(struct ifnet *ifp, u_long cmd, caddr_t addr)
{
	struct tsec_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)addr;
	struct ifreq *ifr = (struct ifreq *)addr;
	int error = 0, s;

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		if (ifa->ifa_addr->sa_family == AF_INET)
			arp_ifinit(&sc->sc_ac, ifa);
		/* FALLTHROUGH */
	case SIOCSIFFLAGS:
		if (ifp->if_flags & IFF_UP) {
			if (ifp->if_flags & IFF_RUNNING)
				error = ENETRESET;
			else
				tsec_up(sc);
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				tsec_down(sc);
		}
		break;

	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;

	case SIOCGIFRXR:
		error = if_rxr_ioctl((struct if_rxrinfo *)ifr->ifr_data,
		    NULL, MCLBYTES, &sc->sc_rx_ring);
		break;

	default:
		error = ether_ioctl(ifp, &sc->sc_ac, cmd, addr);
		break;
	}

	if (error == ENETRESET) {
		if (ifp->if_flags & IFF_RUNNING)
			tsec_iff(sc);
		error = 0;
	}

	splx(s);
	return (error);
}

void
tsec_watchdog(struct ifnet *ifp)
{
	printf("%s\n", __func__);
}

int
tsec_media_change(struct ifnet *ifp)
{
	struct tsec_softc *sc = ifp->if_softc;

	if (LIST_FIRST(&sc->sc_mii.mii_phys))
		mii_mediachg(&sc->sc_mii);

	return (0);
}

void
tsec_media_status(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct tsec_softc *sc = ifp->if_softc;

	if (LIST_FIRST(&sc->sc_mii.mii_phys)) {
		mii_pollstat(&sc->sc_mii);
		ifmr->ifm_active = sc->sc_mii.mii_media_active;
		ifmr->ifm_status = sc->sc_mii.mii_media_status;
	}
}

int
tsec_mii_readreg(struct device *self, int phy, int reg)
{
	struct tsec_softc *sc = (void *)self;
	uint32_t v;
	int n;

	tsec_mii_write(sc, TSEC_MIIMADD, (phy << 8) | reg);
	tsec_mii_write(sc, TSEC_MIIMCOM, 0);
	tsec_mii_write(sc, TSEC_MIIMCOM, TSEC_MIIMCOM_READ);
	for (n = 0; n < 100; n++) {
		v = tsec_mii_read(sc, TSEC_MIIMIND);
		if ((v & (TSEC_MIIMIND_NOTVALID | TSEC_MIIMIND_BUSY)) == 0)
			return (tsec_mii_read(sc, TSEC_MIIMSTAT));
		delay(10);
	}

	printf("%s: mii_read timeout\n", sc->sc_dev.dv_xname);
	return (0);
}

void
tsec_mii_writereg(struct device *self, int phy, int reg, int val)
{
	struct tsec_softc *sc = (void *)self;
	uint32_t v;
	int n;

	tsec_mii_write(sc, TSEC_MIIMADD, (phy << 8) | reg);
	tsec_mii_write(sc, TSEC_MIIMCON, val);
	for (n = 0; n < 100; n++) {
		v = tsec_mii_read(sc, TSEC_MIIMIND);
		if ((v & TSEC_MIIMIND_BUSY) == 0)
			return;
		delay(10);
	}

	printf("%s: mii_write timeout\n", sc->sc_dev.dv_xname);
}

void
tsec_mii_statchg(struct device *self)
{
	struct tsec_softc *sc = (void *)self;
	uint32_t maccfg2, ecntrl;

	ecntrl = tsec_read(sc, TSEC_ECNTRL);
	maccfg2 = tsec_read(sc, TSEC_MACCFG2);
	maccfg2 &= ~TSEC_MACCFG2_IF_MODE;

	switch (IFM_SUBTYPE(sc->sc_mii.mii_media_active)) {
	case IFM_1000_SX:
	case IFM_1000_LX:
	case IFM_1000_CX:
	case IFM_1000_T:
		ecntrl &= TSEC_ECNTRL_R100M;
		maccfg2 |= TSEC_MACCFG2_IF_GMII;
		break;
	case IFM_100_TX:
		ecntrl |= TSEC_ECNTRL_R100M;
		maccfg2 |= TSEC_MACCFG2_IF_MII;
		break;
	case IFM_10_T:
		ecntrl &= ~TSEC_ECNTRL_R100M;
		maccfg2 |= TSEC_MACCFG2_IF_MII;
		break;
	}

	if ((sc->sc_mii.mii_media_active & IFM_GMASK) == IFM_FDX)
		maccfg2 |= TSEC_MACCFG2_FDX;
	else
		maccfg2 &= ~TSEC_MACCFG2_FDX;

	tsec_write(sc, TSEC_MACCFG2, maccfg2);
	tsec_write(sc, TSEC_ECNTRL, ecntrl);
}

void
tsec_tick(void *arg)
{
	struct tsec_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	timeout_add_sec(&sc->sc_tick, 1);
}

int
tsec_txintr(void *arg)
{
	struct tsec_softc *sc = arg;
	uint32_t ievent;

	ievent = tsec_read(sc, TSEC_IEVENT);
	if ((ievent & (TSEC_IEVENT_TXC | TSEC_IEVENT_TXE |
	    TSEC_IEVENT_TXB | TSEC_IEVENT_TXF)) == 0)
		printf("%s: tx %b\n", DEVNAME(sc), ievent, TSEC_IEVENT_FMT);
	ievent &= (TSEC_IEVENT_TXC | TSEC_IEVENT_TXE |
	    TSEC_IEVENT_TXB | TSEC_IEVENT_TXF);
	tsec_write(sc, TSEC_IEVENT, ievent);

	tsec_tx_proc(sc);

	return (1);
}

int
tsec_rxintr(void *arg)
{
	struct tsec_softc *sc = arg;
	uint32_t ievent;

	ievent = tsec_read(sc, TSEC_IEVENT);
	if ((ievent & (TSEC_IEVENT_RXB | TSEC_IEVENT_RXF)) == 0)
		printf("%s: rx %b\n", DEVNAME(sc), ievent, TSEC_IEVENT_FMT);
	ievent &= (TSEC_IEVENT_RXB | TSEC_IEVENT_RXF);
	tsec_write(sc, TSEC_IEVENT, ievent);

	tsec_rx_proc(sc);

	return (1);
}

int
tsec_errintr(void *arg)
{
	struct tsec_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	uint32_t ievent;

	ievent = tsec_read(sc, TSEC_IEVENT);
	if ((ievent & TSEC_IEVENT_BSY) == 0)
		printf("%s: err %b\n", DEVNAME(sc), ievent, TSEC_IEVENT_FMT);
	ievent &= TSEC_IEVENT_BSY;
	tsec_write(sc, TSEC_IEVENT, ievent);

	if (ievent & TSEC_IEVENT_BSY) {
		/*
		 * We ran out of buffers and dropped one (or more)
		 * packets.  We must clear RSTAT[QHLT] after
		 * processing the ring to get things started again.
		 */
		tsec_rx_proc(sc);
		tsec_write(sc, TSEC_RSTAT, TSEC_RSTAT_QHLT);
		ifp->if_ierrors++;
	}

	return (1);
}

void
tsec_tx_proc(struct tsec_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	struct tsec_desc *txd;
	struct tsec_buf *txb;
	int idx;

	bus_dmamap_sync(sc->sc_dmat, TSEC_DMA_MAP(sc->sc_txring), 0,
	    TSEC_DMA_LEN(sc->sc_txring),
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	while (sc->sc_tx_cnt > 0) {
		idx = sc->sc_tx_cons;
		KASSERT(idx < TSEC_NTXDESC);

		txd = &sc->sc_txdesc[idx];
		if (betoh16(txd->td_status) & TSEC_TX_R)
			break;

		txb = &sc->sc_txbuf[idx];
		if (txb->tb_m) {
			bus_dmamap_sync(sc->sc_dmat, txb->tb_map, 0,
			    txb->tb_map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, txb->tb_map);

			m_freem(txb->tb_m);
			txb->tb_m = NULL;
			ifp->if_opackets++;
		}

		ifp->if_flags &= ~IFF_OACTIVE;

		sc->sc_tx_cnt--;

		if (betoh16(txd->td_status) & TSEC_TX_W)
			sc->sc_tx_cons = 0;
		else
			sc->sc_tx_cons++;

		cpu_drain_writebuf();
		//__asm volatile("eieio" ::: "memory");
		txd->td_status &= htobe16(TSEC_TX_W);
	}

	if (sc->sc_tx_cnt == 0)
		ifp->if_timer = 0;
}

void
tsec_rx_proc(struct tsec_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	struct tsec_desc *rxd;
	struct tsec_buf *rxb;
	struct mbuf_list ml = MBUF_LIST_INITIALIZER();
	struct mbuf *m;
	int idx, len;

	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return;

	bus_dmamap_sync(sc->sc_dmat, TSEC_DMA_MAP(sc->sc_rxring), 0,
	    TSEC_DMA_LEN(sc->sc_rxring),
	    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

	while (if_rxr_inuse(&sc->sc_rx_ring) > 0) {
		idx = sc->sc_rx_cons;
		KASSERT(idx < TSEC_NRXDESC);

		rxd = &sc->sc_rxdesc[idx];
		if (betoh16(rxd->td_status) & TSEC_RX_E)
			break;

		len = betoh16(rxd->td_len);
		rxb = &sc->sc_rxbuf[idx];
		KASSERT(rxb->tb_m);

		bus_dmamap_sync(sc->sc_dmat, rxb->tb_map, 0,
		    len, BUS_DMASYNC_POSTREAD);
		bus_dmamap_unload(sc->sc_dmat, rxb->tb_map);

		/* Strip off CRC. */
		len -= ETHER_CRC_LEN;
		KASSERT(len > 0);

		m = rxb->tb_m;
		rxb->tb_m = NULL;
		m->m_pkthdr.len = m->m_len = len;

		ifp->if_ipackets++;

		ml_enqueue(&ml, m);

		if_rxr_put(&sc->sc_rx_ring, 1);
		if (betoh16(rxd->td_status) & TSEC_RX_W)
			sc->sc_rx_cons = 0;
		else
			sc->sc_rx_cons++;
	}

	tsec_fill_rx_ring(sc);

	bus_dmamap_sync(sc->sc_dmat, TSEC_DMA_MAP(sc->sc_rxring), 0,
	    TSEC_DMA_LEN(sc->sc_rxring),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	if_input(ifp, &ml);
}

#define	MII_TBICON		0x11
#define	TBICON_CLK_SELECT	0x0020
static void
tsec_configure_serdes(struct tsec_softc *sc)
{
	if (tsec_mii_readreg(&sc->sc_dev, sc->sc_tbi, MII_BMSR) & BMSR_LINK)
		return;

	tsec_mii_writereg(&sc->sc_dev, sc->sc_tbi, MII_TBICON,
	    TBICON_CLK_SELECT);

	tsec_mii_writereg(&sc->sc_dev, sc->sc_tbi, MII_ANAR,
	    ANAR_X_FD | ANAR_X_HD | ANAR_X_PAUSE_SYM |
	    ANAR_X_PAUSE_ASYM);

	tsec_mii_writereg(&sc->sc_dev, sc->sc_tbi, MII_BMCR,
	    BMCR_AUTOEN | BMCR_STARTNEG | BMCR_FDX |
	    BMCR_SPEED1);
}

void
tsec_up(struct tsec_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	struct tsec_desc *txd, *rxd;
	struct tsec_buf *txb, *rxb;
	uint32_t maccfg1, maccfg2, ecntrl, dmactrl;
	int i;

	/* Allocate Tx descriptor ring. */
	sc->sc_txring = tsec_dmamem_alloc(sc,
	    TSEC_NTXDESC * sizeof(struct tsec_desc), 8);
	sc->sc_txdesc = TSEC_DMA_KVA(sc->sc_txring);

	sc->sc_txbuf = malloc(sizeof(struct tsec_buf) * TSEC_NTXDESC,
	    M_DEVBUF, M_WAITOK);
	for (i = 0; i < TSEC_NTXDESC; i++) {
		txb = &sc->sc_txbuf[i];
		bus_dmamap_create(sc->sc_dmat, MCLBYTES, TSEC_NTXSEGS,
		    MCLBYTES, 0, BUS_DMA_WAITOK, &txb->tb_map);
		txb->tb_m = NULL;
	}

	/* Set wrap bit on last descriptor. */
	txd = &sc->sc_txdesc[TSEC_NTXDESC - 1];
	txd->td_status = htobe16(TSEC_TX_W);
	bus_dmamap_sync(sc->sc_dmat, TSEC_DMA_MAP(sc->sc_txring),
	    (TSEC_NTXDESC - 1) * sizeof(*txd), sizeof(*txd),
	    BUS_DMASYNC_PREWRITE);

	sc->sc_tx_prod = sc->sc_tx_cons = 0;
	sc->sc_tx_cnt = 0;

	/* Allocate Rx descriptor ring. */
	sc->sc_rxring = tsec_dmamem_alloc(sc,
	    TSEC_NRXDESC * sizeof(struct tsec_desc), 8);
	sc->sc_rxdesc = TSEC_DMA_KVA(sc->sc_rxring);

	sc->sc_rxbuf = malloc(sizeof(struct tsec_buf) * TSEC_NRXDESC,
	    M_DEVBUF, M_WAITOK);

	for (i = 0; i < TSEC_NRXDESC; i++) {
		rxb = &sc->sc_rxbuf[i];
		bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, BUS_DMA_WAITOK, &rxb->tb_map);
		rxb->tb_m = NULL;
	}

	/* Set wrap bit on last descriptor. */
	rxd = &sc->sc_rxdesc[TSEC_NRXDESC - 1];
	rxd->td_status |= htobe16(TSEC_RX_W);
	bus_dmamap_sync(sc->sc_dmat, TSEC_DMA_MAP(sc->sc_rxring),
	    0, TSEC_DMA_LEN(sc->sc_rxring), BUS_DMASYNC_PREWRITE);

	sc->sc_rx_prod = sc->sc_rx_cons = 0;

	if_rxr_init(&sc->sc_rx_ring, 2, TSEC_NRXDESC);
	tsec_fill_rx_ring(sc);

	tsec_write(sc, TSEC_MAXFRM, MCLBYTES);
	tsec_write(sc, TSEC_MRBLR, MCLBYTES);
	tsec_write(sc, TSEC_MINFLR, 0x40);

	/*
	 * Default to full-duplex MII mode, which is the mode most
	 * likely used by a directly connected integrated switch.  For
	 * a real PHY the mode will be set later, based on the
	 * parameters negotiated by the PHY.
	 */
	maccfg2 = tsec_read(sc, TSEC_MACCFG2);
	maccfg2 &= ~TSEC_MACCFG2_IF_MODE;
	maccfg2 |= TSEC_MACCFG2_IF_MII | TSEC_MACCFG2_FDX;
	tsec_write(sc, TSEC_MACCFG2, maccfg2 | TSEC_MACCFG2_PAD);

	tsec_write(sc, TSEC_IADDR0, 0);
	tsec_write(sc, TSEC_IADDR1, 0);
	tsec_write(sc, TSEC_IADDR2, 0);
	tsec_write(sc, TSEC_IADDR3, 0);
	tsec_write(sc, TSEC_IADDR4, 0);
	tsec_write(sc, TSEC_IADDR5, 0);
	tsec_write(sc, TSEC_IADDR6, 0);
	tsec_write(sc, TSEC_IADDR7, 0);
	tsec_write(sc, TSEC_GADDR0, 0);
	tsec_write(sc, TSEC_GADDR1, 0);
	tsec_write(sc, TSEC_GADDR2, 0);
	tsec_write(sc, TSEC_GADDR3, 0);
	tsec_write(sc, TSEC_GADDR4, 0);
	tsec_write(sc, TSEC_GADDR5, 0);
	tsec_write(sc, TSEC_GADDR6, 0);
	tsec_write(sc, TSEC_GADDR7, 0);

	tsec_lladdr_write(sc);

	/* TODO: interrupt coalescing */
	tsec_write(sc, TSEC_TXIC, 0);
	tsec_write(sc, TSEC_RXIC, 0);

	tsec_write(sc, TSEC_TBASE, TSEC_DMA_DVA(sc->sc_txring));
	tsec_write(sc, TSEC_RBASE, TSEC_DMA_DVA(sc->sc_rxring));

	dmactrl = tsec_read(sc, TSEC_DMACTRL);
	dmactrl |= TSEC_DMACTRL_LE;
	dmactrl |= TSEC_DMACTRL_WWR;
	dmactrl |= TSEC_DMACTRL_WOP;
	tsec_write(sc, TSEC_DMACTRL, dmactrl);

	dmactrl = tsec_read(sc, TSEC_DMACTRL);
	dmactrl &= ~(TSEC_DMACTRL_GTS | TSEC_DMACTRL_GRS);
	tsec_write(sc, TSEC_DMACTRL, dmactrl);

	tsec_write(sc, TSEC_TSTAT, TSEC_TSTAT_THLT);
	tsec_write(sc, TSEC_RSTAT, TSEC_RSTAT_QHLT);

	maccfg1 = tsec_read(sc, TSEC_MACCFG1);
	maccfg1 |= TSEC_MACCFG1_TXEN;
	maccfg1 |= TSEC_MACCFG1_RXEN;
	tsec_write(sc, TSEC_MACCFG1, maccfg1);

	/* Configure media. */
	if (LIST_FIRST(&sc->sc_mii.mii_phys))
		mii_mediachg(&sc->sc_mii);

	/* Configure SerDes if needed. */
	ecntrl = tsec_read(sc, TSEC_ECNTRL);
	if (ecntrl & TSEC_ECNTRL_SGMII_MODE)
		tsec_configure_serdes(sc);

	/* Program promiscuous mode and multicast filters. */
	tsec_iff(sc);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	tsec_write(sc, TSEC_IMASK, TSEC_IMASK_TXEEN |
	    TSEC_IMASK_TXBEN | TSEC_IMASK_TXFEN |
	    TSEC_IMASK_RXBEN | TSEC_IMASK_RXFEN | TSEC_IMASK_BSYEN);

	timeout_add_sec(&sc->sc_tick, 1);
}

void
tsec_down(struct tsec_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	struct tsec_buf *txb, *rxb;
	uint32_t maccfg1;
	int i;

	timeout_del(&sc->sc_tick);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;

	tsec_stop_dma(sc);

	maccfg1 = tsec_read(sc, TSEC_MACCFG1);
	maccfg1 &= ~TSEC_MACCFG1_TXEN;
	maccfg1 &= ~TSEC_MACCFG1_RXEN;
	tsec_write(sc, TSEC_MACCFG1, maccfg1);

	for (i = 0; i < TSEC_NTXDESC; i++) {
		txb = &sc->sc_txbuf[i];
		if (txb->tb_m) {
			bus_dmamap_sync(sc->sc_dmat, txb->tb_map, 0,
			    txb->tb_map->dm_mapsize, BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, txb->tb_map);
			m_freem(txb->tb_m);
		}
		bus_dmamap_destroy(sc->sc_dmat, txb->tb_map);
	}

	tsec_dmamem_free(sc, sc->sc_txring);
	free(sc->sc_txbuf, M_DEVBUF, 0);

	for (i = 0; i < TSEC_NRXDESC; i++) {
		rxb = &sc->sc_rxbuf[i];
		if (rxb->tb_m) {
			bus_dmamap_sync(sc->sc_dmat, rxb->tb_map, 0,
			    rxb->tb_map->dm_mapsize, BUS_DMASYNC_POSTREAD);
			bus_dmamap_unload(sc->sc_dmat, rxb->tb_map);
			m_freem(rxb->tb_m);
		}
		bus_dmamap_destroy(sc->sc_dmat, rxb->tb_map);
	}

	tsec_dmamem_free(sc, sc->sc_rxring);
	free(sc->sc_rxbuf, M_DEVBUF, 0);
}

void
tsec_iff(struct tsec_softc *sc)
{
	struct arpcom *ac = &sc->sc_ac;
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t crc, hash[8];
	uint32_t rctrl;
	int i;

	rctrl = tsec_read(sc, TSEC_RCTRL);
	rctrl &= ~TSEC_RCTRL_PROM;
	ifp->if_flags &= ~IFF_ALLMULTI;

	if (ifp->if_flags & IFF_PROMISC || ac->ac_multirangecnt > 0) {
		ifp->if_flags |= IFF_ALLMULTI;
		rctrl |= TSEC_RCTRL_PROM;
		bzero(hash, sizeof(hash));
	} else {
		ETHER_FIRST_MULTI(step, ac, enm);
		while (enm != NULL) {
			crc = ether_crc32_be(enm->enm_addrlo,
			    ETHER_ADDR_LEN);

			crc >>= 24;
			hash[crc / 32] |= 1 << (31 - (crc % 32));

			ETHER_NEXT_MULTI(step, enm);
		}
	}

	for (i = 0; i < nitems(hash); i++)
		tsec_write(sc, TSEC_GADDR0 + i * 4, hash[i]);

	/* Alignment padding. */
	rctrl |= TSEC_RCTRL_PAL(ETHER_ALIGN);

	tsec_write(sc, TSEC_RCTRL, rctrl);
}

int
tsec_encap(struct tsec_softc *sc, struct mbuf *m, int *idx)
{
	struct tsec_desc *txd, *txd_start;
	bus_dmamap_t map;
	int cur, frag, i;
	uint16_t status;

	cur = frag = *idx;
	map = sc->sc_txbuf[cur].tb_map;

	if (bus_dmamap_load_mbuf(sc->sc_dmat, map, m, BUS_DMA_NOWAIT))
		return (ENOBUFS);

	if (map->dm_nsegs > (TSEC_NTXDESC - sc->sc_tx_cnt - 2)) {
		bus_dmamap_unload(sc->sc_dmat, map);
		return (ENOBUFS);
	}

	/* Sync the DMA map. */
	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	txd = txd_start = &sc->sc_txdesc[frag];
	for (i = 0; i < map->dm_nsegs; i++) {
		status = be16toh(txd->td_status) & TSEC_TX_W;
		status |= TSEC_TX_TO1;
		if (i == (map->dm_nsegs - 1))
			status |= TSEC_TX_L | TSEC_TX_I;
		if (i != 0)
			status |= TSEC_TX_R;
		txd->td_len = htobe16(map->dm_segs[i].ds_len);
		txd->td_addr = htobe32(map->dm_segs[i].ds_addr);
		txd->td_status = htobe16(status);

		bus_dmamap_sync(sc->sc_dmat, TSEC_DMA_MAP(sc->sc_txring),
		    frag * sizeof(*txd), sizeof(*txd), BUS_DMASYNC_PREWRITE);

		cur = frag;
		if (status & TSEC_TX_W) {
			txd = &sc->sc_txdesc[0];
			frag = 0;
		} else {
			txd++;
			frag++;
		}
		KASSERT(frag != sc->sc_tx_cons);
	}

	txd_start->td_status |= htobe16(TSEC_TX_R | TSEC_TX_TC);

	bus_dmamap_sync(sc->sc_dmat, TSEC_DMA_MAP(sc->sc_txring),
	    *idx * sizeof(*txd), sizeof(*txd), BUS_DMASYNC_PREWRITE);

	tsec_write(sc, TSEC_TSTAT, TSEC_TSTAT_THLT);

	KASSERT(sc->sc_txbuf[cur].tb_m == NULL);
	sc->sc_txbuf[*idx].tb_map = sc->sc_txbuf[cur].tb_map;
	sc->sc_txbuf[cur].tb_map = map;
	sc->sc_txbuf[cur].tb_m = m;

	sc->sc_tx_cnt += map->dm_nsegs;
	*idx = frag;

	return (0);
}

void
tsec_reset(struct tsec_softc *sc)
{
	tsec_stop_dma(sc);

	/* Set, then clear MACCFG1[Soft_Reset]. */
	tsec_write(sc, TSEC_MACCFG1, TSEC_MACCFG1_RESET);
	tsec_write(sc, TSEC_MACCFG1, 0);

	/* Clear IEVENT. */
	tsec_write(sc, TSEC_IEVENT, 0xffffffff);
}

void
tsec_stop_dma(struct tsec_softc *sc)
{
	uint32_t dmactrl, ievent;
	int n;

	/* Stop DMA. */
	dmactrl = tsec_read(sc, TSEC_DMACTRL);
	dmactrl |= TSEC_DMACTRL_GTS;
	tsec_write(sc, TSEC_DMACTRL, dmactrl);

	for (n = 0; n < 100; n++) {
		ievent = tsec_read(sc, TSEC_IEVENT);
		if (ievent & TSEC_IEVENT_GTSC)
			break;
	}
	KASSERT(n != 100);

	dmactrl = tsec_read(sc, TSEC_DMACTRL);
	dmactrl |= TSEC_DMACTRL_GRS;
	tsec_write(sc, TSEC_DMACTRL, dmactrl);

	for (n = 0; n < 100; n++) {
		ievent = tsec_read(sc, TSEC_IEVENT);
		if (ievent & TSEC_IEVENT_GRSC)
			break;
	}
	KASSERT(n != 100);
}

struct tsec_dmamem *
tsec_dmamem_alloc(struct tsec_softc *sc, bus_size_t size, bus_size_t align)
{
	struct tsec_dmamem *tdm;
	int nsegs;

	tdm = malloc(sizeof(*tdm), M_DEVBUF, M_WAITOK | M_ZERO);
	tdm->tdm_size = size;

	if (bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_WAITOK | BUS_DMA_ALLOCNOW, &tdm->tdm_map) != 0)
		goto tdmfree;

	if (bus_dmamem_alloc(sc->sc_dmat, size, align, 0, &tdm->tdm_seg, 1,
	    &nsegs, BUS_DMA_WAITOK) != 0)
		goto destroy;

	if (bus_dmamem_map(sc->sc_dmat, &tdm->tdm_seg, nsegs, size,
	    &tdm->tdm_kva, BUS_DMA_WAITOK|BUS_DMA_COHERENT) != 0)
		goto free;

	if (bus_dmamap_load(sc->sc_dmat, tdm->tdm_map, tdm->tdm_kva, size,
	    NULL, BUS_DMA_WAITOK) != 0)
		goto unmap;

	bzero(tdm->tdm_kva, size);

	return (tdm);

unmap:
	bus_dmamem_unmap(sc->sc_dmat, tdm->tdm_kva, size);
free:
	bus_dmamem_free(sc->sc_dmat, &tdm->tdm_seg, 1);
destroy:
	bus_dmamap_destroy(sc->sc_dmat, tdm->tdm_map);
tdmfree:
	free(tdm, M_DEVBUF, 0);

	return (NULL);
}

void
tsec_dmamem_free(struct tsec_softc *sc, struct tsec_dmamem *tdm)
{
	bus_dmamem_unmap(sc->sc_dmat, tdm->tdm_kva, tdm->tdm_size);
	bus_dmamem_free(sc->sc_dmat, &tdm->tdm_seg, 1);
	bus_dmamap_destroy(sc->sc_dmat, tdm->tdm_map);
	free(tdm, M_DEVBUF, 0);
}

struct mbuf *
tsec_alloc_mbuf(struct tsec_softc *sc, bus_dmamap_t map)
{
	struct mbuf *m = NULL;

	m = MCLGETI(NULL, M_DONTWAIT, NULL, MCLBYTES);
	if (!m)
		return (NULL);
	m->m_data += ETHER_ALIGN;
	m->m_len = m->m_pkthdr.len = MCLBYTES - ETHER_ALIGN;

	if (bus_dmamap_load_mbuf(sc->sc_dmat, map, m, BUS_DMA_NOWAIT) != 0) {
		printf("%s: could not load mbuf DMA map", DEVNAME(sc));
		m_freem(m);
		return (NULL);
	}

	bus_dmamap_sync(sc->sc_dmat, map, 0,
	    m->m_pkthdr.len, BUS_DMASYNC_PREREAD);

	return (m);
}

void
tsec_fill_rx_ring(struct tsec_softc *sc)
{
	struct tsec_desc *rxd;
	struct tsec_buf *rxb;
	u_int slots;

	for (slots = if_rxr_get(&sc->sc_rx_ring, TSEC_NRXDESC);
	    slots > 0; slots--) {
		rxb = &sc->sc_rxbuf[sc->sc_rx_prod];
		rxb->tb_m = tsec_alloc_mbuf(sc, rxb->tb_map);
		if (rxb->tb_m == NULL)
			break;

		rxd = &sc->sc_rxdesc[sc->sc_rx_prod];
		rxd->td_len = 0;
		rxd->td_addr = htobe32(rxb->tb_map->dm_segs[0].ds_addr);
		cpu_drain_writebuf();
		rxd->td_status |= htobe16(TSEC_RX_E | TSEC_RX_I);

		if (betoh16(rxd->td_status) & TSEC_RX_W)
			sc->sc_rx_prod = 0;
		else
			sc->sc_rx_prod++;
	}
	if_rxr_put(&sc->sc_rx_ring, slots);
}

void
tsec_get_hwaddr(struct tsec_softc *sc, uint8_t *addr)
{
	char *in;
	union {
		uint32_t reg[2];
		uint8_t addr[6];
	} hw;
	int i;

	hw.reg[0] = hw.reg[1] = 0;

	/* Retrieve the hardware address from the device tree. */

	i = fdt_node_property(sc->sc_node, "local-mac-address", &in);
	if (i == 6) {
		memcpy(hw.reg, in, 6);
		if (hw.reg[0] != 0 || hw.reg[1] != 0) {
			bcopy(hw.addr, addr, 6);
			return;
		}
	}

	/* Also try the mac-address property, which is second-best */
	i = fdt_node_property(sc->sc_node, "mac-address", &in);
	if (i == 6) {
		memcpy(hw.reg, in, 6);
		if (hw.reg[0] != 0 || hw.reg[1] != 0) {
			bcopy(hw.addr, addr, 6);
			return;
		}
	}

	/*
	 * Fall back -- use the currently programmed address in the hope that
	 * it was set be firmware...
	 */
	hw.reg[0] = tsec_read(sc, TSEC_MACSTNADDR1);
	hw.reg[1] = tsec_read(sc, TSEC_MACSTNADDR2);
	for (i = 0; i < 6; i++)
		addr[5-i] = hw.addr[i];
}
