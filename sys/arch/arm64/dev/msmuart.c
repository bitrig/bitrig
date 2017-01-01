/*
 * Copyright (c) 2005,2016 Dale Rahn <drahn@dalerahn.com>
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
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/uio.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/device.h>
#include <sys/syslog.h>
#include <sys/conf.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/kernel.h>

#include <dev/cons.h>


#ifdef DDB
#include <ddb/db_var.h>
#endif

#include <machine/bus.h>
#include <machine/clock.h>
#include <machine/fdt.h>
#include <machine/fdt.h>
#include <arm64/arm64/arm64var.h>
#include <arm64/dev/msmuartvar.h>

/* DataMover UART registers and bits */
#define DM_DMRX         0x34
#define DM_NCHAR_TX       0x40	// Number of Chars for TX

#define DM_SR           0xA4
#define         DM_SR_RX_EMPTY  (1 << 0)
#define         DM_SR_TX_EMPTY  (1 << 3) /* TX underrun */
#define         DM_SR_UART_OVERRUN  (1 << 3) /* RX overrun */

#define DM_CR           0xA8
#define		DM_CR_CMD_RX_EN			(1 << 0)
#define		DM_CR_CMD_RX_DIS		(1 << 1)
#define		DM_CR_CMD_TX_EN			(1 << 2)
#define		DM_CR_CMD_TX_DIS		(1 << 3)
#define		DM_CR_CMD_RESET_ERR		(3 << 4)
#define		DM_CR_CMD_RESET_TX_READY	(3 << 8)
#define		DM_CR_CMD_FORCE_STALE		(4 << 8)
#define		DM_CR_CMD_ENABLE_STALE		(5 << 8)
#define		DM_CR_CMD_DISABLE_STALE		(6 << 8)

#define DM_IMR          0xb0
#define DM_ISR          0xb4
#define		DM_ISR_TX_ERROR			(1 << 8)
#define		DM_ISR_TX_READY			(1 << 7)
#define		DM_ISR_RXLEV			(1 << 7)
#define		DM_ISR_RXSTALE			(1 << 7)

#define DM_TF           0x100
#define DM_RF           0x140

#define MSMUART_SIZE	0x200

#define DEVUNIT(x)      (minor(x) & 0x7f)


#define DEVUNIT(x)      (minor(x) & 0x7f)
#define DEVCUA(x)       (minor(x) & 0x80)

struct msmuart_softc {
	struct device	sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	struct soft_intrhand *sc_si;
	void *sc_irq;
	struct tty	*sc_tty;
	struct timeout	sc_diag_tmo;
	struct timeout	sc_dtr_tmo;
	int		sc_overflows;
	int		sc_floods;
	int		sc_errors;
	int		sc_halt;
	u_int16_t	sc_ucr1;
	u_int16_t	sc_ucr2;
	u_int16_t	sc_ucr3;
	u_int16_t	sc_ucr4;
	u_int8_t	sc_hwflags;
#define COM_HW_NOIEN    0x01
#define COM_HW_FIFO     0x02
#define COM_HW_SIR      0x20
#define COM_HW_CONSOLE  0x40
	u_int8_t	sc_swflags;
#define COM_SW_SOFTCAR  0x01
#define COM_SW_CLOCAL   0x02
#define COM_SW_CRTSCTS  0x04
#define COM_SW_MDMBUF   0x08
#define COM_SW_PPS      0x10
	int		sc_fifolen;

	u_int8_t	sc_initialize;
	u_int8_t	sc_cua;
	u_int16_t 	*sc_ibuf, *sc_ibufp, *sc_ibufhigh, *sc_ibufend;
#define UART_IBUFSIZE 128
#define UART_IHIGHWATER 100
	u_int16_t		sc_ibufs[2][UART_IBUFSIZE];

	struct clk	*sc_clk;
};


int     msmuartprobe(struct device *parent, void *self, void *aux);
void    msmuartattach(struct device *parent, struct device *self, void *aux);

