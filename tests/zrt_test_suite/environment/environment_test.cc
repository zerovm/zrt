/*
 * Tests for zrt
 *  Author: YaroslavLitvinov
 *  Date: 26.07.2012
 */

#include "zrt.h"

#include "gtest/gtest.h"

// Test harness for routines in zmq_netw.c
class ZrtTests : public ::testing::Test {
public:
	ZrtTests(){}
	~ZrtTests(){}
	virtual void SetUp();
	virtual void TearDown();
};


void ZrtTests::SetUp() {
}

void ZrtTests::TearDown() {
}


TEST_F(ZrtTests, TestEnvironment) {
	//EXPECT_EQ( 0, 0 );
}

extern "C"{
int main(int argc, char **argv, char **envp) {
	WRITE_LOG( "main\n" );
	//testing::InitGoogleTest(&argc, argv);
	//return RUN_ALL_TESTS();
	return 0;
}
}

