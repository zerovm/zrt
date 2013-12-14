/*
 *
 * Copyright (c) 2012-2013, LiteStack, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef CHANNELS_CONF_READER_H_
#define CHANNELS_CONF_READER_H_

#include <stddef.h>
#include <stdint.h>

struct ChannelsConfigInterface;

#define IN_DIR "/dev/in"
#define OUT_DIR "/dev/out"

int AddAllChannelsRelatedToNodeTypeFromDir( struct ChannelsConfigInterface *chan_if,
        const char *dirpath, int channel_mode, int nodetype, const char* nodename_text );

/*Close added and opened channels*/
void CloseChannels( struct ChannelsConfigInterface *chan_if );

/* @param nameall string to extract values
 * @param nodenamelen length of extracted text up to trailing '-'
 * @return node id  */
int ExtractNodeNameId( const char *nameall, int *nodenamelen );

#endif /* CHANNELS_CONF_READER_H_ */
