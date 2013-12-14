/*
 * Synthetic numbers are used by stat function
 *
 * Copyright (c) 2013, LiteStack, Inc.
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


/* blocksize for file system I/O */
#define DEV_BLOCK_DEVICE_BLK_SIZE 4096
#define DEV_CHAR_DEVICE_BLK_SIZE  1
#define DEV_DIRECTORY_BLK_SIZE    1

/* directory total size, in bytes, it's value unused in further  */
#define DEV_DIRECTORY_SIZE        4096

/* ID of device containing file, it's value unused in further  */
#define DEV_DEVICE_ID             2049

/* user ID of owner, it's value unused in further */
#define DEV_OWNER_UID             1000
/* group ID of owner, it's value unused in further */
#define DEV_OWNER_GID             1000
