/*	$OpenBSD: fenv.c,v 1.3 2012/12/05 23:20:02 deraadt Exp $	*/

/*
 * Copyright (c) 2011 Martynas Venckus <martynas@openbsd.org>
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

#ifdef __ARM_PCS_VFP
#include <fenv.h>
#include <machine/ieeefp.h>

#define vmrs_fpscr(__r) __asm __volatile("vmrs %0, fpscr" : "=&r"(__r))
#define vmsr_fpscr(__r) __asm __volatile("vmsr fpscr, %0" : : "r"(__r))
#define _FPU_MASK_SHIFT 8

/*
 * The following constant represents the default floating-point environment
 * (that is, the one installed at program startup) and has type pointer to
 * const-qualified fenv_t.
 *
 * It can be used as an argument to the functions within the <fenv.h> header
 * that manage the floating-point environment, namely fesetenv() and
 * feupdateenv().
 */
fenv_t __fe_dfl_env = 0;

/*
 * The feclearexcept() function clears the supported floating-point exceptions
 * represented by `excepts'.
 */
int
feclearexcept(int excepts)
{
	fexcept_t fpsr;
	excepts &= FE_ALL_EXCEPT;

	/* Clear the requested floating-point exceptions */
	vmrs_fpscr(fpsr);
	fpsr &= ~excepts;
	vmsr_fpscr(fpsr);

	return (0);
}

/*
 * The fegetexceptflag() function stores an implementation-defined
 * representation of the states of the floating-point status flags indicated by
 * the argument excepts in the object pointed to by the argument flagp.
 */
int
fegetexceptflag(fexcept_t *flagp, int excepts)
{
	fexcept_t fpsr;
	excepts &= FE_ALL_EXCEPT;

	/* Store the results in flagp */
	vmrs_fpscr(fpsr);
	*flagp = fpsr & excepts;

	return (0);
}

/*
 * The feraiseexcept() function raises the supported floating-point exceptions
 * represented by the argument `excepts'.
 */
int
feraiseexcept(int excepts)
{
	excepts &= FE_ALL_EXCEPT;

	fesetexceptflag((fexcept_t *)&excepts, excepts);

	return (0);
}

/*
 * This function sets the floating-point status flags indicated by the argument
 * `excepts' to the states stored in the object pointed to by `flagp'. It does
 * NOT raise any floating-point exceptions, but only sets the state of the flags.
 */
int
fesetexceptflag(const fexcept_t *flagp, int excepts)
{
	fexcept_t fpsr;
	excepts &= FE_ALL_EXCEPT;

	/* Set the requested status flags */
	vmrs_fpscr(fpsr);
	fpsr &= ~excepts;
	fpsr |= *flagp & excepts;
	vmsr_fpscr(fpsr);

	return (0);
}

/*
 * The fetestexcept() function determines which of a specified subset of the
 * floating-point exception flags are currently set. The `excepts' argument
 * specifies the floating-point status flags to be queried.
 */
int
fetestexcept(int excepts)
{
	fexcept_t fpsr;
	excepts &= FE_ALL_EXCEPT;

	vmrs_fpscr(fpsr);
	return (fpsr & excepts);
}

/*
 * The fegetround() function gets the current rounding direction.
 */
int
fegetround(void)
{
	fenv_t fpsr;

	vmrs_fpscr(fpsr);
	return (fpsr & _ROUND_MASK);
}

/*
 * The fesetround() function establishes the rounding direction represented by
 * its argument `round'. If the argument is not equal to the value of a rounding
 * direction macro, the rounding direction is not changed.
 */
int
fesetround(int round)
{
	/* Check whether requested rounding direction is supported */
	if (round & ~_ROUND_MASK)
		return (-1);

	fenv_t fpsr;

	/* Set the rounding direction */
	vmrs_fpscr(fpsr);
	fpsr &= ~(_ROUND_MASK);
	fpsr |= round;
	vmsr_fpscr(fpsr);

	return (0);
}

/*
 * The fegetenv() function attempts to store the current floating-point
 * environment in the object pointed to by envp.
 */
int
fegetenv(fenv_t *envp)
{
	/* Store the current floating-point environment */
	vmrs_fpscr(*envp);

	return (0);
}

/*
 * The feholdexcept() function saves the current floating-point environment
 * in the object pointed to by envp, clears the floating-point status flags, and
 * then installs a non-stop (continue on floating-point exceptions) mode, if
 * available, for all floating-point exceptions.
 */
int
feholdexcept(fenv_t *envp)
{
	fenv_t env;

	vmrs_fpscr(env);
	*envp = env;
	env &= ~(FE_ALL_EXCEPT);
	vmsr_fpscr(env);

	return (0);
}

/*
 * The fesetenv() function attempts to establish the floating-point environment
 * represented by the object pointed to by envp. The argument `envp' points
 * to an object set by a call to fegetenv() or feholdexcept(), or equal a
 * floating-point environment macro. The fesetenv() function does not raise
 * floating-point exceptions, but only installs the state of the floating-point
 * status flags represented through its argument.
 */
int
fesetenv(const fenv_t *envp)
{
	vmsr_fpscr(*envp);

	return (0);
}

/*
 * The feupdateenv() function saves the currently raised floating-point
 * exceptions in its automatic storage, installs the floating-point environment
 * represented by the object pointed to by `envp', and then raises the saved
 * floating-point exceptions. The argument `envp' shall point to an object set
 * by a call to feholdexcept() or fegetenv(), or equal a floating-point
 * environment macro.
 */
int
feupdateenv(const fenv_t *envp)
{
	fexcept_t fpsr;

	vmrs_fpscr(fpsr);
	vmsr_fpscr(*envp);
	feraiseexcept(fpsr & FE_ALL_EXCEPT);

	return (0);
}

/*
 * The following functions are extentions to the standard
 */
int
feenableexcept(int mask)
{
	fenv_t old_fpsr, new_fpsr;

	vmrs_fpscr(old_fpsr);
	new_fpsr = old_fpsr |
	    ((mask & FE_ALL_EXCEPT) << _FPU_MASK_SHIFT);
	vmsr_fpscr(new_fpsr);
	return ((old_fpsr >> _FPU_MASK_SHIFT) & FE_ALL_EXCEPT);
}

int
fedisableexcept(int mask)
{
	fenv_t old_fpsr, new_fpsr;

	vmrs_fpscr(old_fpsr);
	new_fpsr = old_fpsr &
	    ~((mask & FE_ALL_EXCEPT) << _FPU_MASK_SHIFT);
	vmsr_fpscr(new_fpsr);
	return ((old_fpsr >> _FPU_MASK_SHIFT) & FE_ALL_EXCEPT);
}

int
fegetexcept(void)
{
	fenv_t fpsr;

	vmrs_fpscr(fpsr);
	return (fpsr & FE_ALL_EXCEPT);
}
#endif
