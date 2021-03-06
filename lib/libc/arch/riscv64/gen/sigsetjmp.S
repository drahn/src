/*	$OpenBSD: sigsetjmp.S,v 1.1 2021/04/28 15:28:54 drahn Exp $	*/
/* 
 * Copyright (c) 2020 Dale Rahn <drahn@openbsd.org>
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

#include "SYS.h"
#include <machine/setjmp.h>

ENTRY(sigsetjmp)

	RETGUARD_SETUP(setjmp, t6)
	mv	a2, a0 				/* save jmpbuf in x2 */
	li	a0, 0
	beqz	a1, 1f
	/* Store the signal mask */
	mv	a2, a0				/* set */
	li	a1, 0
	li	a0, 1				/* SIG_BLOCK */
	SYSTRAP(sigprocmask)
1:
	sw	a0, (_JB_SIGMASK * 8)(a2)	/* oset */

	mv	a0, a2
	sd	sp, 8(a0)

	/* Store the general purpose registers and ra */
	sd	s0, 16(a0)
	sd	s1, 24(a0)
	sd	s2, 32(a0)
	sd	s3, 40(a0)
	sd	s4, 48(a0)
	sd	s5, 56(a0)
	sd	s6, 64(a0)
	sd	s7, 72(a0)
	sd	s8, 90(a0)
	sd	s9, 98(a0)
	sd	s10, 106(a0)
	sd	s11, 112(a0)
	sd	ra, 120(a0)

	/* Store the fp registers */
	fsd	fs0, 128(a0)
	fsd	fs1, 136(a0)
	fsd	fs2, 144(a0)
	fsd	fs3, 152(a0)
	fsd	fs4, 160(a0)
	fsd	fs5, 168(a0)
	fsd	fs6, 176(a0)
	fsd	fs7, 184(a0)
	fsd	fs8, 192(a0)
	fsd	fs9, 200(a0)
	fsd	fs10, 208(a0)
	fsd	fs11, 216(a0)
	frcsr	t0	
	sd	t0, 232(a0)

	/* Return value */
	mv	a0, x0
	RETGUARD_CHECK(setjmp, t6)
	ret

END_STRONG(sigsetjmp)

ENTRY(siglongjmp)
	RETGUARD_SYMBOL(longjmp)
	RETGUARD_LOAD_RANDOM(longjmp, t6)
	mv	a2, a0				/* move jmpbuf */
	mv	a3, a1				/* final return value */

	/* Restore the signal mask */
	lw	a1, (_JB_SIGMASK * 8)(a2)	/* set */
	beqz	a1, 1f

	add	a0, zero, 3			/* SIG_SETMASK */
	SYSTRAP(sigprocmask)
1:

	mv	a0, a2
	mv	a1, a3

	/* Restore the stack pointer */
	ld	t0, 8(a0)
	mv	sp, t0

	/* Store the general purpose registers and ra */
	ld	s0, 16(a0)
	ld	s1, 24(a0)
	ld	s2, 32(a0)
	ld	s3, 40(a0)
	ld	s4, 48(a0)
	ld	s5, 56(a0)
	ld	s6, 64(a0)
	ld	s7, 72(a0)
	ld	s8, 90(a0)
	ld	s9, 98(a0)
	ld	s10, 106(a0)
	ld	s11, 112(a0)
	ld	ra, 120(a0)

	/* Store the fp registers */
	fld	fs0, 128(a0)
	fld	fs1, 136(a0)
	fld	fs2, 144(a0)
	fld	fs3, 152(a0)
	fld	fs4, 160(a0)
	fld	fs5, 168(a0)
	fld	fs6, 176(a0)
	fld	fs7, 184(a0)
	fld	fs8, 192(a0)
	fld	fs9, 200(a0)
	fld	fs10, 208(a0)
	fld	fs11, 216(a0)
	ld	t0, 232(a0)
	fscsr	t0	

	/* Load the return value */
	add	a0, zero, 1
	beqz	a1, 1f
	mv	a0, a1
1:
	RETGUARD_CHECK(longjmp, t6)
	ret

END_STRONG(siglongjmp)
