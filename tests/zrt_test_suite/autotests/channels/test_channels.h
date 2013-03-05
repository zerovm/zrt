#ifndef __TEST_CHANNELS_H__
#define __TEST_CHANNELS_H__


#define CHANNEL_NAME_READONLY        "/dev/readonly"
#define CHANNEL_NAME_WRITEONLY       "/dev/writeonly"
#define CHANNEL_NAME_RDWR            "/dev/read-write"

#ifndef EXIT_FAILURE
#define EXIT_FAILURE -1
#endif

#define TEST_FILE "/newfilefortests"
#define DATA_FOR_FILE "something something something something something\0"
#define DATASIZE_FOR_FILE sizeof(DATA_FOR_FILE)

#define DIR_NAME  "somedir"
#define DIR_NAME2 "somedir2"
#define DIR_FILE  "dirfile1"


#define TEST_OPERATION_RESULT(operation, ret_p, expect_expr){		\
	*ret_p = operation;						\
	if ( !(expect_expr) ){ error (EXIT_FAILURE, errno,		\
				      "ERROR: %s unexpected result %d, expected %s\n", \
				      #operation, (int)*ret_p, #expect_expr); \
	}								\
	else{								\
	    fprintf(stderr, "OK: " #operation " %s\n", #expect_expr);	\
	    fflush(0);							\
	}								\
    }

#define CREATE_FILE(filepath, datastr, datasize){		\
    	int ret,ret2;						\
	fprintf(stderr,						\
		"CREATE_FILE filepath=%s, datasize=%lld\n",	\
		filepath, (off_t)datasize );			\
	fflush(0);						\
	/*create data file*/					\
	TEST_OPERATION_RESULT(					\
			      open(filepath, O_WRONLY|O_CREAT, S_IRWXU), \
			      &ret, ret!=-1);			\
	TEST_OPERATION_RESULT(					\
			      write(ret, datastr,datasize ),	\
			      &ret2, ret2==datasize);		\
	TEST_OPERATION_RESULT(					\
			      close(ret),			\
			      &ret2, ret2!=-1);			\
    }

#define CHECK_FILE_EXISTANCE(filepath){			\
    	int ret;					\
	struct stat st;					\
	TEST_OPERATION_RESULT(				\
			      stat(filepath, &st),	\
			      &ret, ret==0);		\
    }
 

#define GET_FILE_SIZE(filepath, filesize_p){		\
    	int ret;					\
	struct stat st;					\
	TEST_OPERATION_RESULT(				\
			      stat(filepath, &st),	\
			      &ret, ret==0);		\
	*filesize_p = st.st_size;			\
    }

#define REMOVE_EXISTING_FILEPATH(filepath){			\
	int ret;						\
	struct stat st;						\
	fprintf(stderr,						\
		"REMOVE_EXISTING_FILEPATH filepath=%s\n",	\
		filepath );					\
	TEST_OPERATION_RESULT(					\
			      unlink(filepath),			\
			      &ret, ret!=-1);			\
	TEST_OPERATION_RESULT(					\
			      stat(filepath, &st),		\
			      &ret, ret==-1);			\
    }

#define CLOSE_FILE(fd){					\
	fprintf(stderr, "CLOSE_FILE fd=%d\n", fd);	\
	fflush(0);					\
	int ret;					\
	TEST_OPERATION_RESULT(				\
			      close(fd),		\
			      &ret, ret != -1 );	\
    }


#define MUNMAP_FILE(data,datasize){			\
	int ret;					\
	fprintf(stderr,					\
		"MUNMAP_FILE data=%p, datasize=%lld\n",	\
		data, (off_t)datasize );		\
	fflush(0);					\
	TEST_OPERATION_RESULT(				\
			      munmap(data, datasize),	\
			      &ret, ret!=-1);		\
    }

#define MMAP_READONLY_SHARED_FILE(filename, offset, fd_p, data_p){	\
	char** pp = (char**)&data_p;					\
	fprintf(stderr,							\
		"MMAP_READONLY_SHARED_FILE filename=%s, offset=%lld\n",	\
		filename, (off_t)offset );				\
	fflush(0);							\
	TEST_OPERATION_RESULT(						\
			      open(filename, O_RDONLY),			\
			      fd_p, *fd_p!=-1);				\
	off_t filesize;							\
	GET_FILE_SIZE(filename,&filesize);				\
	TEST_OPERATION_RESULT(						\
			      mmap(NULL, filesize, PROT_READ, MAP_SHARED, *fd_p, offset), \
			      pp, data_p!=NULL);			\
    }


#define CMP_MEM_DATA(data1, data2, datasize){				\
	int ret;							\
	fprintf(stderr, "CMP_MEM_DATA p1=%p, p2=%p, datasize=%lld\n",	\
		data1, data2, datasize );				\
	fflush(0);							\
	TEST_OPERATION_RESULT(						\
			      memcmp((char*)data1, (char*)data2, datasize), \
			      &ret, ret==0);				\
    }

#define CREATE_EMPTY_DIR(dirpath){			\
    int ret;						\
    fprintf(stderr,					\
	    "CREATE_EMPTY_DIR dirpath=%s\n", dirpath );	\
    fflush(0);						\
    /*create dir*/					\
    TEST_OPERATION_RESULT(				\
			  mkdir(dirpath, S_IRWXU),	\
			  &ret, ret!=-1);		\
}


#define CREATE_NON_EMPTY_DIR(dirpath, justfilename){			\
	int ret;							\
	fprintf(stderr,							\
		"CREATE_NON_EMPTY_DIR dirpath=%s, justfilename=%s\n",	\
		dirpath, justfilename );				\
	fflush(0);							\
	/*create dir*/							\
	TEST_OPERATION_RESULT(						\
			      mkdir(dirpath, S_IRWXU),			\
			      &ret, ret!=-1);				\
	int filenamelen = strlen(dirpath)+strlen(justfilename)+2;	\
	char* filename = (char*)alloca(filenamelen);			\
	snprintf(filename, filenamelen, "%s/%s", dirpath, justfilename); \
	/*create file*/							\
	CREATE_FILE(filename, DATA_FOR_FILE, DATASIZE_FOR_FILE);	\
    }


#define EXPECT_FILESIZE

#endif //__TEST_CHANNELS_H__
