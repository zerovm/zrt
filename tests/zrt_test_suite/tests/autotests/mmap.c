/*
 * mmap test
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <assert.h>

#include "zrtlog.h"
#include "zvm.h"
#include "macro_tests.h"

#define EXPECTED_TRUE  1
#define EXPECTED_FALSE 0
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define MMAP_FILE TEST_FILE
#define DATA_FOR_MMAP DATA_FOR_FILE
#define DATASIZE_FOR_MMAP DATASIZE_FOR_FILE


void sbrk_mmap_test();
void mmap_test_file_mapping(off_t offset);
void mmap_test_unmap(void* unmap_addr, size_t length);
void* mmap_test_align(size_t length, int result_expected);

int main(int argc, char**argv){
    sbrk_mmap_test();
    int ret;
    int pagesize = sysconf(_SC_PAGE_SIZE);
    uint32_t rounded_up_heap_addr = ROUND_UP( (uint32_t)MANIFEST->heap_ptr, sysconf(_SC_PAGESIZE) );
    int rounded_up_heap_size = MANIFEST->heap_size;
    rounded_up_heap_size -= rounded_up_heap_addr - (uint32_t)MANIFEST->heap_ptr;
    int rounded_up_heap_end_addr = rounded_up_heap_addr+rounded_up_heap_size-pagesize;

    LOG_STDERR(ELogAddress, rounded_up_heap_addr, "heap first page" )
    LOG_STDERR(ELogAddress, rounded_up_heap_end_addr, "heap last page" )
    LOG_STDERR(ELogSize,    pagesize, "memory page size" )
    LOG_STDERR(ELogSize,    rounded_up_heap_size, "memory heap size" )

    /*create data file*/
    CREATE_FILE(MMAP_FILE, DATA_FOR_MMAP, DATASIZE_FOR_MMAP);

    /*test1: test mmap+munmap with 0 offset*/
    mmap_test_file_mapping(0);

    /*test2: test mmap with not 0 offset*/
    mmap_test_file_mapping(10);

    /*test3: test mmap address must be aligned to page size.
      PROT_READ|PROT_WRITE, MAP_ANONYMOUS*/
    void* addr;
    void* addr2;
    addr = mmap_test_align(0x10000-1, EXPECTED_TRUE);
    mmap_test_unmap( addr, ROUND_UP(0x10000-1, pagesize));
    /*It is not an error if the indicated range does not contain any mapped pages.*/
    mmap_test_unmap( addr, ROUND_UP(0x10000-1, pagesize));

#define MANY_PAGES_FOR_ALLOC 0x20
    addr2 = mmap_test_align(pagesize*MANY_PAGES_FOR_ALLOC, EXPECTED_TRUE);
    addr = mmap_test_align(pagesize, EXPECTED_TRUE);
    LOG_STDERR(ELogAddress, addr, "maped big block of memory" )
    LOG_STDERR(ELogAddress, addr2+pagesize*MANY_PAGES_FOR_ALLOC, "expected next maped memory address" )
    addr = NULL;
    /*alloc all map pages*/
    int avphys = sysconf(_SC_AVPHYS_PAGES);
    while( avphys-- ){
	addr = mmap_test_align(pagesize, EXPECTED_TRUE);
	fprintf(stderr, "sbrk(0)=%p, mmap_addr=%p, avphys=%d\n", sbrk(0), addr, avphys);
	TEST_OPERATION_RESULT( addr!=NULL&&sbrk(0)<=addr, &ret, ret!=0);
    }

    mmap_test_align(pagesize, EXPECTED_FALSE);
    /*unmap some memory page*/
    mmap_test_unmap( (void*)(rounded_up_heap_addr+rounded_up_heap_size/2), pagesize );
    mmap_test_align(pagesize, EXPECTED_TRUE);

    /*we need to free some memory to properly run some code inside of exit handler*/
    mmap_test_unmap((void*)rounded_up_heap_addr, rounded_up_heap_size);

    return 0;
}



void mmap_test_file_mapping(off_t offset){
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

void* mmap_test_align(size_t length, int result_expected){
    int32_t addr;
    int ret;
    int pagesize = sysconf(_SC_PAGE_SIZE);

    LOG_STDERR(ELogLength,  length,   "anonymous maping length")
    LOG_STDERR(ELogSize,    pagesize, "memory page size" )

    addr = (int32_t)mmap(NULL, length, PROT_READ|PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    int32_t expected_addr = ROUND_UP(addr, pagesize);
    LOG_STDERR(ELogAddress, addr,          "created address maping" )
    LOG_STDERR(ELogAddress, expected_addr, "expected address maping" )

    TEST_OPERATION_RESULT(
    			  expected_addr==addr,
    			  &ret, ret==result_expected);
    return (void*)addr;
}

void mmap_test_unmap(void* unmap_addr, size_t length){
    int ret;
    int pagesize = sysconf(_SC_PAGE_SIZE);
    int expected_addr = ROUND_UP((intptr_t)unmap_addr, pagesize);

    LOG_STDERR(ELogLength,  length,   "unmapping memory block length")
    LOG_STDERR(ELogAddress, expected_addr, "unmapping memory address rounded up" )
    LOG_STDERR(ELogAddress, unmap_addr,"unmapping memory" )

    TEST_OPERATION_RESULT(
			  (int)munmap(unmap_addr, length),
			  &ret, ret==0);
}

void sbrk_mmap_test(){
#define MALLOC_SIZE 0x40000
#define MMAP_SIZE 0x40000
    int i;

    // try to allocate memory via malloc
    void* ptr = malloc(MALLOC_SIZE * sizeof(int));
    // filling with bytes
    for (i=0;i<MALLOC_SIZE;++i)
        ((int*)ptr)[i] = 0xDEAD;
    printf("Allocated via malloc\n");

    // now allocating via mmap
    void *mmap_ptr = mmap(NULL, MMAP_SIZE * sizeof(int), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0);
    for (i=0;i<MMAP_SIZE;++i) 
        ((int*)mmap_ptr)[i] = 0xCAFE;
    printf("Allocated via anon mmap\n");

    for (i=0;i<MALLOC_SIZE;++i) {
	assert( ((int*)ptr)[i] == 0xDEAD );
    }
    return;
}
