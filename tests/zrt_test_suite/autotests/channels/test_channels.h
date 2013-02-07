#ifndef __TEST_CHANNELS_H__
#define __TEST_CHANNELS_H__


#define CHANNEL_NAME_READONLY        "/dev/readonly"
#define CHANNEL_NAME_WRITEONLY       "/dev/writeonly"
#define CHANNEL_NAME_RDWR            "/dev/read-write"

#define EXIT_FAILURE -1

#define TEST_OPERATION_RESULT(operation, ret_p, expect_expr){		\
	*ret_p = operation;					\
	if ( !(expect_expr) ) error (EXIT_FAILURE, errno,		\
				     "%s unexpected result %d, expected %s\n", \
				     #operation, *ret_p, #expect_expr);	\
    }


#endif //__TEST_CHANNELS_H__
