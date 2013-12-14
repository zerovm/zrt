/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Description
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

#ifndef NACL_STRUCT_H_
#define NACL_STRUCT_H_

#include <sys/stat.h>

struct nacl_abi_dirent {
    unsigned long long   d_ino;     /*offsets NaCl 0 */
    unsigned long long  d_off;     /*offsets NaCl 8 */
    uint16_t  d_reclen;  /*offsets NaCl 16 */
    char     d_name[];  /*offsets NaCl 18 */
};

/* same here */
struct nacl_abi_timeval {
    int64_t   nacl_abi_tv_sec;
    int32_t   nacl_abi_tv_usec;
};


#endif /* NACL_STRUCT_H_ */
