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
 * this example can (and hopefully will) be extended to zrt library. information
 * about that library will be placed to zerovm website
 *
 * to extend this example just write code of all zrt_* functions. these functions
 * can use any of zerovm appliances for user code (like input stream, output stream, log,
 * extended attributes e.t.c). any of this can be obtained from the user manifest.
 *
 * note: direct syscalls defined here not only for test purposes. direct nacl syscalls
 *       can be used for "selective syscallback" - this is allow to intercept particular
 *       syscalls while leaving other syscalls to zerovm care (see zrt_mmap()).
 *
 *  Created on: Feb 18, 2012
 *      Author: d'b
 */

#include <stdint.h>
#include <string.h>
#include <unistd.h> /* only for tests */

#include "zvm.h"
#include "zrt.h"
#include "syscallbacks.h"

// ### revise it
#undef main /* prevent misuse macro */

/* entry point for zrt library sample (see "syscall_manager.S" file) */
void syscall_director(void);

/*
 * initialize zerovm api, get the user manifest, install syscallback
 * and invoke user code
 */
int main(int argc, char **argv, char **envp)
{
	struct UserManifest *setup = zvm_init();
	if(setup == NULL) return ERR_CODE;

	/* todo(d'b): replace it with a good engine. should be done asap */
	setup->envp = envp; /* user custom attributes passed via environment */
	zrt_setup( setup );
	if(zvm_syscallback((intptr_t)syscall_director) == 0)
		return ERR_CODE;

	/* call user main() and care about return code */
	return slave_main(argc, argv, envp);
}
