#define asconst_SIG_BLOCK 0
#define asconst_SIG_SETMASK 2
#define asconst__NSIG8 8
#define asconst_oRBP 60
#define asconst_oRSP 80
#define asconst_oRBX 64
#define asconst_oR8 20
#define asconst_oR9 24
#define asconst_oR10 28
#define asconst_oR11 32
#define asconst_oR12 36
#define asconst_oR13 40
#define asconst_oR14 44
#define asconst_oR15 48
#define asconst_oRDI 52
#define asconst_oRSI 56
#define asconst_oRDX 68
#define asconst_oRAX 72
#define asconst_oRCX 76
#define asconst_oRIP 84
#define asconst_oEFL 88
#define asconst_oFPREGS 112
#define asconst_oSIGMASK 148
#define asconst_oFPREGSMEM 280
#define asconst_oMXCSR 304
#include <stddef.h>
#include <signal.h>
#include <sys/ucontext.h>

#include <inttypes.h>

#include <stdio.h>

#if __WORDSIZE__ == 64

typedef uint64_t c_t;

#define U(n) UINT64_C (n)

#define PRI PRId64

#else

typedef uint32_t c_t;

#define U(n) UINT32_C (n)

#define PRI PRId32

#endif

static int do_test (void)
{
  int bad = 0, good = 0;

#define TEST(name, source, expr) \
  if (U (asconst_##name) != (c_t) (expr)) { ++bad; fprintf (stderr, "%s: %s is %" PRI " but %s is %"PRI "\n", source, #name, U (asconst_##name), #expr, (c_t) (expr)); } else ++good;

  TEST (SIG_BLOCK, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:7", SIG_BLOCK)
  TEST (SIG_SETMASK, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:8", SIG_SETMASK)
  TEST (_NSIG8, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:10", (_NSIG / 8))
#define ucontext(member)	offsetof (ucontext_t, member)
#define mcontext(member)	ucontext (uc_mcontext.member)
#define mreg(reg)		mcontext (gregs[REG_##reg])
  TEST (oRBP, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:16", mreg (RBP))
  TEST (oRSP, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:17", mreg (RSP))
  TEST (oRBX, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:18", mreg (RBX))
  TEST (oR8, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:19", mreg (R8))
  TEST (oR9, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:20", mreg (R9))
  TEST (oR10, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:21", mreg (R10))
  TEST (oR11, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:22", mreg (R11))
  TEST (oR12, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:23", mreg (R12))
  TEST (oR13, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:24", mreg (R13))
  TEST (oR14, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:25", mreg (R14))
  TEST (oR15, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:26", mreg (R15))
  TEST (oRDI, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:27", mreg (RDI))
  TEST (oRSI, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:28", mreg (RSI))
  TEST (oRDX, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:29", mreg (RDX))
  TEST (oRAX, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:30", mreg (RAX))
  TEST (oRCX, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:31", mreg (RCX))
  TEST (oRIP, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:32", mreg (RIP))
  TEST (oEFL, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:33", mreg (EFL))
  TEST (oFPREGS, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:34", mcontext (fpregs))
  TEST (oSIGMASK, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:35", ucontext (uc_sigmask))
  TEST (oFPREGSMEM, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:36", ucontext (__fpregs_mem))
  TEST (oMXCSR, "../sysdeps/unix/sysv/linux/x86_64/ucontext_i.sym:37", ucontext (__fpregs_mem.mxcsr))
  printf ("%d errors in %d tests\n", bad, good + bad);
  return bad != 0 || good == 0;
}

#define TEST_FUNCTION do_test ()
#include "../test-skeleton.c"