void msmuartcnprobe(struct consdev *cp);
void msmuartcnprobe(struct consdev *cp);
void msmuartcninit(struct consdev *cp);
int msmuartcnattach(bus_space_tag_t iot, bus_addr_t iobase, int rate,
    tcflag_t cflag);
int msmuartcngetc(dev_t dev);
void msmuartcnputc(dev_t dev, int c);
void msmuartcnpollc(dev_t dev, int on);
int  msmuart_param(struct tty *tp, struct termios *t);
void msmuart_start(struct tty *);
void msmuart_pwroff(struct msmuart_softc *sc);
void msmuart_diag(void *arg);
void msmuart_raisedtr(void *arg);
void msmuart_softint(void *arg);
struct msmuart_softc *msmuart_sc(dev_t dev);

int msmuart_intr(void *);


/* XXX - we imitate 'com' serial ports and take over their entry points */
/* XXX: These belong elsewhere */
cdev_decl(msmuart);

struct cfdriver msmuart_cd = {
	NULL, "msmuart", DV_TTY
};

struct cfattach msmuart_ca = {
	sizeof(struct msmuart_softc), msmuartprobe, msmuartattach
};

bus_space_tag_t	msmuartconsiot;
bus_space_handle_t msmuartconsioh;
bus_addr_t	msmuartconsaddr;
tcflag_t	msmuartconscflag = TTYDEF_CFLAG;
int		msmuartdefaultrate = B115200;

int
msmuartprobe(struct device *parent, void *self, void *aux)
{
	struct arm64_attach_args *aa = aux;

	// full is qcom,msm-uartdm-v1.4
	if (fdt_node_compatible("qcom,msm-uartdm", aa->aa_node))
		return 1;

	return 0;
}

struct cdevsw msmuartdev =
	cdev_tty_init(3/*XXX NUART */ ,msmuart);		/* 12: serial port */

void
msmuartattach(struct device *parent, struct device *self, void *args)
{
	struct arm64_attach_args *aa = args;
	struct msmuart_softc *sc = (struct msmuart_softc *) self;
	struct fdt_memory mem;

	if (aa->aa_node == NULL)
		panic("%s: no fdt node", __func__);

	if (fdt_get_memory_address(aa->aa_node, 0, &mem))
		panic("%s: could not extract memory data from FDT", __func__);

	sc->sc_irq = arm_intr_establish_fdt(aa->aa_node, IPL_TTY, msmuart_intr,
	    sc, sc->sc_dev.dv_xname);

	sc->sc_iot = aa->aa_iot;
	if (bus_space_map(sc->sc_iot, mem.addr, mem.size, 0, &sc->sc_ioh))
		panic("msmuartattach: bus_space_map failed!");

	if (mem.addr == msmuartconsaddr)
		printf(" console");

	timeout_set(&sc->sc_diag_tmo, msmuart_diag, sc);
	timeout_set(&sc->sc_dtr_tmo, msmuart_raisedtr, sc);
	sc->sc_si = softintr_establish(IPL_TTY, msmuart_softint, sc);

	if(sc->sc_si == NULL)
		panic("%s: can't establish soft interrupt.",
		    sc->sc_dev.dv_xname);

	// need to turn enable interrupts, anything else?
	// ?? enable rx/tx
	// ?? set baud
	// ?? clear existing events
	// 
	printf("time to implement msmuart fully");
#if 0
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, UART_IMSC, (UART_IMSC_RXIM | UART_IMSC_TXIM));
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, UART_ICR, 0x7ff);
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, UART_LCR_H,
	    bus_space_read_4(sc->sc_iot, sc->sc_ioh, UART_LCR_H) &
	    ~UART_LCR_H_FEN);
#endif

	printf("\n");
}

