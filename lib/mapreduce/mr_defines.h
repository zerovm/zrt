/*
 * mr_defines.h
 *
 *  Created on: 14.07.2012
 *      Author: yaroslav
 */

#ifndef MR_DEFINES_H_
#define MR_DEFINES_H_

#ifdef DEBUG
#  define WRITE_FMT_LOG(fmt, args...) {fprintf(stderr, fmt, args); fflush(0);}
#  define WRITE_LOG(str) {fprintf(stderr, "%s\n", str); fflush(0);}
#else
#  define WRITE_FMT_LOG(fmt, args...)
#  define WRITE_LOG(str)
#endif

/*this option disables macros WRITE_LOG_BUFFER */
#define DISABLE_WRITE_LOG_BUFFER

#ifndef DISABLE_WRITE_LOG_BUFFER
#  define WRITE_LOG_BUFFER( mif_p,map )					\
    if ( (map).header.count ){						\
	ElasticBufItemData *data;					\
	for (int i_=0; i_ < (map).header.count; i_++){			\
	    data = (ElasticBufItemData *)BufferItemPointer(&(map), i_);	\
	    WRITE_FMT_LOG( "[%d] %s\n",					\
			   i_, PRINTABLE_HASH(mif_p, &data->key_hash) ); \
	}								\
	fflush(0);							\
    }
#else
#  define WRITE_LOG_BUFFER(mif_p, map)
#endif

#define MAX_UINT32 4294967295
#define HASH_SIZE(mif_p) (mif_p)->data.hash_size
#define ALLOC_HASH_IN_STACK alloca(HASH_SIZE)

typedef int exclude_flag_t;
#define MAP_NODE_NO_EXCLUDE 0
#define MAP_NODE_EXCLUDE 1

#define GET_HISTOGRAM_BY_NODE(mif_p, node_index, histogram_p)			\
    (histogram_p) = &(mif)->data.histograms_list[node_index];

#define PRINTABLE_HASH(mif_p, hash)					\
    ((mif_p)!=NULL && (mif_p)->DebugHashAsString != NULL)?		\
    (mif_p)->DebugHashAsString( (char*)alloca((mif_p)->data.hash_size*2+1), \
				(hash),					\
				(mif_p)->data.hash_size)		\
    :""

#define HASH_COPY(dest, src, size) memcpy((dest), (src), (size))
#define HASH_CMP(mif_p, h1_p, h2_p) (mif_p)->ComparatorHash( (h1_p), (h2_p) )

/*Buffer item size used by mapreduce library*/
#define MRITEM_SIZE(mif_p) ((mif_p)->data.mr_item_size)

#define BOUNDS_OK(item_index, items_count)  (item_index) < (items_count)

#define MALLOC_AND_CHECK(p_p, size)					\
    if ( !(p_p) = malloc( (size) ) ) {					\
	MALLOC_ERROR(size);						\
    }

#ifdef DEBUG
#define IF_ALLOC_ERROR(size) if ((size)) {WRITE_FMT_LOG("Can't allocate %d bytes\n", (size)); assert(0);}
#else
#define IF_ALLOC_ERROR(size) if ((size)) {assert(0);}
#endif

#endif /* MR_DEFINES_H_ */
