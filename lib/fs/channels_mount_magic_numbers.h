/*
 * channels_mount_magic_numbers.h
 *
 */

/*synthetic numbers are used by stat function*/


/* blocksize for file system I/O */
#define DEV_BLOCK_DEVICE_BLK_SIZE 4096
#define DEV_CHAR_DEVICE_BLK_SIZE  1
#define DEV_DIRECTORY_BLK_SIZE    1

/* directory total size, in bytes, it's value unused in further  */
#define DEV_DIRECTORY_SIZE        4096

/* ID of device containing file, it's value unused in further  */
#define DEV_DEVICE_ID             2049

/* user ID of owner, it's value unused in further */
#define DEV_OWNER_UID             1000
/* group ID of owner, it's value unused in further */
#define DEV_OWNER_GID             1000
