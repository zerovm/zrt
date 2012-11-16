/*
 * sqlite sample
 * Execute Select query for database passed to stdin and output query result into stdout;
 * In order to do run sqlite3 without a file system virtual fs setup was made;
 * Now can be served only readonly sql queries;
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h> //O_RDONLY
#include <assert.h>

#include "sqlite3/sqlite3.h"
#include "sqlite3/vfs_channel.h" //sqlite virtual file system

#define STDIN 0
#define STDERR 2

/*if define READ_ONLY_SQL enabled then database mounted on READ_ONLY channel
 *will opened in READ_ONLY mode and can serve native read queries; 
*/
#define READ_ONLY_SQL

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

int sqlite_pragma (sqlite3* db, const char* request){
    // get current database version of schema
    static sqlite3_stmt *stmt_pragma;
    int rc=0;

    if( (rc=sqlite3_prepare_v2(db, request, -1, &stmt_pragma, NULL)) == SQLITE_OK) {
        while(sqlite3_step(stmt_pragma) == SQLITE_ROW);
    }
    else{
	rc = sqlite3_errcode(db);
    }
    sqlite3_finalize(stmt_pragma);
    return rc;
}

int open_db(const char* path, sqlite3** db)
{
    int rc=0;

    /*check provided path*/
    struct stat st;
    rc=stat(path, &st);

#ifdef READ_ONLY_SQL
    /*if READ_ONLY  character device or block device specified
     *trying to open channel described by manifest*/
    if ( rc == 0 &&
	 ((st.st_mode&S_IFMT) == S_IFCHR || (st.st_mode&S_IFMT) == S_IFBLK) &&
	 (st.st_mode&S_IRWXU) == S_IRUSR ){
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
    }
    else
#endif
	{
	    fprintf( stderr, "open db in read/write mode\n");
	    rc = sqlite3_open( path,  /* Database filename (UTF-8) */
			       db    /* OUT: SQLite db handle */
			       );
	    rc = sqlite_pragma(*db, "PRAGMA synchronous=OFF;" );
	}
    return rc;
}


/*Issue db request.
 * @param nodename which records are needed
 * @param db_records data structure to get results, should be only valid pointer*/
int exec_query_print_results(sqlite3* db, const char *query_string){
    char *zErrMsg = 0;
    int rc = 0;

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