int
msmuart_intr(void *arg)
{
	struct msmuart_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	//struct tty *tp = sc->sc_tty;
	//u_int16_t fr;
	u_int16_t *p;
	//u_int16_t c;

	//bus_space_write_4(iot, ioh, UART_ICR, -1);

	if (sc->sc_tty == NULL)
		return(0);

	// implement TX path
#if 0
	fr = bus_space_read_4(iot, ioh, UART_FR);
	if (ISSET(fr, UART_FR_TXFE) && ISSET(tp->t_state, TS_BUSY)) {
		CLR(tp->t_state, TS_BUSY | TS_FLUSH);
		if (sc->sc_halt > 0)
			wakeup(&tp->t_outq);
		(*linesw[tp->t_line].l_start)(tp);
	}
#endif

	// if nothing in RX, return
	if(!ISSET(bus_space_read_4(iot, ioh, DM_SR), DM_SR_RX_EMPTY))
		return 0;

	p = sc->sc_ibufp;

#if 0
	while(ISSET(bus_space_read_4(iot, ioh, UART_FR), UART_FR_RXFF)) {
		c = bus_space_read_1(iot, ioh, UART_DR);
		if (p >= sc->sc_ibufend) {
			sc->sc_floods++;
			if (sc->sc_errors++ == 0)
				timeout_add(&sc->sc_diag_tmo, 60 * hz);
		} else {
			*p++ = c;
			if (p == sc->sc_ibufhigh && ISSET(tp->t_cflag, CRTSCTS)) {
				/* XXX */
				//CLR(sc->sc_ucr3, IMXUART_CR3_DSR);
				//bus_space_write_4(iot, ioh, IMXUART_UCR3,
				//    sc->sc_ucr3);
			}
		}
		/* XXX - msr stuff ? */
	}
#endif
	sc->sc_ibufp = p;

	softintr_schedule(sc->sc_si);

	return 1;
}

int
msmuart_param(struct tty *tp, struct termios *t)
{
	struct msmuart_softc *sc = msmuart_cd.cd_devs[DEVUNIT(tp->t_dev)];
	//bus_space_tag_t iot = sc->sc_iot;
	//bus_space_handle_t ioh = sc->sc_ioh;
	int ospeed = t->c_ospeed;
	int error;
	tcflag_t oldcflag;


	if (t->c_ospeed < 0 || (t->c_ispeed && t->c_ispeed != t->c_ospeed))
		return EINVAL;

	switch (ISSET(t->c_cflag, CSIZE)) {
	case CS5:
		return EINVAL;
	case CS6:
		return EINVAL;
	case CS7:
		//CLR(sc->sc_ucr2, IMXUART_CR2_WS);
		break;
	case CS8:
		//SET(sc->sc_ucr2, IMXUART_CR2_WS);
		break;
	}
//	bus_space_write_4(iot, ioh, IMXUART_UCR2, sc->sc_ucr2);

/*
	if (ISSET(t->c_cflag, PARENB)) {
		SET(sc->sc_ucr2, IMXUART_CR2_PREN);
		bus_space_write_4(iot, ioh, IMXUART_UCR2, sc->sc_ucr2);
	}
*/
	/* STOPB - XXX */
	if (ospeed == 0) {
		/* lower dtr */
	}

	if (ospeed != 0) {
		while (ISSET(tp->t_state, TS_BUSY)) {
			++sc->sc_halt;
			error = ttysleep(tp, &tp->t_outq,
			    TTOPRI | PCATCH, "msmuartprm", 0);
			--sc->sc_halt;
			if (error) {
				msmuart_start(tp);
				return (error);
			}
		}
		/* set speed */
	}

	/* setup fifo */

	/* When not using CRTSCTS, RTS follows DTR. */
	/* sc->sc_dtr = MCR_DTR; */


	/* and copy to tty */
	tp->t_ispeed = t->c_ispeed;
	tp->t_ospeed = t->c_ospeed;
	oldcflag = tp->t_cflag;
	tp->t_cflag = t->c_cflag;

        /*
	 * If DCD is off and MDMBUF is changed, ask the tty layer if we should
	 * stop the device.
	 */
	 /* XXX */

	msmuart_start(tp);

	return 0;
}

