define(MACHINE,armv7)dnl
vers(__file__,
	{-$OpenBSD: MAKEDEV.md,v 1.8 2014/12/11 19:48:03 tedu Exp $-},
etc.MACHINE)dnl
dnl
dnl Copyright (c) 2001-2004 Todd T. Fries <todd@OpenBSD.org>
dnl All rights reserved.
dnl
dnl Redistribution and use in source and binary forms, with or without
dnl modification, are permitted provided that the following conditions
dnl are met:
dnl 1. Redistributions of source code must retain the above copyright
dnl    notice, this list of conditions and the following disclaimer.
dnl 2. The name of the author may not be used to endorse or promote products
dnl    derived from this software without specific prior written permission.
dnl
dnl THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
dnl INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
dnl AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
dnl THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
dnl EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
dnl PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
dnl OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
dnl WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
dnl OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
dnl ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
dnl
dnl
__devitem(apm, apm, Power management device)dnl
_TITLE(make)
_DEV(all)
_DEV(ramdisk)
_DEV(std)
_DEV(local)
_TITLE(dis)
_DEV(cd, 15, 6)
_DEV(sd, 13, 4)
_DEV(tmpfsrd, 48, 19)
_DEV(nbd, 43, 20)
_DEV(vnd, 41, 14)
_DEV(wd, 3, 0)
_TITLE(tap)
_DEV(ch, 17)
_DEV(st, 14, 5)
_TITLE(term)
_DEV(com, 8)
_TITLE(pty)
_DEV(ptm, 81)
_DEV(pty, 6)
_DEV(tty, 5)
_TITLE(cons)
_DEV(wsdisp, 12)
_DEV(wscons)
_DEV(wskbd, 67)
_DEV(wsmux, 69)
_TITLE(point)
_DEV(wsmouse, 68)
_TITLE(usb)
_DEV(ttyU, 66)
_DEV(uall)
_DEV(ugen, 63)
_DEV(uhid, 62)
_DEV(ulpt, 64)
_DEV(usb, 61)
_TITLE(spec)
_DEV(apm, 83)
_DEV(au, 42)
_DEV(bio, 79)
_DEV(bktr, 49)
_DEV(bpf, 23)
_DEV(diskmap, 90)
_DEV(drm, 87)
_DEV(fdesc, 22)
_DEV(fuse, 92)
_DEV(gpio, 88)
_DEV(hotplug, 82)
_DEV(pci, 72)
_DEV(pf, 73)
_DEV(pppx, 91)
_DEV(radio, 76)
_DEV(rnd, 45)
_DEV(rmidi, 52)
_DEV(systrace, 78)
_DEV(tun, 40)
_DEV(tuner, 49)
_DEV(uk, 20)
_DEV(vi, 44)
_DEV(vscsi, 89)
dnl
divert(__mddivert)dnl
dnl
ramdisk)
	_recurse std bpf0 wd0 wd1 sd0 tty00 tmpfsrd0 wsmouse
	_recurse st0 ttyC0 wskbd0 apm bio diskmap random nbd0
	;;

_std(1, 2, 50, 7)
	;;
dnl
dnl *** arm64 specific targets
dnl
target(all, ch, 0)dnl
target(all, vscsi, 0)dnl
target(all, diskmap)dnl
target(all, pty, 0)dnl
target(all, bio)dnl
target(all, bpf, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9)dnl
target(all, tun, 0, 1, 2, 3)dnl
target(all, xy, 0, 1, 2, 3)dnl
target(all, tmpfsrd, 0)dnl
target(all, cd, 0, 1)dnl
target(all, sd, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9)dnl
target(all, nbd, 0, 1, 2, 3)dnl
target(all, vnd, 0, 1, 2, 3)dnl
target(all, gpio, 0, 1, 2, 3, 4, 5, 6, 7, 8)dnl
target(all, drm, 0, 1, 2, 3)dnl
