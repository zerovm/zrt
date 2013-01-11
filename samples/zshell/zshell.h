/*
 * zshell.h
 *
 *  Created on: 24.07.2012
 *      Author: yaroslav
 */

#ifndef ZSHELL_H_
#define ZSHELL_H_

/*Database file access modes: */
enum { 
    EDbReadOnly,     /*Read only: open mode with random read access*/
    EDbReWritable,   /*Read/Append: open mode with random read access and, 
		       Append only - write data to the end of file, 
		       it has no random write ability*/
    EDbWrong         /*Bad access mode*/
};

/* Run LUA script and pass command line parameters
 * @param buffer buffer containing script data
 * @param buf_size*/
int run_lua_buffer_script (const char* buffer, size_t buf_size, const char** argv);

/* Run PYTHON script from file readed as arg1 and pass command line parameters*/
int
run_python_interpreter(int argc, char **argv);

/* Run SQL query, on specified DB;
 * @param dbpath opening DB  
 * @param open_mode DB can be opened in EDbReadOnly / EDbReWritable access modes
 * @param buffer buffer containing script data
 * @param buf_size
 */
int 
run_sql_query_buffer(
		     const char* dbpath, 
		     int open_mode, 
		     const char* buffer, size_t buf_size );

/*obsolete, run lua source file, it should not have lua ident string '#lua'
 *it loads whole file and uses different launch method than run_lua_buffer_script*/
int run_lua_script (const char* filename);


#endif /* ZSHELL_H_ */
