/*
 * should be enhanced to be main test on user side for data provided by trusted side
 * based on manifest files
 * manifest_test.c
 *
 *  Created on: 01.08.2012
 *      Author: yaroslav
 */


#include "zrt.h"
#include "zvm.h"
#include <stdlib.h> //atoi
#include <stdio.h> //printf
#include <string.h>
#include <assert.h>

enum { ESimpleManifest=1 };

/*all available channels we has in manifest*/
#define CHANNEL1 "/dev/stdin"
#define CHANNEL2 "/dev/stdout"


void test_channel(const struct ZVMChannel *ch,
        int type, int getslimit, int getsizelimit, int putslimit, int putsizelimit){
    fprintf(stderr, "zvmchanel name=%s; type=%d, limits=%lld, %lld, %lld, %lld \n",
            ch->name, ch->type, ch->limits[GetsLimit], ch->limits[GetSizeLimit],
            ch->limits[PutsLimit], ch->limits[PutSizeLimit] );
    fflush(0);
    assert( ch->type == type  );
    assert( ch->limits[GetsLimit] == getslimit  );
    assert( ch->limits[GetSizeLimit] == getsizelimit  );
    assert( ch->limits[PutsLimit] == putslimit  );
    assert( ch->limits[PutSizeLimit] == putsizelimit  );
    fprintf(stderr, "channel %s OK\n", ch->name );fflush(0);
}


int main(int argc, char **argv){
    fprintf(stderr, "manifest_test started\n"); fflush(0);
    /*manifest for given test should not be modified, only according to test*/
    int channels_count = 0;
    const struct ZVMChannel *channels = zvm_channels_c( &channels_count );
    int index=0;

    const struct ZVMChannel *ch = 0;
    switch( atoi( getenv("MANIFEST_ID") ) )
    {
    /*simplest manifest parsing ok*/
    case ESimpleManifest:
        fprintf(stderr, "ESimpleManifest\n" );fflush(0);
        for(index=0; index < channels_count; index++){
            ch = &channels[index];
            if ( !strcmp( ch->name, CHANNEL1 ) ){
                test_channel( ch, 0, 1, 1, 0, 0);
            }
        }
        break;
        fprintf(stderr, "UnknownManifestId = %s\n", getenv("MANIFEST_ID") ); fflush(0);
    default:
        break;
    }

    return 0;
}
