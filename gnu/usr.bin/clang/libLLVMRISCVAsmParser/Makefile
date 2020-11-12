# $OpenBSD: Makefile,v 1.4 2020/08/03 14:45:23 patrick Exp $

LIB=	LLVMRISCVAsmParser
NOPROFILE=

CPPFLAGS+=	-I${.OBJDIR}/../include/llvm/AArch64 \
		-I${LLVM_SRCS}/lib/Target/AArch64

SRCS+=	RISCVAsmParser.cpp

.PATH:	${.CURDIR}/../../../llvm/llvm/lib/Target/RISCV/AsmParser
