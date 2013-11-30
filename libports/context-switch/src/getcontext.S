/* Save current context.
   Copyright (C) 2002, 2005 Free Software Foundation, Inc.
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




/*  int __getcontext (ucontext_t *ucp)

  Saves the machine context in UCP such that when it is activated,
  it appears as if __getcontext() returned again.

  This implementation is intended to be used for *synchronous* context
  switches only.  Therefore, it does not have to save anything
  other than the PRESERVED state.  */

#include "context.h"


ENTRY(getcontext)
	/* Save the preserved registers, the registers used for passing
	   args, and the return address.  */
	movl %ebx, %nacl:(OFF+REG_RBX*4)(%r15,%rdi)
	movl %ebp, %nacl:(OFF+REG_RBP*4)(%r15,%rdi)
	movl %r12d, %nacl:(OFF+REG_R12*4)(%r15,%rdi)
	movl %r13d, %nacl:(OFF+REG_R13*4)(%r15,%rdi)
	movl %r14d, %nacl:(OFF+REG_R14*4)(%r15,%rdi)

	movl	%edi, %nacl:(OFF+REG_RDI*4)(%r15,%rdi)
	movl	%esi, %nacl:(OFF+REG_RSI*4)(%r15,%rdi)
	movl	%edx, %nacl:(OFF+REG_RDX*4)(%r15,%rdi)
	movl	%ecx, %nacl:(OFF+REG_RCX*4)(%r15,%rdi)
	movl	%r8d, %nacl:(OFF+REG_R8*4)(%r15,%rdi)
	movl	%r9d, %nacl:(OFF+REG_R9*4)(%r15,%rdi)

	leaq 8(%rsp), %rdx	/* Save SP as it will be after we return.  */
	movl %edx, %nacl:(OFF+REG_RSP*4)(%r15,%rdi)
	movl (%rsp), %eax	/* Save PC we are returning to now.  */
	movl %eax, %nacl:(OFF+REG_RIP*4)(%r15,%rdi)

	/* All done, return 0 for success.  */
	xorl	%eax, %eax
	pop %r11
	nacljmp %r11d,%r15
