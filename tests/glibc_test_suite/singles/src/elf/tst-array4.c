#include <dlfcn.h>

#define zmain array1_main
#include "tst-array1.c"
#undef zmain

int
zmain (void)
{
  void *handle = dlopen ("tst-array2dep.so", RTLD_LAZY);

  array1_main ();

  if (handle != NULL)
    dlclose (handle);

  return 0;
}