void
msmuart_start(struct tty *tp)
{
	struct msmuart_softc *sc = msmuart_cd.cd_devs[DEVUNIT(tp->t_dev)];
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	int s;
	s = spltty();
	if (ISSET(tp->t_state, TS_BUSY | TS_TIMEOUT | TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc <= tp->t_lowat) {
		if (ISSET(tp->t_state, TS_ASLEEP)) {
			CLR(tp->t_state, TS_ASLEEP);
			wakeup(&tp->t_outq);
		}
		if (tp->t_outq.c_cc == 0)
			goto out;
		selwakeup(&tp->t_wsel);
	}
	SET(tp->t_state, TS_BUSY);

	if (ISSET(sc->sc_hwflags, COM_HW_FIFO)) {
		u_char buffer[64];	/* largest fifo */
		int i, n;

		n = q_to_b(&tp->t_outq, buffer,
		    min(sc->sc_fifolen, sizeof buffer));
		for (i = 0; i < n; i++) {
#if 0
			bus_space_write_4(iot, ioh, UART_DR, buffer[i]);
#endif
		}
		bzero(buffer, n);
	} else if (tp->t_outq.c_cc != 0) {
		bus_space_write_4(iot, ioh, DM_NCHAR_TX, 1);
		bus_space_write_4(iot, ioh, DM_TF, getc(&tp->t_outq));
	}

out:
	splx(s);
}

void
msmuart_pwroff(struct msmuart_softc *sc)
{
}

void
msmuart_diag(void *arg)
{
	struct msmuart_softc *sc = arg;
	int overflows, floods;
	int s;

	s = spltty();
	sc->sc_errors = 0;
	overflows = sc->sc_overflows;
	sc->sc_overflows = 0;
	floods = sc->sc_floods;
	sc->sc_floods = 0;
	splx(s);
	log(LOG_WARNING, "%s: %d silo overflow%s, %d ibuf overflow%s\n",
	    sc->sc_dev.dv_xname,
	    overflows, overflows == 1 ? "" : "s",
	    floods, floods == 1 ? "" : "s");
}

void
msmuart_raisedtr(void *arg)
{
	//struct msmuart_softc *sc = arg;

	//SET(sc->sc_ucr3, IMXUART_CR3_DSR); /* XXX */
	//bus_space_write_4(sc->sc_iot, sc->sc_ioh, IMXUART_UCR3, sc->sc_ucr3);
}

void
msmuart_softint(void *arg)
{
	struct msmuart_softc *sc = arg;
	struct tty *tp;
	u_int16_t *ibufp;
	u_int16_t *ibufend;
	int c;
	int err;
	int s;

	if (sc == NULL || sc->sc_ibufp == sc->sc_ibuf)
		return;

	tp = sc->sc_tty;
	s = spltty();

	ibufp = sc->sc_ibuf;
	ibufend = sc->sc_ibufp;

	if (ibufp == ibufend || tp == NULL || !ISSET(tp->t_state, TS_ISOPEN)) {
		splx(s);
		return;
	}

	sc->sc_ibufp = sc->sc_ibuf = (ibufp == sc->sc_ibufs[0]) ?
	    sc->sc_ibufs[1] : sc->sc_ibufs[0];
	sc->sc_ibufhigh = sc->sc_ibuf + UART_IHIGHWATER;
	sc->sc_ibufend = sc->sc_ibuf + UART_IBUFSIZE;

#if 0
	if (ISSET(tp->t_cflag, CRTSCTS) &&
	    !ISSET(sc->sc_ucr3, IMXUART_CR3_DSR)) {
		/* XXX */
		SET(sc->sc_ucr3, IMXUART_CR3_DSR);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, IMXUART_UCR3,
		    sc->sc_ucr3);
	}
#endif

	splx(s);

	while (ibufp < ibufend) {
		c = *ibufp++;
		/*
		if (ISSET(c, IMXUART_RX_OVERRUN)) {
			sc->sc_overflows++;
			if (sc->sc_errors++ == 0)
				timeout_add(&sc->sc_diag_tmo, 60 * hz);
		}
		*/
		/* This is ugly, but fast. */

		err = 0;
		/*
		if (ISSET(c, IMXUART_RX_PRERR))
			err |= TTY_PE;
		if (ISSET(c, IMXUART_RX_FRMERR))
			err |= TTY_FE;
		*/
		c = (c & 0xff) | err;
		(*linesw[tp->t_line].l_rint)(c, tp);
	}
}

