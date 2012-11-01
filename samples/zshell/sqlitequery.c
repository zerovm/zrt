/*
 * sqlite sample
 * Execute Select query for database passed to stdin and output query result into stdout;
 * In order to do run sqlite3 without a file system virtual fs setup was made;
 * Now can be served only readonly sql queries;
 */

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> //O_RDONLY
#include <assert.h>

#include "sqlite3.h"
#include "vfs_channel.h" //sqlite virtual file system

#define STDIN 0
#define STDERR 2

//#define SQLITE_TRACE_ENABLE

/* In use case it's should not be directly called, but only as callback for sqlite API;
 *@param argc columns count
 *@param argv columns values */
int get_dbrecords_callback(void *not_used, int argc, char **argv, char **az_col_name){
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s\t", az_col_name[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}

#ifdef SQLITE_TRACE_ENABLE
void xTrace(void* param,const char* sql){
    printf("xTrace %p, %s", param, sql);fflush(0);
}
#endif

int open_db(const char* path, sqlite3** db)
{
    int rc=0;

#ifdef READ_ONLY_SQL
    /*open db filename that will be used by sqlite VFS*/
    int database_fd = open( path, O_RDONLY );
    assert( database_fd >= 0 );

    /*setup mode that forcing opening DB in special READ-ONLY mode
      with using VFS feature that natively supported by SQLITE but
      VFS implementation is part of libsqlite3 provided among ZRT library*/
    int regerr = register_fs_channel( database_fd );
    assert( SQLITE_OK == regerr );

    /*Open sqlite db, hosted on our registered VFS, it's used "fake" name
     that will not be used because sqlite was tuned to use database_fd 
     descriptor of already opened file*/
    fprintf( stderr, "open db in read only mode\n");
    rc = sqlite3_open_v2( "fake",      /* fake DB name will not be used */
			  db,         /* OUT: SQLite db handle */
			  SQLITE_OPEN_MAIN_DB|SQLITE_OPEN_READONLY,    /* Flags */
			  FS_VFS_NAME  /* Name of VFS module to use */
			  );
#else
    fprintf( stderr, "open db in read/write mode\n");
    rc = sqlite3_open( path,  /* Database filename (UTF-8) */
    		       db    /* OUT: SQLite db handle */
    		       );
#endif
    return rc;
}

/*Issue db request.
 * @param nodename which records are needed
 * @param db_records data structure to get results, should be only valid pointer*/
int exec_query_print_results(sqlite3* db, const char *query_string){
    char *zErrMsg = 0;
    int rc = 0;

#ifdef SQLITE_TRACE_ENABLE
    void* pArg=NULL; 
    void* trace_func = sqlite3_trace(db, xTrace, pArg);
    fprintf( stderr, "enable trace, returned prev tracer= %p\n", trace_func);
    fprintf( stderr, "sqlite prepare\n");

    sqlite3_stmt *statement_handle = NULL;
    rc=sqlite3_prepare_v2(db, query_string, strlen(query_string), &statement_handle, NULL);
    if(rc!=SQLITE_OK){
	fprintf( stderr, "rc=%d, %s\n", rc, sqlite3_errmsg(db));fflush(0);
	return 1;
    }
    printf("\n compile/step/finalize \n");

    int colcount=sqlite3_column_count(statement_handle);
    while(sqlite3_step(statement_handle)!=SQLITE_DONE){
	for(int i=0;i<colcount;i++) {
	    fprintf( stderr, "%s(%s)=%s |", 
		   sqlite3_column_name(statement_handle,i), 
		   sqlite3_column_decltype(statement_handle,i), 
		   sqlite3_column_text(statement_handle,i));
	}
	fprintf( stderr, "\n");
    }
    rc=sqlite3_finalize(statement_handle);
    if(rc!=SQLITE_OK)	{
	fprintf( stderr,  "rc=%d, %s\n", rc, sqlite3_errmsg(db));fflush(0);
	sqlite3_free(zErrMsg);
	return 1;
    }
#else
    /*DB should be previously opened by open_db, 
      run sql query and handle results and errors*/
    fprintf( stderr, "exec db\n");
    rc = sqlite3_exec(db, query_string, get_dbrecords_callback, NULL, &zErrMsg);
    if ( SQLITE_OK != rc ){
        fprintf( stderr, "Sql statement : %s, exec error text=%s, errcode=%d\n",
                query_string, sqlite3_errmsg(db), rc);
        sqlite3_free(zErrMsg);
        return -2;
    }

#endif //SQLITE_TRACE_ENABLE
    return 0;
}

int run_sql_query_buffer(const char* dbpath, const char *sqldump, size_t dump_size )
{
    int rc =0;
    sqlite3* db = NULL;

    fprintf( stderr, "open db '%s'\n", dbpath);
    rc = open_db(dbpath, &db); 

    /*if error ocured while opening DB*/
    if( rc ){
        fprintf( stderr, 
		 "error opening database, "
		 "errtext %s, errcode=%d\n", 
		 sqlite3_errmsg(db), rc);
    }
    /*if no errors related to opening DB, try to run SQL query */
    else{
	/*enable extended sqlite error code*/
	rc = sqlite3_extended_result_codes(db, 1);
	fprintf(stderr, "sql extended result code enable, err code=%d \n", rc);

	rc = exec_query_print_results(db, sqldump );
	fprintf(stderr, "sql query complete, err code=%d \n", rc);
    }
    
    /*one way to close DB for case with or without errors*/
    sqlite3_close(db);
    fprintf( stderr, "closed db\n");
    fflush(0);

#ifdef READ_ONLY_SQL
    /*just get fd for already opened file*/
    int db_fd = open(dbpath, O_RDONLY);
    close(db_fd); /*release file descriptor for file opened explicitly*/
#endif

    return rc;
}

