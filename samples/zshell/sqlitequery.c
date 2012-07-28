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

/*Issue db request.
 * @param nodename which records are needed
 * @param db_records data structure to get results, should be only valid pointer*/
int exec_query_print_results(const char *query_string){
	sqlite3 *db = NULL;
	char *zErrMsg = 0;
	int rc = 0;

	fprintf( stderr, "open db\n");
	/*Open sqlite db hosted on our registered VFS*/
	rc = sqlite3_open_v2( "fake",  /* Database filename (UTF-8) */
	  &db,                     /* OUT: SQLite db handle */
	    SQLITE_OPEN_CREATE        /* Flags */
	  | SQLITE_OPEN_READWRITE     /* Flags */
	  | SQLITE_OPEN_MAIN_DB       /* Flags */
	  ,FS_VFS_NAME        /* Name of VFS module to use */
	);

	fprintf( stderr, "opened db\n");
	if( rc ){
		fprintf( stderr, "error opening database on provided input, errtext %s, errcode=%d\n", sqlite3_errmsg(db), rc);
		sqlite3_close(db);
		return -1;
	}
	fprintf( stderr, "exec db\n");
	rc = sqlite3_exec(db, query_string, get_dbrecords_callback, NULL, &zErrMsg);
	if ( SQLITE_OK != rc ){
		fprintf( stderr, "Sql statement : %s, exec error text=%s, errcode=%d\n",
				query_string, sqlite3_errmsg(db), rc);
		sqlite3_free(zErrMsg);
		return -2;
	}
	fprintf( stderr, "closing db\n");
	sqlite3_close(db);
	fprintf( stderr, "closed db\n");
	return 0;
}


int run_sql_query_buffer(int fd_database, const char *sqldump, size_t dump_size )
{
	int regerr = register_fs_channel( fd_database );
	assert( SQLITE_OK == regerr );
	int rc = exec_query_print_results( sqldump );
	fprintf(stderr, "sql query complete, err code=%d \n", rc);fflush(0);
	return rc;
}

