/*
 * nacl_simple_tar.h
 *
 *  Created on: 24.09.2012
 *      Author: yaroslav
 */

#ifndef UNPACK_TAR_H
#define UNPACK_TAR_H

#include "unpack_interface.h"

struct MountsReader;

/*alloc unpacker
 *@param channels_mount intended to read tar image channel*/
struct UnpackInterface* alloc_unpacker_tar( struct MountsReader*, struct UnpackObserver* observer );

void free_unpacker_tar( struct UnpackInterface* unpacker_tar );

#endif /* UNPACK_TAR_H */
