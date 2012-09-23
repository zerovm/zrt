/*
 * Tests for zerovm network protocol layer
 *  Author: YaroslavLitvinov
 *  Date: 7.03.2012
 */

#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "gtest/gtest.h"
extern "C" {
#include "user_implem.h"
#include "map_reduce_lib.h"
#include "channels_conf.h"
#include "common_channels_conf.h"
}

#define MAX_UINT32 4294967295

// Test harness for routines in zmq_netw.c
class MapReduceLibTests : public ::testing::Test {
public:
	MapReduceLibTests(){}
	~MapReduceLibTests(){}
	virtual void SetUp();
	virtual void TearDown();
};


void MapReduceLibTests::SetUp() {
}

void MapReduceLibTests::TearDown() {
}

/*file size on filesystem*/
static uint64_t get_file_size(const char *name)
{
  struct stat fs;
  int handle;
  int i;

  handle = open(name, O_RDONLY);
  if(handle < 0) return -1;

  i = fstat(handle, &fs);
  close(handle);
  return i < 0 ? -1 : fs.st_size;
}

KeyType*
alloc_array_fill_random( int array_len ){
	KeyType *unsorted_array = (KeyType*)malloc( sizeof(KeyType)*array_len );

	pid_t pid = getpid();
	//fill array by random numbers
	srand((time_t)pid );
	for (int i=0; i<array_len; i++){
		unsorted_array[i]=rand();
	}
	return unsorted_array;
}

int
quicksort_uint32_comparator( const void *m1, const void *m2 ){

	const KeyType *t1= (KeyType* const)(m1);
	const KeyType *t2= (KeyType* const)(m2);

	if ( *t1 < *t2 )
		return -1;
	else if ( *t1 > *t2 )
		return 1;
	else return 0;
	return 0;
}




//TEST_F(MapReduceLibTests, TestDataProvider){
//	uint64_t fsize = get_file_size( "data/1input.txt" );
//	int fd = open( "sample.txt", O_RDONLY );
//	ASSERT_TRUE( fd >= 0 );
//
//	char *buffer = NULL;
//	size_t readed_size = 0;
//	size_t buf_returned_size;
//	size_t buf_request_size = 1024; //1KB
//	int count = 100;
//	do{
//		buf_returned_size = MapInputDataProvider( fd, &buffer, buf_request_size, buf_request_size );
//		//if waiting less then requested
//		if ( (fsize - readed_size) < buf_request_size ){
//			ASSERT_EQ( fsize - readed_size, buf_returned_size  );
//		}
//		else{
//			ASSERT_EQ( buf_request_size, buf_returned_size  );
//		}
//		readed_size += buf_returned_size;
//		count--;
//		if ( !count ){
//			ASSERT_TRUE(count);
//		}
//	}while( buf_returned_size > 0 );
//
//	close(fd);
//}


TEST_F(MapReduceLibTests, TestHistogramsSummarize) {
	srand((time_t)getpid());
	const int histograms_count = 3;
	Histogram histograms[histograms_count];
	KeyType* data[histograms_count];
	int data_lens[histograms_count];
	int generated_count =0;
	int rando = 0;
	uint32_t all_histograms_size=0;
	uint32_t all_data_size=0;
	//generate data and histograms based on data
	do{
		int foo;
		int ii=0;
		do {
			rando = rand();
			foo = rando % 100000;
			rando = rand();
			data_lens[generated_count] = rando % 100000;
			ii++;
		}while( abs(data_lens[generated_count] - foo) < 40000 );
		data[generated_count] = alloc_array_fill_random( data_lens[generated_count] );
		all_data_size += data_lens[generated_count];
		//sort array
		qsort( data[generated_count], data_lens[generated_count], sizeof(KeyType), quicksort_uint32_comparator );

		//generate histogram with offset
		size_t hist_step = data_lens[generated_count] / 100 / histograms_count;
		AllocHistogram( data[generated_count], data_lens[generated_count], hist_step, &histograms[generated_count] );

		all_histograms_size += (histograms[generated_count].array_len-1)*histograms[generated_count].step_hist_common;
		if ( histograms[generated_count].array_len > 1 )
			all_histograms_size += histograms[generated_count].step_hist_last;

		generated_count++;
	}while( generated_count != histograms_count );

	/*test that data size = data size represented by histogram*/
	ASSERT_EQ( all_data_size, all_histograms_size );

	size_t dividers_count = 8;
	KeyType dividers[dividers_count];
	SummarizeHistograms( histograms, histograms_count, dividers_count, dividers );
	printf("\ndividers=[");
	for( int i=0; i < dividers_count; i++ ){
		printf(" %u", dividers[i]);
	}
	printf("]\n"); fflush(0);
	for ( int i=0; i < histograms_count; i++ ){
		printf( "histogram#%d items count=%u, stepf=%d, stepl=%d\n",
				i, (uint32_t)histograms[i].array_len, histograms[i].step_hist_common, histograms[i].step_hist_last );
		free(histograms[i].array);
	}
}


TEST_F(MapReduceLibTests, TestMapCallEvent) {
	/*setup channels conf, now used static data but should be replaced by data from zrt*/
	int map_nodes_list=1;
	int reduce_nodes_list=4;
	struct ChannelsConfInterface chan_if;
	SetupChannelsConfInterface( &chan_if, 1 );
	chan_if.NodesListAdd( &chan_if, EMapNode, &map_nodes_list, 1 );
	chan_if.NodesListAdd( &chan_if, EReduceNode, &reduce_nodes_list, 4 );
	FillUserChannelsList( &chan_if ); /*temp. setup distributed conf, by static data */
	/*--------------*/

	struct MapReduceUserIf mr_if;
	InitInterface( &mr_if );

	InitInternals( &mr_if, &chan_if);

	Buffer keys;
	Buffer values;

	int fd = open( "data/1input.txt", O_RDONLY );
	SetInputChannelFd(fd);

	MapCallEvent(&chan_if, &keys, &values);

	close(fd);
}


int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