int
msmuartopen(dev_t dev, int flag, int mode, struct proc *p)
{
	int unit = DEVUNIT(dev);
	struct msmuart_softc *sc;
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	struct tty *tp;
	int s;
	int error = 0;

	if (unit >= msmuart_cd.cd_ndevs)
		return ENXIO;
	sc = msmuart_cd.cd_devs[unit];
	if (sc == NULL)
		return ENXIO;

	s = spltty();
	if (sc->sc_tty == NULL)
		tp = sc->sc_tty = ttymalloc(0);
	else
		tp = sc->sc_tty;

	splx(s);

	tp->t_oproc = msmuart_start;
	tp->t_param = msmuart_param;
	tp->t_dev = dev;

	if (!ISSET(tp->t_state, TS_ISOPEN)) {
		SET(tp->t_state, TS_WOPEN);
		ttychars(tp);
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;

		if (ISSET(sc->sc_hwflags, COM_HW_CONSOLE))
			tp->t_cflag = msmuartconscflag;
		else
			tp->t_cflag = TTYDEF_CFLAG;
		if (ISSET(sc->sc_swflags, COM_SW_CLOCAL))
			SET(tp->t_cflag, CLOCAL);
		if (ISSET(sc->sc_swflags, COM_SW_CRTSCTS))
			SET(tp->t_cflag, CRTSCTS);
		if (ISSET(sc->sc_swflags, COM_SW_MDMBUF))
			SET(tp->t_cflag, MDMBUF);
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = msmuartdefaultrate;

		s = spltty();

		sc->sc_initialize = 1;
		msmuart_param(tp, &tp->t_termios);
		ttsetwater(tp);
		sc->sc_ibufp = sc->sc_ibuf = sc->sc_ibufs[0];
		sc->sc_ibufhigh = sc->sc_ibuf + UART_IHIGHWATER;
		sc->sc_ibufend = sc->sc_ibuf + UART_IBUFSIZE;

		iot = sc->sc_iot;
		ioh = sc->sc_ioh;

#if 0
		sc->sc_ucr1 = bus_space_read_4(iot, ioh, IMXUART_UCR1);
		sc->sc_ucr2 = bus_space_read_4(iot, ioh, IMXUART_UCR2);
		sc->sc_ucr3 = bus_space_read_4(iot, ioh, IMXUART_UCR3);
		sc->sc_ucr4 = bus_space_read_4(iot, ioh, IMXUART_UCR4);

		/* interrupt after one char on tx/rx */
		/* reference frequency divider: 1 */
		bus_space_write_4(iot, ioh, IMXUART_UFCR,
		    1 << IMXUART_FCR_TXTL_SH |
		    5 << IMXUART_FCR_RFDIV_SH |
		    1 << IMXUART_FCR_RXTL_SH);

		bus_space_write_4(iot, ioh, IMXUART_UBIR,
		    (msmuartdefaultrate / 100) - 1);

		/* formula: clk / (rfdiv * 1600) */
		bus_space_write_4(iot, ioh, IMXUART_UBMR,
		    (clk_get_rate(sc->sc_clk) * 1000) / 1600);

		SET(sc->sc_ucr1, IMXUART_CR1_EN|IMXUART_CR1_RRDYEN);
		SET(sc->sc_ucr2, IMXUART_CR2_TXEN|IMXUART_CR2_RXEN);
		bus_space_write_4(iot, ioh, IMXUART_UCR1, sc->sc_ucr1);
		bus_space_write_4(iot, ioh, IMXUART_UCR2, sc->sc_ucr2);

		/* sc->sc_mcr = MCR_DTR | MCR_RTS;  XXX */
		SET(sc->sc_ucr3, IMXUART_CR3_DSR); /* XXX */
		bus_space_write_4(iot, ioh, IMXUART_UCR3, sc->sc_ucr3);
#endif

		SET(tp->t_state, TS_CARR_ON); /* XXX */


	} else if (ISSET(tp->t_state, TS_XCLUDE) && p->p_ucred->cr_uid != 0)
		return EBUSY;
	else
		s = spltty();

	if (DEVCUA(dev)) {
		if (ISSET(tp->t_state, TS_ISOPEN)) {
			splx(s);
			return EBUSY;
		}
		sc->sc_cua = 1;
	} else {
		/* tty (not cua) device; wait for carrier if necessary */
		if (ISSET(flag, O_NONBLOCK)) {
			if (sc->sc_cua) {
				/* Opening TTY non-blocking... but the CUA is busy */
				splx(s);
				return EBUSY;
			}
		} else {
			while (sc->sc_cua ||
			    (!ISSET(tp->t_cflag, CLOCAL) &&
				!ISSET(tp->t_state, TS_CARR_ON))) {
				SET(tp->t_state, TS_WOPEN);
				error = ttysleep(tp, &tp->t_rawq,
				    TTIPRI | PCATCH, ttopen, 0);
				/*
				 * If TS_WOPEN has been reset, that means the
				 * cua device has been closed.  We don't want
				 * to fail in that case,
				 * so just go around again.
				 */
				if (error && ISSET(tp->t_state, TS_WOPEN)) {
					CLR(tp->t_state, TS_WOPEN);
					if (!sc->sc_cua && !ISSET(tp->t_state,
					    TS_ISOPEN))
						msmuart_pwroff(sc);
					splx(s);
					return error;
				}
			}
		}
	}
	splx(s);
	return (*linesw[tp->t_line].l_open)(dev,tp,p);
}

