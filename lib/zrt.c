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
//__attribute__((externally_visible))
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
    zrt_log("DEBUG INFORMATION FOR '%s' NODE", argv[0]);
    zrt_log("user heap pointer address = 0x%x", (intptr_t)setup->heap_ptr);
    zrt_log("user memory size = %u", setup->heap_size);
    if ( setup->heap_ptr ){
        zrt_log("calculated heap end address= 0x%x", (intptr_t)setup->heap_ptr+setup->heap_size);
    }
    zrt_log("heap bounds [0x%X-0xFFFFFFFF]", 0xFFFFFFFF-0x1000000);
    ZRT_LOG_DELIMETER;
    zrt_log("sizeof(struct ZVMChannel) = %d", sizeof(struct ZVMChannel));
    zrt_log("channels count = %d", setup->channels_count);
    ZRT_LOG_DELIMETER;
    for(i = 0; i < setup->channels_count; ++i)
    {
        zrt_log("channel[%d].name = '%s'", i, setup->channels[i].name);
        zrt_log("channel[%d].type = %d", i, setup->channels[i].type);
        zrt_log("channel[%d].size = %lld", i, setup->channels[i].size);
        zrt_log("channel[%d].limits[GetsLimit] = %lld", i, setup->channels[i].limits[GetsLimit]);
        zrt_log("channel[%d].limits[GetSizeLimit] = %lld", i, setup->channels[i].limits[GetSizeLimit]);
        zrt_log("channel[%d].limits[PutsLimit] = %lld", i, setup->channels[i].limits[PutsLimit]);
        zrt_log("channel[%d].limits[PutSizeLimit] = %lld", i, setup->channels[i].limits[PutSizeLimit]);
    }
    ZRT_LOG_DELIMETER;

    if(zvm_syscallback((intptr_t)syscall_director) == 0)
        return ERR_CODE;

    zrt_setup_finally();

    /* call user main() and care about return code */
    return zmain(argc, argv);
}

