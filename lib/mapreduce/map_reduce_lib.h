
#include <stddef.h> //size_t
#include <stdint.h> //uint32_t

#include "buffer.h"

//forward decl
struct ChannelsConfigInterface;

enum { EMapNode=1, EReduceNode=2, EInputOutputNode=3 };

typedef uint32_t KeyType;
#define MIN(a,b) (a < b ? a : b )
#define SPLIT_FILE_SIZE_BYTES (1024*1024)


/*structure intended to wrap key data to get it sortable together with value, because
 * linked key, values data are hold by different buffers and can't be sorted in common way */
struct SortableKeyVal{
	uint32_t key;
	int original_index;
};


/****************************************************
 Histogram related structures and functions */

typedef struct Histogram{
	int src_nodeid;
	size_t array_len;
	uint16_t step_hist_common;
	uint16_t step_hist_last;
	KeyType* array;
} Histogram;


struct MapReduceData{
	DataType keytype;
	DataType valuetype;
	Histogram *histograms_list;
	int  histograms_count; /*histograms count is equal to map nodes count*/
	KeyType *dividers_list; /*divider list is used to divide map data to reducers*/
	int dividers_count;
};

struct MapReduceUserIf{
	/* read input buffer, allocate and fill keys & values arrays.
	 * User is not responsible to free keys & values arrays;
	 * @param last_chunk 1 if last chunk data provided, otherwise 0
	 * @return handled data pos*/
	int (*Map)(const char *data, size_t size, int last_chunk, Buffer *keys, Buffer *values );
	/* Waiting sorted data by keys.
	 * reduce and put reduced into reduced_keys, reduced_values*/
	int (*Combine)( const Buffer *keys, const Buffer *values, Buffer *reduced_keys, Buffer *reduced_values );
	/*reduce and output data into stdout*/
	int (*Reduce)( const Buffer *keys, const Buffer *values );
	/*data*/
	struct MapReduceData data;
};


/*currently pointer functions not used, but real function exist in 'internals' section*/
struct MapNodeEvents{
	/*@return actual data size copied into input_buffer from input(file|stdin) */
	size_t
	(*MapInputDataProvider)( int fd, char **input_buffer, size_t requested_buf_size, int unhandled_data_pos );
	/* @param buf input  buffer
	 * @param buf_size  buffer size
	 * @param result_keys Saving orocessed keys. should be valid pointer.
	 * * @param result_values Save processed values. should be valid pointer.
	 * @return unhandled data buffer pos*/
	size_t
	(*MapInputDataLocalProcessing)( const char *buf, size_t buf_size, int last_chunk, Buffer*result_keys, Buffer *result_values );
	/**/
	void
	(*MapCreateHistogramSendEachToOtherCreateDividersList)(
			struct ChannelsConfigInterface *ch_if, struct MapReduceData *data, const Buffer *input_keys );
	/**/
	void
	(*MapSendKeysValuesToAllReducers)(struct ChannelsConfigInterface *ch_if, int last_data, Buffer *keys, Buffer *values);
};


int MapNodeMain(struct MapReduceUserIf *userif, struct ChannelsConfigInterface *ch_if );
int ReduceNodeMain(struct MapReduceUserIf *userif, struct ChannelsConfigInterface *ch_if );

/****************************************************
 * Inernals**/
void InitInternals(struct MapReduceUserIf *userif, const struct ChannelsConfigInterface *chif, struct MapNodeEvents* ev);
/*sort by key keys and linked values*/
void LocalSort( const Buffer *keys, const Buffer *values, Buffer *sorted_keys, Buffer *sorted_values);
size_t MapInputDataProvider( int fd, char **input_buffer, size_t buf_size, int unhandled_data_pos );
void SummarizeHistograms( Histogram *histograms, int histograms_count, int dividers_count, KeyType *divider_array );
size_t AllocHistogram( const KeyType *array, const int array_len, int step, Histogram *histogram );


//MapReduce library functions
//Reduce
//Comparator
//Progress


