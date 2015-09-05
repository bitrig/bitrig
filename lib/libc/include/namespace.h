/*	$OpenBSD: namespace.h,v 1.4 2015/09/05 11:25:30 guenther Exp $	*/

#ifndef _LIBC_NAMESPACE_H_
#define _LIBC_NAMESPACE_H_

/* These will be replaced with symbol renaming ala PROTO_NORMAL */
#define err		_err
#define errx		_errx
#define strtoq		_strtoq
#define strtouq		_strtouq
#define sys_errlist	_sys_errlist
#define sys_nerr	_sys_nerr
#define sys_siglist	_sys_siglist
#define verr		_verr
#define verrx		_verrx
#define vwarn		_vwarn
#define vwarnx		_vwarnx
#define warn		_warn
#define warnx		_warnx

#define pthread_getspecific	_pthread_getspecific
#define pthread_setspecific	_pthread_setspecific
#define pthread_key_create	_pthread_key_create
#define pthread_once		_pthread_once

#endif
