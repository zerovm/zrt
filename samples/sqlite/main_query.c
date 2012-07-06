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

#include "zrt.h"
#include "sqlite3.h"
#include "vfs_channel.h" //sqlite virtual ile system

#define STDIN 0
#define STDERR 2

int add_to_str_unescape(char *dst, const char *src, int n ){
	for ( int i=0; i < n; i++ ){
		/*if escape character then unescape it*/
		if ( src[i] == '#' )
			dst[i] = '=';
		else
			dst[i] = src[i];
	}
	return n;
}


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
	  SQLITE_OPEN_MAIN_DB|SQLITE_OPEN_READONLY,    /* Flags */
	  FS_VFS_NAME        /* Name of VFS module to use */
	);

	fprintf( stderr, "opened db\n");
	if( rc ){
		fprintf( stderr, "error opening database on std input, errtext %s, errcode=%d\n", sqlite3_errmsg(db), rc);
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




int main(int argc, char **argv)
{
	assert( SQLITE_OK == register_fs_channel( STDIN ) );
	const int buf_size = 255;
	char buffer[buf_size];
	const char *query_string = 0;
	int rc = 0;

	if ( argc < 2 ){
		query_string = "select * from channels;";
		fprintf(stderr, "SQL QUERY STRING should be passed to application parameters\n\n");
		fprintf(stderr, "default SQL QUERY STRING: %s\n", query_string);
	}
	else{
		/*combine sql query from input parameters passed separately*/
		memset(buffer, '\0',  buf_size);
		int len = 0;
		for ( int i=1; i < argc; i++ ){
			len += add_to_str_unescape( &buffer[len], argv[i], strlen(argv[i]) );
			if ( i+1 < argc ){
				buffer[len] = ' ';
				len += 1;
			}
		}
		query_string = buffer;
		fprintf(stderr, "command line parameter used: SQL QUERY STRING: '%s'\n", query_string);
	}

	rc = exec_query_print_results( query_string );
	fprintf(stderr, "sql query complete, err code=%d \n", rc);fflush(0);
	return rc;
}

