/*
 * Tests for zrt
 *  Author: YaroslavLitvinov
 *  Date: 26.07.2012
 */

#include <stdio.h>
extern "C"{
#include "zrt.h"
} //extern "C"

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


int zmain(int argc, char **argv) {
    printf("test\n");fflush(0);
	//testing::InitGoogleTest(&argc, argv);
	//return RUN_ALL_TESTS();
	return 0;

}

