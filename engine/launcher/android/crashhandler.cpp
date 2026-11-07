/*
Copyright (C) 2022 nillerusr

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <unwind.h>
#include <android/log.h>
#include <string.h>
#include <signal.h>
#include "tier0/dbg.h"
#include <stdlib.h>
#include <inttypes.h>
#include "libunwind/libunwind.h"

struct sigaction old_sa;

#define IN_LIBGCC2 1 // means we want to define __cxxabiv1::__cxa_demangle
namespace __cxxabiv1
{
	extern "C"
	{
		#include "demangle/cp-demangle.c"
	}
}

#define MAX_FRAMES 2048

struct backtrace_t
{
	int count;
	uintptr_t frames[MAX_FRAMES];
};

#define Log(msg) __android_log_print(ANDROID_LOG_DEBUG, "SRCENG", "%s", msg); DebugLogger()->Write(msg);

void printPC(void *pc)
{
	char message[4096];

	const char* symbol = "unknown";

	Dl_info info = { 0 };
	const char *fname = "unknown";

	if( dladdr(pc, &info) <= 0 )
	{
		snprintf( message, sizeof(message), "0x%" PRIXPTR "\n", (uintptr_t)pc );
		Log(message);
		return;
	}

	if( info.dli_fname )
		fname = info.dli_fname;

	if( info.dli_sname )
		symbol = info.dli_sname;

	int status = 0;
	char *demangled = __cxxabiv1::__cxa_demangle(symbol, 0, 0, &status);

	if( NULL != demangled && 0 == status )
		symbol = demangled;

	uintptr_t relative_addr = (uintptr_t)pc - (uintptr_t)info.dli_fbase;

	snprintf( message, sizeof(message), "0x%" PRIXPTR ":\t%s (base=0x%" PRIXPTR ", %s)\n", relative_addr, symbol, (uintptr_t)info.dli_fbase, fname );
	Log(message);
}

_Unwind_Reason_Code UnwindBacktraceCallback(struct _Unwind_Context* unwind_context, void* state_voidp)
{
	uintptr_t pc = _Unwind_GetIP(unwind_context);
	backtrace_t *bt = (backtrace_t*)state_voidp;

	if( bt->count < MAX_FRAMES )
		bt->frames[bt->count++] = pc;
	else
		return _URC_END_OF_STACK;

	return _URC_NO_REASON;
}

static void CrashHandler( int sig, siginfo_t *si, void *uc)
{
	static char message[4096], symbol[256];
	int len, line, logfd, i = 0;


	const ucontext_t* signal_ucontext = (ucontext_t*)uc;
	const mcontext_t* signal_mcontext = &(signal_ucontext->uc_mcontext);
#ifdef __aarch64__
	// Doesn't work good on armv7a
	// Log the fault first and walk frame pointers from the crash context: this works even when the
	// unwinder can't get past the signal frame.
	Log(">>> crash report begin\n");

	snprintf(message, sizeof(message), "Signal=%d, errno=%d, code=%d, addr=0x%" PRIXPTR "\n", sig, si->si_errno, si->si_code, (uintptr_t)si->si_addr);
	Log(message);

	printPC( (void*)signal_mcontext->pc );
	printPC( (void*)signal_mcontext->regs[30] );

	uintptr_t fp = signal_mcontext->regs[29];
	uintptr_t sp = signal_mcontext->sp;
	for( int i = 0; i < 64; i++ )
	{
		if( fp == 0 || (fp & 7) || fp < sp || fp - sp > 0x4000000 )
			break;
		uintptr_t *frame = (uintptr_t *)fp;
		uintptr_t ret = frame[1] & 0xFFFFFFFFFFFFFFULL;
		if( ret == 0 )
			break;
		printPC( (void*)ret );
		if( frame[0] <= fp )
			break;
		fp = frame[0];
	}
#else
	Log(">>> crash report begin\n");

	snprintf(message, sizeof(message), "Signal=%d, errno=%d, code=%d, addr=0x%" PRIXPTR "\n", sig, si->si_errno, si->si_code, (uintptr_t)si->si_addr);
	Log(message);

	// Initialize unw_context and unw_cursor.
	unw_context_t unw_context = {};
	unw_getcontext(&unw_context);
	unw_cursor_t  unw_cursor = {};
	unw_init_local(&unw_cursor, &unw_context);

	while (unw_step(&unw_cursor) > 0) {
		unw_word_t ip = 0;
		unw_get_reg(&unw_cursor, UNW_REG_IP, &ip);
		printPC( (void*)ip );
	}
#endif

	Log(">>> crash report end\n");

	if (old_sa.sa_sigaction)
		(*old_sa.sa_sigaction)(sig, si, uc);
	else if(old_sa.sa_handler)
		(*old_sa.sa_handler)(sig);
}

void InitCrashHandler()
{
	// Report stack overflows too: the handler needs its own stack for those.
	static char s_AltStack[256 * 1024];
	stack_t ss;
	memset( &ss, 0, sizeof( ss ) );
	ss.ss_sp = s_AltStack;
	ss.ss_size = sizeof( s_AltStack );
	sigaltstack( &ss, NULL );

	struct sigaction act;
	memset( &act, 0, sizeof( act ) );
	sigemptyset( &act.sa_mask );
	act.sa_sigaction = CrashHandler;
	act.sa_flags = SA_SIGINFO | SA_ONSTACK;
	sigaction(SIGSEGV, &act, &old_sa);
	sigaction(SIGABRT, &act, &old_sa);
	sigaction(SIGBUS, &act, &old_sa);
	sigaction(SIGFPE, &act, &old_sa);
	sigaction(SIGTRAP, &act, &old_sa);
}
