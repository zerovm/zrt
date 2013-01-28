/*
 * zrt library. simple release
 *
 * this source file demonstrates "syscallback" mechanism. user can
 * install it's own handler for all syscalls (except syscall #0).
 * when installed, "syscallback" will pass control to user code whenever
 * syscalls invoked. to return control to zerovm user can use trap (syscall #0)
 * or uninstall "syscallback" (samples how to do it can be found in this code
 * as well).
 *
 * note: direct syscalls defined here not only for test purposes. direct nacl syscalls
 *       can be used for "selective syscallback" - this is allow to intercept particular
 *       syscalls while leaving other syscalls to zerovm care (see zrt_mmap()).
 *
 *  Created on: Feb 18, 2012
 *      Author: d'b, YaroslavLitvinov
 */

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "zvm.h"
#include "zrt.h"
#include "zrtsyscalls.h"
#include "zrtlog.h"

/*
 * initialize zerovm api, get the user manifest, install syscallback
 * and invoke user code
 */
int main(int argc, char** argv, char** envp)
{
    int i;
    struct UserManifest *setup = zvm_init();
    if(setup == NULL) return ERR_CODE;

    /* user custom attributes passed via environment. 
     * do we need this? */
    setup->envp = envp;
    zrt_setup( setup );

    /* debug print */
    ZRT_LOG(L_SHORT, "DEBUG INFORMATION FOR '%s' NODE", argv[0]);
    ZRT_LOG(L_SHORT, "user heap pointer address = 0x%x", (intptr_t)setup->heap_ptr);
    ZRT_LOG(L_SHORT, "user memory size = %u", setup->heap_size);
    ZRT_LOG_DELIMETER;
    ZRT_LOG(L_SHORT, "sizeof(struct ZVMChannel) = %d", sizeof(struct ZVMChannel));
    ZRT_LOG(L_SHORT, "channels count = %d", setup->channels_count);
    ZRT_LOG_DELIMETER;
    /*print environment variables*/
    i=0;
    while( envp[i] ){
        ZRT_LOG(L_SHORT, "envp[%d] = '%s'", i, envp[i]);
	++i;
    }
    /*print environment variables*/
    i=0;
    while( argv[i] ){
        ZRT_LOG(L_SHORT, "argv[%d] = '%s'", i, argv[i]);
	++i;
    }
    ZRT_LOG_DELIMETER;
    /*print channels list*/
    for(i = 0; i < setup->channels_count; ++i)
    {
        ZRT_LOG(L_SHORT, "channel[%d].name = '%s'", i, setup->channels[i].name);
        ZRT_LOG(L_SHORT, "channel[%d].type = %d", i, setup->channels[i].type);
        ZRT_LOG(L_SHORT, "channel[%d].size = %lld", i, setup->channels[i].size);
        ZRT_LOG(L_SHORT, "channel[%d].limits[GetsLimit] = %lld", i, setup->channels[i].limits[GetsLimit]);
        ZRT_LOG(L_SHORT, "channel[%d].limits[GetSizeLimit] = %lld", i, setup->channels[i].limits[GetSizeLimit]);
        ZRT_LOG(L_SHORT, "channel[%d].limits[PutsLimit] = %lld", i, setup->channels[i].limits[PutsLimit]);
        ZRT_LOG(L_SHORT, "channel[%d].limits[PutSizeLimit] = %lld", i, setup->channels[i].limits[PutSizeLimit]);
    }
    ZRT_LOG_DELIMETER;
    ZRT_LOG(L_SHORT, "_SC_PAGE_SIZE=%ld", sysconf(_SC_PAGE_SIZE));

    if(zvm_syscallback((intptr_t)syscall_director) == 0)
        return ERR_CODE;

    zrt_setup_finally();

    /* call user main() and care about return code */
    return zmain(argc, argv);
}

