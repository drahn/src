/*-
 * Copyright (c) 2020 Brian Bamsch <bbamsch@google.com>
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)SYS.h	5.5 (Berkeley) 5/7/91
 */

#include "DEFS.h"
#include <sys/syscall.h>

#define SYSENTRY(x)					\
	.weak _C_LABEL(x);				\
	_C_LABEL(x) = _C_LABEL(_thread_sys_ ## x);	\
	ENTRY(_thread_sys_ ## x)
#define SYSENTRY_HIDDEN(x)				\
	ENTRY(_thread_sys_ ## x)
#define __END_HIDDEN(x)					\
	END(_thread_sys_ ## x);				\
	_HIDDEN_FALIAS(x, _thread_sys_ ## x);		\
	END(_HIDDEN(x))
#define __END(x)					\
	__END_HIDDEN(x); END(x)

#define SYSTRAP(x) \
	li	t0, SYS_ ## x;		\
	ecall

#define	CERROR		_C_LABEL(__cerror)
#if defined(__pic__) || 1
#define HANDLE_ERROR()							\
	bnez	t0, 1f;							\
	ret;								\
1:	auipc	t0, %got_pcrel_hi(CERROR);				\
	ld	t1, %pcrel_lo(1b)(t0);					\
	jr	t1
#else
#define HANDLE_ERROR()							\
	bnez	t0, 1f;							\
	ret;								\
1:	la	t1, CERROR;						\
	jr	t1
#endif

#define _SYSCALL_NOERROR(x,y)						\
	SYSENTRY(x);							\
	SYSTRAP(y)
#define _SYSCALL_HIDDEN_NOERROR(x,y)					\
	SYSENTRY_HIDDEN(x);						\
	SYSTRAP(y)

#define _SYSCALL(x, y)							\
	_SYSCALL_NOERROR(x,y);						\
	HANDLE_ERROR
#define _SYSCALL_HIDDEN(x, y)						\
	_SYSCALL_HIDDEN_NOERROR(x,y);					\
	HANDLE_ERROR

#define SYSCALL_NOERROR(x)						\
	_SYSCALL_NOERROR(x,x)

#define SYSCALL(x)							\
	_SYSCALL(x,x)


#define PSEUDO_NOERROR(x,y)						\
	SYSENTRY(x);							\
	RETGUARD_SETUP(x, t6);						\
	SYSTRAP(y);							\
	RETGUARD_CHECK(x, t6);						\
	ret;								\
	__END(x)

#define PSEUDO(x,y)							\
	SYSENTRY(x);							\
	RETGUARD_SETUP(x, t6);						\
	SYSTRAP(y);							\
	HANDLE_ERROR();							\
	RETGUARD_CHECK(x, t6);						\
	ret;								\
	__END(x)
#define PSEUDO_HIDDEN(x,y)						\
	SYSENTRY_HIDDEN(x);						\
	RETGUARD_SETUP(x, t6);						\
	SYSTRAP(y);							\
	HANDLE_ERROR();							\
	RETGUARD_CHECK(x, t6);						\
	ret;								\
	__END_HIDDEN(x)

#define RSYSCALL_NOERROR(x)						\
	PSEUDO_NOERROR(x,x)

#define RSYSCALL(x)							\
	PSEUDO(x,x)
#define RSYSCALL_HIDDEN(x)						\
	PSEUDO_HIDDEN(x,x)
#define SYSCALL_END(x)							\
	__END(x)
#define SYSCALL_END_HIDDEN(x)						\
	__END_HIDDEN(x)

	.globl	CERROR
