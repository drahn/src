/*	$OpenBSD: setjmp.S,v 1.13 2016/05/29 07:59:36 guenther Exp $ */
/*
 * Copyright (c) 1996 Dale Rahn. All rights reserved.
 *
 *   
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */  

#include "SYS.h"
#include <machine/asm.h>

/* int setjmp(jmp_buf env) */

#define JMP_r1	0x08
#define JMP_r14	0x10
#define JMP_r15	0x18
#define JMP_r16	0x20
#define JMP_r17	0x28
#define JMP_r18	0x30
#define JMP_r19	0x38
#define JMP_r20	0x40
#define JMP_r21	0x48
#define JMP_r22	0x50
#define JMP_r23	0x58
#define JMP_r24	0x60
#define JMP_r25	0x68
#define JMP_r26	0x70
#define JMP_r27	0x78
#define JMP_r28	0x80
#define JMP_r29	0x88
#define JMP_r30	0x90
#define JMP_r31	0x98
#define JMP_lr  0xa0
#define JMP_cr  0xa8
#define JMP_ctr	0xb0
#define JMP_xer	0xb8
#define JMP_sig	0xc0

	.section	.openbsd.randomdata,"aw",@progbits
	.balign	4
	.globl	__jmpxor
	.hidden	__jmpxor
__jmpxor:
	.zero	4*2		# (r1, lr)
	END(__jmpxor)
	.type	__jmpxor,@object


ENTRY(setjmp)
	mr	5, 3			/* save jmpbuf addr in r5 */
	li	3, 1			/* how = SIG_BLOCK */
	li	4, 0			/* oset = empty */
	li	0, SYS_sigprocmask
	sc
	std	3, JMP_sig(5)
	mr	3, 5
ENTRY(_setjmp)
	mflr	6
	bcl	20, 31, 1f
1:	mflr	7
	addis	7, 7, __jmpxor-1b@ha
	addi	7, 7, __jmpxor-1b@l
	mtlr	6
	ld	0, 0(7)			/* xor for r1 */
	ld	7, 4(7)			/* xor for lr, overwrite addr */

	/* r1, r14-r31 */
	xor  0, 0, 1			/* use and overwrite the r1 xor */
	std  0, JMP_r1 (3)
	std 14, JMP_r14(3)
	std 15, JMP_r15(3)
	std 16, JMP_r16(3)
	std 17, JMP_r17(3)
	std 18, JMP_r18(3)
	std 19, JMP_r19(3)
	std 20, JMP_r20(3)
	std 21, JMP_r21(3)
	std 22, JMP_r22(3)
	std 23, JMP_r23(3)
	std 24, JMP_r24(3)
	std 25, JMP_r25(3)
	std 26, JMP_r26(3)
	std 27, JMP_r27(3)
	std 28, JMP_r28(3)
	std 29, JMP_r29(3)
	std 30, JMP_r30(3)
	std 31, JMP_r31(3)
	/* cr, lr, ctr, xer */
	mfcr 0
	stw 0, JMP_cr(3)
	/* "mflr 6" done at start of _setjmp() */
	xor  7, 6, 7			/* use and overwrite the lr xor */
	std 7, JMP_lr(3)
	mfctr 0
	std 0, JMP_ctr(3)
	mfxer 0
	std 0, JMP_xer(3)
	/* f14-f31, fpscr */
	li 3, 0
	blr
END(_setjmp)
END(setjmp)


ENTRY(longjmp)
	mr	%r5, %r3			/* save jmpbuf addr in r5 */
	mr	%r6, %r4			/* save val in r6 */
	li	%r3, 3			/* how = SIG_SETMASK */
	ld	%r4, JMP_sig(%r5)		/* oset from the jmpbuf */
	li	%r0, SYS_sigprocmask
	sc
	mr	%r3, %r5			/* restore jmpbuf and val to r3,r4 */
	mr	%r4, %r6

ENTRY(_longjmp)
	bcl	20, 31, 1f
1:	mflr	%r9
	addis	%r9, %r9, __jmpxor-1b@ha
	addi	%r9, %r9, __jmpxor-1b@l
	ld	8, 0(9)			/* xor for r1 */
	ld	9, 4(9)			/* xor for lr, overwrite addr */

	/* r1, r14-r30 */
	ld	0, JMP_r1 (3)
	xor	%r1, %r0, %r8			/* use the r1 xor */
	ld	%r14, JMP_r14(%r3)
	ld	%r15, JMP_r15(%r3)
	ld	%r16, JMP_r16(%r3)
	ld	%r17, JMP_r17(%r3)
	ld	%r18, JMP_r18(%r3)
	ld	%r19, JMP_r19(%r3)
	ld	%r20, JMP_r20(%r3)
	ld	%r21, JMP_r21(%r3)
	ld	%r22, JMP_r22(%r3)
	ld	%r23, JMP_r23(%r3)
	ld	%r24, JMP_r24(%r3)
	ld	%r25, JMP_r25(%r3)
	ld	%r26, JMP_r26(%r3)
	ld	%r27, JMP_r27(%r3)
	ld	%r28, JMP_r28(%r3)
	ld	%r29, JMP_r29(%r3)
	ld	%r30, JMP_r30(%r3)
	ld	%r31, JMP_r31(%r3)
	/* cr, lr, ctr, xer */
	ld	%r8, JMP_cr(%r3)		/* overwrite the r1 xor */
	mtcr	%r8
	ld	%r0, JMP_lr(%r3)
	xor	%r0, %r0, %r9			/* use the lr xor */
	mtlr	%r0
	ld	%r9, JMP_ctr(%r3)		/* overwrite the lr xor */
	mtctr	%r9
	ld	%r0, JMP_xer(%r3)
	mtxer	%r0
	/* f14-f31, fpscr */

	/* if r4 == 0, return 1, not 0 */
	mr	%r3, %r4
	cmpdi	%r4, 0
	bnelr
	li	%r3, 1
	blr
END(_longjmp)
END(longjmp)
