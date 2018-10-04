/*
 * Copyright (c) 2003-2006 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_OSREFERENCE_LICENSE_HEADER_START@
 * 
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. The rights granted to you under the License
 * may not be used to create, or enable the creation or redistribution of,
 * unlawful or unlicensed copies of an Apple operating system, or to
 * circumvent, violate, or enable the circumvention or violation of, any
 * terms of an Apple operating system software license agreement.
 * 
 * Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 * 
 * @APPLE_OSREFERENCE_LICENSE_HEADER_END@
 */

#include <sys/appleapiopts.h>
#include <machine/cpu_capabilities.h>
#include <machine/commpage.h>
#include <i386/asm.h>

#include <assym.s>

        .text
        .align  2, 0x90

Lmach_absolute_time:
	int	$0x3
	ret

	COMMPAGE_DESCRIPTOR(mach_absolute_time,_COMM_PAGE_ABSOLUTE_TIME,0,0)

/* Nanotime is being moved out of the way of bcopy in the commpage.
 * First we put it in both places, old and new.  Then, when all the build
 * trains have rebuilt libSystem, we can remove the deprecated instance.
 */
 
/* return nanotime in %edx:%eax */
Lold_nanotime:
Lnanotime:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	push	%ebx

0:
	mov	_COMM_PAGE_NT_TSC_BASE,%esi
	mov	_COMM_PAGE_NT_TSC_BASE+4,%edi

	rdtsc
	sub	%esi,%eax
	sbb	%edi,%edx

	mov	_COMM_PAGE_NT_SCALE,%ecx

	mov	%edx,%ebx
	mull	%ecx
	mov	%ebx,%eax
	mov	%edx,%ebx
	mull	%ecx
	add	%ebx,%eax
	adc	$0,%edx

	add	_COMM_PAGE_NT_NS_BASE,%eax
	adc	_COMM_PAGE_NT_NS_BASE+4,%edx

	cmp	_COMM_PAGE_NT_TSC_BASE,%esi
	jne	0b
	cmp	_COMM_PAGE_NT_TSC_BASE+4,%edi
	jne	0b

	pop	%ebx
	pop	%edi
	pop	%esi
	pop	%ebp
	ret

	COMMPAGE_DESCRIPTOR(nanotime,_COMM_PAGE_NANOTIME,0,0)
	COMMPAGE_DESCRIPTOR(old_nanotime,_COMM_PAGE_OLD_NANOTIME,0,0)


/* The 64-bit version.  We return the 64-bit nanotime in %rax,
 * and by convention we must preserve %r9, %r10, and %r11.
 */
	.text
	.align	2
	.code64
Lold_nanotime_64:
Lnanotime_64:				// NB: must preserve r9, r10, and r11
	pushq	%rbp			// set up a frame for backtraces
	movq	%rsp,%rbp
	movq	$_COMM_PAGE_32_TO_64(_COMM_PAGE_NT_TSC_BASE),%rsi
1:
	movq	_NT_TSC_BASE(%rsi),%r8	// r8  := base_tsc
	rdtsc				// edx:eax := tsc
	shlq	$32,%rdx		// rax := ((edx << 32) | eax), ie 64-bit tsc
	orq	%rdx,%rax
	subq	%r8, %rax		// rax := (tsc - base_tsc)
	movl	_NT_SCALE(%rsi),%ecx
	mulq	%rcx			// rdx:rax := (tsc - base_tsc) * scale
	shrdq	$32,%rdx,%rax		// _COMM_PAGE_NT_SHIFT is always 32
	addq	_NT_NS_BASE(%rsi),%rax	// (((tsc - base_tsc) * scale) >> 32) + ns_base
	
	cmpq	_NT_TSC_BASE(%rsi),%r8	// did the data change during computation?
	jne	1b
	popq	%rbp
	ret

	COMMPAGE_DESCRIPTOR(nanotime_64,_COMM_PAGE_NANOTIME,0,0)
	COMMPAGE_DESCRIPTOR(old_nanotime_64,_COMM_PAGE_OLD_NANOTIME,0,0)
