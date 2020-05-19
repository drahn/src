/*	$OpenBSD: profile.h,v 1.7 2013/08/19 08:39:30 mpi Exp $ */

/*
 * Copyright (c) 1998 Dale Rahn.
 * All rights reserved.
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
#define	MCOUNT \
	__asm__(" \
		.section \".text\" \n\
		.align 2 \n\
		.globl _mcount \n\
		.type	_mcount,@function \n\
	_mcount: \n\
		ld	%r11, 8(%r1) \n\
		mflr	0 \n\
		std	0, 4(%r1) \n\
		stdu	%r1, -96(%r1) \n\
		std	%r3, 16(%r1) \n\
		std	%r4, 24(%r1) \n\
		std	%r5, 32(%r1) \n\
		std	%r6, 40(%r1) \n\
		std	%r7, 48(%r1) \n\
		std	%r8, 56(%r1) \n\
		std	%r9, 64(%r1) \n\
		std	%r10,72(%r1) \n\
		std	%r11,80(%r1) \n\
		mr	%r4, %r0 \n\
		mr 	%r3, %r11 \n\
		bl __mcount \n\
		ld	%r3, 16(%r1) \n\
		ld	%r4, 24(%r1) \n\
		ld	%r5, 32(%r1) \n\
		ld	%r6, 40(%r1) \n\
		ld	%r7, 48(%r1) \n\
		ld	%r8, 56(%r1) \n\
		ld	%r9, 64(%r1) \n\
		ld	%r10,72(%r1) \n\
		ld	%r11,80(%r1) \n\
		addi	%r1, 1, 96 \n\
		ld	%r0, 8(%r1) \n\
		mtlr	%r11 \n\
		std	%r11, 8(%r1) \n\
		mtctr	0 \n\
		bctr \n\
	.Lfe2: \n\
		.size _mcount, .Lfe2-_mcount \n\
	");
#define _MCOUNT_DECL static void __mcount
#ifdef _KERNEL
#define MCOUNT_ENTER						\
	__asm volatile("mfmsr %0" : "=r"(s));			\
	s &= ~PSL_POW;						\
	__asm volatile("mtmsr %0" :: "r"(s & ~PSL_EE))

#define	MCOUNT_EXIT						\
	__asm volatile("mtmsr %0" :: "r"(s))
#endif /* _KERNEL */
