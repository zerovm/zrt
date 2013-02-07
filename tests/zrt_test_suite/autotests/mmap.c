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

#include "channels/test_channels.h"

#define MMAP_FILE "/newfileformmap"
#define DATA_FOR_MMAP "something something something something something\0"
#define DATASIZE_FOR_MMAP sizeof(DATA_FOR_MMAP)

void mmap_test(off_t offset);

int zmain(int argc, char**argv){
    int ret;
    int ret2;
    /*create data file*/
    TEST_OPERATION_RESULT(
			  open(MMAP_FILE, O_WRONLY|O_CREAT), 
			  &ret, ret!=-1);
    TEST_OPERATION_RESULT(
			  write(ret, DATA_FOR_MMAP, DATASIZE_FOR_MMAP ), 
			  &ret2, ret2==DATASIZE_FOR_MMAP);
    TEST_OPERATION_RESULT(
			  close(ret), 
			  &ret2, ret2!=-1);
    /*test1: test mmap+munmap with 0 offset*/
    mmap_test(0);

    /*test2: test mmap with not 0 offset*/
    mmap_test(10);
    return 0;
}



void mmap_test(off_t offset)
{
    int ret;
    int* p;
    int** pp = &p;

    fprintf(stderr, "%s offset=%lld\n", __func__, offset);
    TEST_OPERATION_RESULT(
			  open(MMAP_FILE, O_RDONLY), 
			  &ret, ret!=-1);
    TEST_OPERATION_RESULT(
			  mmap(NULL, DATASIZE_FOR_MMAP, PROT_READ, MAP_SHARED, ret, offset), 
			  pp, p!=NULL);
    fprintf(stderr, "expected data:%s\n", DATA_FOR_MMAP+offset);
    fprintf(stderr, "mmaped data  :%s\n", (char*)((char*)p+offset));
    TEST_OPERATION_RESULT(
			  memcmp((char*)DATA_FOR_MMAP+offset, (char*)p+offset, DATASIZE_FOR_MMAP-offset),
			  &ret, ret==0);
    TEST_OPERATION_RESULT(
			  munmap(p, DATASIZE_FOR_MMAP), 
			  &ret, ret!=-1);
}
