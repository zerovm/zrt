/*
 * pythonexec.c
 * Run python code at zshell
 *  Created on: 06.01.2013
 *      Author: yaroslav
 */

#define _SVID_SOURCE

#include <string.h>
#include <stdio.h>
#include <locale.h>

#include "zshell.h"
#include "Python.h"


int
run_python_interpreter(int argc, char **argv)
{
    wchar_t **argv_copy = (wchar_t **)PyMem_Malloc(sizeof(wchar_t*)*argc);
    /* We need a second copies, as Python might modify the first one. */
    wchar_t **argv_copy2 = (wchar_t **)PyMem_Malloc(sizeof(wchar_t*)*argc);
    int i, res;
    char *oldloc;
    /* 754 requires that FP exceptions run in "no stop" mode by default,
     * and until C vendors implement C99's ways to control FP exceptions,
     * Python requires non-stop mode.  Alas, some platforms enable FP
     * exceptions by default.  Here we disable them.
     */
    if (!argv_copy || !argv_copy2) {
        fprintf(stderr, "out of memory\n");
        return 1;
    }
    oldloc = strdup(setlocale(LC_ALL, NULL));
    setlocale(LC_ALL, "");
    for (i = 0; i < argc; i++) {
        argv_copy[i] = _Py_char2wchar(argv[i], NULL);
	//printf("i=%d, argv[%d]=%s, argc=%d\n", i, i, argv[i], argc);
        if (!argv_copy[i])
            return 1;
        argv_copy2[i] = argv_copy[i];
    }
    setlocale(LC_ALL, oldloc);
    free(oldloc);
    res = Py_Main(argc, argv_copy);
    for (i = 0; i < argc; i++) {
        PyMem_Free(argv_copy2[i]);
    }
    PyMem_Free(argv_copy);
    PyMem_Free(argv_copy2);
    return res;
}
