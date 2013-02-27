/*
 * this sample demonstrate zrt library - simple way to use libc
 * from untrusted code.
 */
#include <stdio.h>
#include <stdlib.h>

#include <wchar.h>
#include <assert.h>

int main(int argc, char **argv)
{
  /* write to default device (in our case it is stdout) */
  printf("hello, world\n");

  /* write to user log (stderr) */
  fprintf(stderr, "hello, world\n");

  wint_t wc = btowc( EOF );
  if ( wc == WEOF ){
      printf("btowc WEOF\n");
  }
  else{
      wprintf();
  }
  return 0;
}
