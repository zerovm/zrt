/*
 * Tests for zrt
 *  Author: YaroslavLitvinov
 *  Date: 26.07.2012
 */

#include "zrt.h"
#include <stdio.h>
#ifdef GTEST
#include "gtest/gtest.h"

// Test harness for routines in zmq_netw.c
class ZrtTests : public ::testing::Test {
public:
	ZrtTests(){}
};


TEST_F(ZrtTests, TestEnvironment) {
	//EXPECT_EQ( 0, 0 );
}
#endif

extern "C"{
int main(int argc, char **argv, char **envp) {
    printf("test\n");fflush(0);
	//testing::InitGoogleTest(&argc, argv);
	//return RUN_ALL_TESTS();
	return 0;
}
}