int
msmuartclose(dev_t dev, int flag, int mode, struct proc *p)
{
	int unit = DEVUNIT(dev);
	struct msmuart_softc *sc = msmuart_cd.cd_devs[unit];
	//bus_space_tag_t iot = sc->sc_iot;
	//bus_space_handle_t ioh = sc->sc_ioh;
	struct tty *tp = sc->sc_tty;
	int s;

	/* XXX This is for cons.c. */
	if (!ISSET(tp->t_state, TS_ISOPEN))
		return 0;

	(*linesw[tp->t_line].l_close)(tp, flag, p);
	s = spltty();
	if (ISSET(tp->t_state, TS_WOPEN)) {
		/* tty device is waiting for carrier; drop dtr then re-raise */
		//CLR(sc->sc_ucr3, IMXUART_CR3_DSR);
		//bus_space_write_4(iot, ioh, IMXUART_UCR3, sc->sc_ucr3);
		timeout_add(&sc->sc_dtr_tmo, hz * 2);
	} else {
		/* no one else waiting; turn off the uart */
		msmuart_pwroff(sc);
	}
	CLR(tp->t_state, TS_BUSY | TS_FLUSH);

	sc->sc_cua = 0;
	splx(s);
	ttyclose(tp);

	return 0;
}

int
msmuartread(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tty;

	tty = msmuarttty(dev);
	if (tty == NULL)
		return ENODEV;

	return((*linesw[tty->t_line].l_read)(tty, uio, flag));
}

int
msmuartwrite(dev_t dev, struct uio *uio, int flag)
{
	struct tty *tty;

	tty = msmuarttty(dev);
	if (tty == NULL)
		return ENODEV;

	return((*linesw[tty->t_line].l_write)(tty, uio, flag));
}

