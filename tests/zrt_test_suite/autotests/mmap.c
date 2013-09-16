/*mmap test for zrt*/

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include "channels/test_channels.h"

#define MMAP_FILE TEST_FILE
#define DATA_FOR_MMAP DATA_FOR_FILE
#define DATASIZE_FOR_MMAP DATASIZE_FOR_FILE


void mmap_test(off_t offset);
void mmap_test_align(size_t length);

int main(int argc, char**argv){
    /*create data file*/
    CREATE_FILE(MMAP_FILE, DATA_FOR_MMAP, DATASIZE_FOR_MMAP);

    /*test1: test mmap+munmap with 0 offset*/
    mmap_test(0);

    /*test2: test mmap with not 0 offset*/
    mmap_test(10);

    /*test3: test mmap address must be aligned for cases:
      PROT_READ|PROT_WRITE, MAP_ANONYMOUS*/
    mmap_test_align(0x10000-1);
    mmap_test_align(0x10000-1);
    mmap_test_align(0x10000-1);
    mmap_test_align(0x10000-1);
    mmap_test_align(0x10000-1);
    return 0;
}



void mmap_test(off_t offset){
    int fd;
    char* data;
    MMAP_READONLY_SHARED_FILE(MMAP_FILE, offset, &fd, data)

    fprintf(stderr, "expected data:%s\n", DATA_FOR_MMAP+offset);
    fprintf(stderr, "mmaped data  :%s\n", ((char*)data+offset));

    off_t filesize;
    GET_FILE_SIZE(MMAP_FILE, &filesize);
    CMP_MEM_DATA( DATA_FOR_MMAP+offset, data+offset, filesize-offset );

    MUNMAP_FILE(data, filesize);
    CLOSE_FILE(fd);
}

void mmap_test_align(size_t length){
    int32_t addr;
    int ret;
    fprintf(stderr, "length=(%d)0x%x\n", length, length);
    TEST_OPERATION_RESULT(
			  (int)mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, -1, 0),
			  &addr, addr>0);
    int pagesize = sysconf(_SC_PAGE_SIZE);
    int expected = addr&~(pagesize-1)&(pagesize-1);
    fprintf(stderr, "addr=0x%x, pagesize=0x%x, expected=0x%x\n", addr, pagesize, expected);
    TEST_OPERATION_RESULT(
    			  expected,
    			  &ret, ret==0);
    /* TEST_OPERATION_RESULT( */
    /* 			  munmap((void*)addr, length), */
    /* 			  &ret, ret==0); */
}
