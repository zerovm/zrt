#include <stdio.h>
#include <stdlib.h>

static char str[] = "SOMETHING_NOBODY_USES=something_else";

static int
do_test (void)
{
  if (putenv (str) != 0)
    {
      puts ("putenv failed");
      return 1;
    }

  char *p = getenv ("SOMETHING_NOBODY_USES");
  if (p == NULL)
    {
      puts ("envvar not defined");
      return 1;
    }

  return 0;
}

#define TEST_FUNCTION do_test ()
#include "../test-skeleton.c"