int
msmuartioctl( dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	struct msmuart_softc *sc;
	struct tty *tp;
	int error;

	sc = msmuart_sc(dev);
	if (sc == NULL)
		return (ENODEV);

	tp = sc->sc_tty;
	if (tp == NULL)
		return (ENXIO);

	error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	error = ttioctl(tp, cmd, data, flag, p);
	if (error >= 0)
		return (error);

	switch(cmd) {
	case TIOCSBRK:
		break;
	case TIOCCBRK:
		break;
	case TIOCSDTR:
		break;
	case TIOCCDTR:
		break;
	case TIOCMSET:
		break;
	case TIOCMBIS:
		break;
	case TIOCMBIC:
		break;
	case TIOCMGET:
		break;
	case TIOCGFLAGS:
		break;
	case TIOCSFLAGS:
		error = suser(p, 0);
		if (error != 0)
			return(EPERM);
		break;
	default:
		return (ENOTTY);
	}

	return 0;
}

int
msmuartstop(struct tty *tp, int flag)
{
	return 0;
}

struct tty *
msmuarttty(dev_t dev)
{
	int unit;
	struct msmuart_softc *sc;
	unit = DEVUNIT(dev);
	if (unit >= msmuart_cd.cd_ndevs)
		return NULL;
	sc = (struct msmuart_softc *)msmuart_cd.cd_devs[unit];
	if (sc == NULL)
		return NULL;
	return sc->sc_tty;
}

struct msmuart_softc *
msmuart_sc(dev_t dev)
{
	int unit;
	struct msmuart_softc *sc;
	unit = DEVUNIT(dev);
	if (unit >= msmuart_cd.cd_ndevs)
		return NULL;
	sc = (struct msmuart_softc *)msmuart_cd.cd_devs[unit];
	return sc;
}


/* serial console */
void
msmuartcnprobe(struct consdev *cp)
{
	cp->cn_dev = makedev(12 /* XXX */, 0);
	cp->cn_pri = CN_MIDPRI;
}

void
msmuartcninit(struct consdev *cp)
{
	// Assume hardware is set up by u-boot
}

int
msmuartcnattach(bus_space_tag_t iot, bus_addr_t iobase, int rate, tcflag_t cflag)
{
	static struct consdev msmuartcons = {
		NULL, NULL, msmuartcngetc, msmuartcnputc, msmuartcnpollc, NULL,
		NODEV, CN_MIDPRI
	};

	if (bus_space_map(iot, iobase, MSMUART_SIZE, 0, &msmuartconsioh))
			return ENOMEM;

	cn_tab = &msmuartcons;
	cn_tab->cn_dev = makedev(12 /* XXX */, 0);
	cdevsw[12] = msmuartdev; 	/* KLUDGE */

	msmuartconsiot = iot;
	msmuartconsaddr = iobase;
	msmuartconscflag = cflag;

	return 0;
}

int
msmuartcngetc(dev_t dev)
{
	int c;
	int s;
	s = splhigh();
	while(!ISSET(bus_space_read_4(msmuartconsiot, msmuartconsioh, DM_SR),
	    DM_SR_RX_EMPTY))
		;
	// not correct, may be more than one char in buffer.
	c = bus_space_read_4(msmuartconsiot, msmuartconsioh, DM_RF);
	splx(s);
	return c;
}

void
msmuartcnputc(dev_t dev, int c)
{
	int s;
	s = splhigh();
	while(!ISSET(bus_space_read_4(msmuartconsiot, msmuartconsioh, DM_SR),
	    DM_SR_TX_EMPTY))
		;
	bus_space_write_4(msmuartconsiot, msmuartconsioh, DM_NCHAR_TX, 1);
	bus_space_write_4(msmuartconsiot, msmuartconsioh, DM_TF, (uint8_t)c);
	splx(s);
}

void
msmuartcnpollc(dev_t dev, int on)
{
}
