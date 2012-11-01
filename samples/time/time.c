/*
 * demonstration of zrt_gettimeofday()
 */
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>

int zmain(int argc, char **argv)
{
  time_t cur_time;
  struct tm *date_time;

  /* call zrt_gettimeofday() indirectly */
  fprintf(stdout, "timestamp from manifest = %ld\n"
      "which means..\n", (long)time(&cur_time));

  /* infere and print the current time and date */
  date_time = localtime(&cur_time);
  fprintf(stdout, "date = %d-%02d-%02d "
      "time = %02d:%02d:%02d %s\n\n",
      date_time->tm_year + 1900,
      date_time->tm_mon + 1,
      date_time->tm_mday,
      date_time->tm_hour,
      date_time->tm_min,
      date_time->tm_sec,
      date_time->tm_zone);

  return 0;
}
