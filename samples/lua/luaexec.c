/*
 * lua_exec.c
 * Run lua code from C. script got from stdin, result put into stdout.
 *  Created on: 06.07.2012
 *      Author: yaroslav
 */


#include "zrt.h"
#include <lua.h>                                /* Always include this when calling Lua */
#include <lauxlib.h>                            /* Always include this when calling Lua */
#include <lualib.h>                             /* Always include this when calling Lua */

#include <stdlib.h>                             /* For function exit() */
#include <stdio.h>                              /* For input/output */

static void l_message (const char *pname, const char *msg) {
  if (pname) luai_writestringerror("%s: ", pname);
  luai_writestringerror("%s\n", msg);
}


static int report (lua_State *L, int status) {
  if (status != LUA_OK && !lua_isnil(L, -1)) {
    const char *msg = lua_tostring(L, -1);
    if (msg == NULL) msg = "(error object is not a string)";
    l_message( "'stdin lua'", msg);
    lua_pop(L, 1);
    /* force a complete garbage collection in case of errors */
    lua_gc(L, LUA_GCCOLLECT, 0);
  }
  return status;
}

static int do_stdin_file (lua_State *L) {
  int status = luaL_loadfile(L, NULL);
  if ( status )
	  return report(L, status);         /* Error out if Lua file has an error */
  status = lua_pcall(L, 0, 0, 0);                  /* Run the loaded Lua script */
  if ( status )
  	  return report(L, status);         /* Error out if Lua file has an error */
  return LUA_OK;
}


int main(int argc, char **argv)
{
    lua_State *L;

    L = luaL_newstate();                        /* Create Lua state variable */
    luaL_openlibs(L);                           /* Load Lua libraries */

    fprintf(stderr, "In C, calling Lua\n");

    do_stdin_file (L);                          /* executes stdin as a file */

    fprintf(stderr, "Back in C again\n");

    lua_close(L);                               /* Clean up, free the Lua state var */

    fprintf(stderr, "Finish\n");

    return 0;
}
