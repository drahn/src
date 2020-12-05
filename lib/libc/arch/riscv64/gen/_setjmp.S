/*	$OpenBSD$	*/
/* 
 * Copyright (c) 2020 Dale Rahn. All rights reserved.
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


#include "DEFS.h"
#include <machine/setjmp.h>

ENTRY(_setjmp)
	addi	sp, sp, -32
	RETGUARD_SETUP(_setjmp, t6)

	sd	sp, 8(a0)
	/* Store the general purpose registers and lr */
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

#ifndef _STANDALONE
	/* Store the vfp registers */
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
#endif

	/* Return value */
	mv	a0, x0
	RETGUARD_CHECK(_setjmp, t6)
	addi sp, sp, 32
	ret
END_STRONG(_setjmp)

ENTRY(_longjmp)
	RETGUARD_SYMBOL(_longjmp)
	RETGUARD_LOAD_RANDOM(_longjmp, t6)

	/* Restore the stack pointer */
	ld	t0, 8(a0)
	mv	sp, t0

	/* Store the general purpose registers and lr */
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
	sd	ra, 120(a0)

#ifndef _STANDALONE
	/* Store the vfp registers */
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
#endif

	/* Load the return value */
	add	a0, zero, 1
	beqz	a1, 1f
	mv	a0, a1
1:
	RETGUARD_CHECK(_longjmp, t6)
	ret

END_STRONG(_longjmp)
