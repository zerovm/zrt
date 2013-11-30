/* Install given context.
   Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Andreas Jaeger <aj@suse.de>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include "context.h"

/*  int __setcontext (const ucontext_t *ucp)

  Restores the machine context in UCP and thereby resumes execution
  in that context.

  This implementation is intended to be used for *synchronous* context
  switches only.  Therefore, it does not have to restore anything
  other than the PRESERVED state.  */

ENTRY(setcontext)
	/* Restore registers.  */

	movl %nacl:(OFF+REG_RSP*4)(%r15,%rdi),%r8d
	movl %nacl:(OFF+REG_RBP*4)(%r15,%rdi),%r9d
	movl %nacl:(OFF+REG_RIP*4)(%r15,%rdi),%r11d

	/* We add unwind information for the target here.  */
	# cfi_def_cfa(%rdi, 0)
	# cfi_register(%rsp,%r8)
	# cfi_register(%rbp,%r9)
	# cfi_register(%rip,%r11)
	# cfi_offset(%rbx,REG_RBX*8)
	# cfi_offset(%rbp,REG_RBP*8)
	# cfi_offset(%r12,REG_R12*8)
	# cfi_offset(%r13,REG_R13*8)
	# cfi_offset(%r14,REG_R14*8)
	# cfi_offset(%rsp,REG_RSP*8)
	# cfi_offset(%rip,REG_RIP*8)

	movl %nacl:(OFF+REG_RBX*4)(%r15,%rdi),%ebx
	movl %nacl:(OFF+REG_R12*4)(%r15,%rdi),%r12d
	movl %nacl:(OFF+REG_R13*4)(%r15,%rdi),%r13d
	movl %nacl:(OFF+REG_R14*4)(%r15,%rdi),%r14d
	movl %nacl:(OFF+REG_RSI*4)(%r15,%rdi),%esi
	movl %nacl:(OFF+REG_RDX*4)(%r15,%rdi),%edx
	movl %nacl:(OFF+REG_RCX*4)(%r15,%rdi),%ecx

	xorl %eax, %eax

	naclrestsp %r8d,%r15
	naclrestbp %r9d,%r15
	movl %nacl:(OFF+REG_R8*4)(%r15,%rdi),%r8d
	movl %nacl:(OFF+REG_R9*4)(%r15,%rdi),%r9d
	movl %nacl:(OFF+REG_RDI*4)(%r15,%rdi),%edi
	nacljmp %r11d,%r15


