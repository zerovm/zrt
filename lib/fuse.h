#ifndef __FUSE_INTERFACE_H__
#define __FUSE_INTERFACE_H__

#include <sys/uio.h> /*struct uiovec*/

struct stat;
struct statvfs;
struct flock;

/*FUSE section*/

struct fuse_file_info {
    int flags;
    unsigned int direct_io : 1;
    unsigned int keep_cache : 1;
    unsigned int flush : 1;
    unsigned int nonseekable : 1;
    unsigned int flock_release : 1;
    unsigned int padding : 27;
    uint64_t fh;
    uint64_t lock_owner;
};

struct fuse_args {
	int argc;
	char **argv;
	int allocated;
};

struct fuse_opt {
	const char *templ;
	unsigned long offset;
	int value;
};

struct fuse_chan;
struct fuse_chan_ops {
	int (*receive)(struct fuse_chan **chp, char *buf, size_t size);
	int (*send)(struct fuse_chan *ch, const struct iovec iov[],
		    size_t count);
	void (*destroy)(struct fuse_chan *ch);
};

struct fuse_chan {
	struct fuse_chan_ops op;
	struct fuse_session *se;
	int fd;
	size_t bufsize;
	void *data;
	int compat;
};




typedef int (*fuse_opt_proc_t)(void *data, const char *arg, int key,
			       struct fuse_args *outargs);
typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
				const struct stat *stbuf, off_t off);

/**
 * FUSE interface subset.
 */
struct fuse_operations {
	void *(*init) (void *conn);
	void (*destroy) (void *);

	int (*getattr) (const char *, struct stat *);
	int (*readlink) (const char *, char *, size_t);
	int (*mknod) (const char *, mode_t, dev_t);
	int (*mkdir) (const char *, mode_t);
	int (*unlink) (const char *);
	int (*rmdir) (const char *);
	int (*symlink) (const char *, const char *);
	int (*rename) (const char *, const char *);
	int (*link) (const char *, const char *);
	int (*chmod) (const char *, mode_t);
	int (*chown) (const char *, uid_t, gid_t);
	int (*truncate) (const char *, off_t);
	int (*open) (const char *, struct fuse_file_info *);
	int (*read) (const char *, char *, size_t, off_t,
		     struct fuse_file_info *);
	int (*write) (const char *, const char *, size_t, off_t,
		      struct fuse_file_info *);
	int (*statfs) (const char *, struct statvfs *);
	int (*flush) (const char *, struct fuse_file_info *);
	int (*release) (const char *, struct fuse_file_info *);
	int (*fsync) (const char *, int, struct fuse_file_info *);
	int (*setxattr) (const char *, const char *, const char *, size_t, int);
	int (*getxattr) (const char *, const char *, char *, size_t);
	int (*listxattr) (const char *, char *, size_t);
	int (*removexattr) (const char *, const char *);
	int (*opendir) (const char *, struct fuse_file_info *);
	int (*readdir) (const char *, void *, fuse_fill_dir_t, off_t,
			struct fuse_file_info *);
	int (*releasedir) (const char *, struct fuse_file_info *);
	int (*fsyncdir) (const char *, int, struct fuse_file_info *);
	int (*access) (const char *, int);
	int (*create) (const char *, mode_t, struct fuse_file_info *);
	int (*ftruncate) (const char *, off_t, struct fuse_file_info *);
	int (*fgetattr) (const char *, struct stat *, struct fuse_file_info *);
	int (*lock) (const char *, struct fuse_file_info *, int cmd,
		     struct flock *);
	int (*utimens) (const char *, const struct timespec tv[2]);
	int (*ioctl) (const char *, int cmd, void *arg,
		      struct fuse_file_info *, unsigned int flags, void *data);
	int (*flock) (const char *, struct fuse_file_info *, int op);
	int (*fallocate) (const char *, int, off_t, off_t,
			  struct fuse_file_info *);
};

/*FUSE API*/
int fuse_opt_parse(struct fuse_args *args, void *data,
		   const struct fuse_opt opts[], fuse_opt_proc_t proc);
int fuse_opt_add_arg(struct fuse_args *args, const char *arg);
int fuse_parse_cmdline(struct fuse_args *args, char **mountpoint,
		       int *multithreaded, int *foreground);
int fuse_chan_fd(struct fuse_chan *ch);
struct fuse *fuse_new_common(struct fuse_chan *ch, struct fuse_args *args,
			     const struct fuse_operations *op,
			     size_t op_size, void *user_data, int compat);
void fuse_remove_signal_handlers(struct fuse_session *se);
int fuse_set_signal_handlers(struct fuse_session *se);
int fuse_main_real(int argc, char *argv[], const struct fuse_operations *op,
		   size_t op_size, void *user_data);
int fuse_opt_add_arg(struct fuse_args *args, const char *arg);
struct fuse_session *fuse_get_session(struct fuse *f);
int fuse_loop_mt(struct fuse *f);
struct fuse_session *fuse_get_session(struct fuse *f);
int fuse_daemonize(int foreground);
void fuse_unmount(const char *mountpoint, struct fuse_chan *ch);
void fuse_destroy(struct fuse *f);
int fuse_loop(struct fuse *f);
void fuse_unmount_compat22(const char *mountpoint);
#    define fuse_unmount fuse_unmount_compat22
void fuse_unmount_compat22(const char *mountpoint);
#define fuse_unmount fuse_unmount_compat22

#define fuse_new_compat25 fuse_new 
#define fuse_new_compat22 fuse_new
#define fuse_new_compat2 fuse_new
#define fuse_new_compat1 fuse_new


/*ZRT API*/

enum {ESqlarfs =0, 
      EFusefsCount 
};

/*Supported fuse file systems.  Uncomment FUSE_XXXX_NAME macro to get
 related filesystem compiled into zrt*/

#define FUSE_SQLARFS 0
#define FUSE_ARCHIVEMOUNT 1

//#define FUSE_SQLARFS_NAME "sqlarfs"
#define FUSE_SQLARFS_ARGS_FILL(argc_p, argv_p)	\
    *(argc_p) = 0;				\
    (*(argv_p))[*(argc_p)++] = "sqlarfs";	\
    (*(argv_p))[*(argc_p)++] = "param1";	\
    (*(argv_p))[*(argc_p)++] = "param2";
extern int sqlarfs_main(int, char**);

//#define FUSE_ARCHIVEMOUNT_NAME "archivemount"
#define FUSE_ARCHIVEMOUNT_ARGS_FILL(argc_p, argv_p)	\
    *(argc_p) = 0;					\
    (*(argv_p))[*(argc_p)++] = "archivemount";		\
    (*(argv_p))[*(argc_p)++] = "param1";		\
    (*(argv_p))[*(argc_p)++] = "param2";

typedef int (*fs_main)(int, char**);

extern int archivemount_main(int, char**);


fs_main
fusefs_entrypoint_get_args_by_fsname(const char *fsname, 
				     int *argc, char ***argv);

int fuse_main(int argc, char **argv, struct fuse_operations *op, void *user_data);

/*Run userfs prefix_main() as part of zrt infrastructure.*/
int mount_fusefs(const char *mount_point,
		 int (*fs_main)(int, char**), int fs_argc, char **fs_argv);
/*Initiate mount close, actually it releases cond lock in fuse_main*/
void mount_exit();


#endif //__FUSE_INTERFACE_H__

