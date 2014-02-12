<h1>
Status of zrt & glibc library
</h1>

<h2>
WON'T IMPLEMENT
</h2>
<pre><code>
&lt;signal.h&gt;
         void (*signal(int sig, void (*func)(int)))(int);
         int raise(int sig);
</code></pre>

<h2>
EXPERIMENTAL FEATURE SET:
</h2>
<pre><code>
&lt;pthread.h&gt; //any pthread function is experimental

&lt;locale.h&gt;
         char *setlocale(int category, const char *locale);
         struct lconv *localeconv(void);
</code></pre>

<h2>
PRODUCTION FEATURE SET:
</h2>
<pre><code>
&lt;errno.h>
&lt;stddef.h>
&lt;assert.h>
&lt;limits.h>
&lt;float.h>

&lt;unistd.h>
       ssize_t pread(int fd, void *buf, size_t count, off_t offset);
       ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);

&lt;ctype.h>
         int isalnum(int c);
         int isalpha(int c);
         int iscntrl(int c);
         int isdigit(int c);
         int isgraph(int c);
         int islower(int c);
         int isprint(int c);
         int ispunct(int c);
         int isspace(int c);
         int isupper(int c);
         int isxdigit(int c);
         int tolower(int c);
         int toupper(int c);

&lt;math.h>
         double acos(double x);
         double asin(double x);
         double atan(double x);
         double atan2(double y, double x);
         double cos(double x);
         double sin(double x);
         double tan(double x);
         double cosh(double x);
         double sinh(double x);
         double tanh(double x);
         double exp(double x);
         double frexp(double value, int *exp);
         double ldexp(double x, int exp);
         double log(double x);
         double log10(double x);
         double modf(double value, double *iptr);
         double pow(double x, double y);
         double sqrt(double x);
         double ceil(double x);
         double fabs(double x);
         double floor(double x);
         double fmod(double x, double y);

&lt;setjmp.h>
         jmp_buf
         int setjmp(jmp_buf env);
         void longjmp(jmp_buf env, int val);

&lt;stdarg.h>
         va_list
         void va_start(va_list ap,  parmN);
          type va_arg(va_list ap,  type);
         void va_end(va_list ap);

&lt;stdio.h>
         int remove(const char *filename);
         int rename(const char *old, const char *new);
         FILE *tmpfile(void);
         char *tmpnam(char *s);
         int fclose(FILE *stream);
         int fflush(FILE *stream);
         FILE *fopen(const char *filename, const char *mode);
         FILE *freopen(const char *filename, const char *mode, FILE *stream);
         void setbuf(FILE *stream, char *buf);
         int setvbuf(FILE *stream, char *buf, int mode, size_t size);
         int fprintf(FILE *stream, const char *format, ...);
         int fscanf(FILE *stream, const char *format, ...);
         int printf(const char *format, ...);
         int scanf(const char *format, ...);
         int sprintf(char *s, const char *format, ...);
         int sscanf(const char *s, const char *format, ...);
         int vfprintf(FILE *stream, const char *format, va_list arg);
         int vprintf(const char *format, va_list arg);
         int vsprintf(char *s, const char *format, va_list arg);
         int fgetc(FILE *stream);
         char *fgets(char *s, int n, FILE *stream);
         int fputc(int c, FILE *stream);
         int fputs(const char *s, FILE *stream);
         int getc(FILE *stream);
         int getchar(void);
         char *gets(char *s);
         int putc(int c, FILE *stream);
         int putchar(int c);
         int puts(const char *s);
         int ungetc(int c, FILE *stream);
         size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
         size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
         int fgetpos(FILE *stream, fpos_t *pos);
         int fseek(FILE *stream, long int offset, int whence);
         int fsetpos(FILE *stream, const fpos_t *pos);
         long int ftell(FILE *stream);
         void rewind(FILE *stream);
         void clearerr(FILE *stream);
         int feof(FILE *stream);
         int ferror(FILE *stream);
         void perror(const char *s);

&lt;stdlib.h>
         double atof(const char *nptr);
         int atoi(const char *nptr);
         long int atol(const char *nptr);
         double strtod(const char *nptr, char **endptr);
         long int strtol(const char *nptr, char **endptr, int base);
         unsigned long int strtoul(const char *nptr, char **endptr, int base);
         int rand(void);
         void srand(unsigned int seed);
         void *calloc(size_t nmemb, size_t size);
         void free(void *ptr);
         void *malloc(size_t size);
         void *realloc(void *ptr, size_t size);
         void abort(void);
         int atexit(void (*func)(void));
         void exit(int status);
         char *getenv(const char *name);
         int system(const char *string);
         void *bsearch(const void *key, const void *base,
                  size_t nmemb, size_t size,
                  int (*compar)(const void *, const void *));
         void qsort(void *base, size_t nmemb, size_t size,
                  int (*compar)(const void *, const void *));
         int abs(int j);
         div_t div(int numer, int denom);
         long int labs(long int j);
         ldiv_t ldiv(long int numer, long int denom);
         int mblen(const char *s, size_t n);
         int mbtowc(wchar_t *pwc, const char *s, size_t n);
         int wctomb(char *s, wchar_t wchar);
         size_t mbstowcs(wchar_t *pwcs, const char *s, size_t n);
         size_t wcstombs(char *s, const wchar_t *pwcs, size_t n);

&lt;string.h>
         NULL
         size_t
         void *memcpy(void *s1, const void *s2, size_t n);
         void *memmove(void *s1, const void *s2, size_t n);
         char *strcpy(char *s1, const char *s2);
         char *strncpy(char *s1, const char *s2, size_t n);
         char *strcat(char *s1, const char *s2);
         char *strncat(char *s1, const char *s2, size_t n);
         int memcmp(const void *s1, const void *s2, size_t n);
         int strcmp(const char *s1, const char *s2);
         int strcoll(const char *s1, const char *s2);
         int strncmp(const char *s1, const char *s2, size_t n);
         size_t strxfrm(char *s1, const char *s2, size_t n);
         void *memchr(const void *s, int c, size_t n);
         char *strchr(const char *s, int c);
         size_t strcspn(const char *s1, const char *s2);
         char *strpbrk(const char *s1, const char *s2);
         char *strrchr(const char *s, int c);
         size_t strspn(const char *s1, const char *s2);
         char *strstr(const char *s1, const char *s2);
         char *strtok(char *s1, const char *s2);
         void *memset(void *s, int c, size_t n);
         char *strerror(int errnum);
         size_t strlen(const char *s);

&lt;time.h>
         clock_t clock(void);
         double difftime(time_t time1, time_t time0);
         time_t mktime(struct tm *timeptr);
         time_t time(time_t *timer);
         char *asctime(const struct tm *timeptr);
         char *ctime(const time_t *timer);
         struct tm *gmtime(const time_t *timer);
         struct tm *localtime(const time_t *timer);
         size_t strftime(char *s, size_t maxsize,
                  const char *format, const struct tm *timeptr);


 
</code></pre>
