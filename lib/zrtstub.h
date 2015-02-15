#ifndef __ZRTSTUB_H__
#define __ZRTSTUB_H__

/*to avoid on zvm platform linkage errors */
#ifndef sigaction
#define sigaction(a,b,c) (-1)
#endif

#ifndef waitpid
#define waitpid(a,b,c) (-1)
#endif

#ifndef kill
#define kill(a,b) (-1)
#endif

#ifndef fork
#define fork() (-1)
#endif

#ifndef getgrgid
#define getgrgid(a) (NULL)
#endif

#ifndef getgrgid_r
#define getgrgid_r(a,b,c,d,e) (NULL)
#endif

#ifndef getgrnam
#define getgrnam(a) (NULL)
#endif

#ifndef getgrnam_r
#define getgrnam_r(a,b,c,d,e) (NULL)
#endif

#ifndef dlopen
#define dlopen(a,b) (NULL)
#define dlerror() (NULL)
#define dlsym(a,b) (NULL)
#define dlclose(a) (-1)
#endif


#endif //__ZRTSTUB_H__

