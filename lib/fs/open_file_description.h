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


#ifndef __OPEN_FILE_DESCRIPTION_H__
#define __OPEN_FILE_DESCRIPTION_H__

#include <stdio.h>
#include <sys/types.h>

struct OpenFileDescription{
    off_t offset; /*used by read, write functions*/
    int   flags; /*opened file's flags*/
};


struct OpenFilesPool{
    /*increment refcount for new ofd, set flags
     *@return id, or -1 error*/
    int (*getnew_ofd)(int flags);
    /*increment refcount for existing open file description
     *@return -1 if bad id, 0 if ok*/
    int (*refer_ofd)(int id_ofd);
    /*decrement refcount, and if it's become 0 then free it.
     *@return 0 if ok, -1 if not located*/
    int (*release_ofd)(int id_ofd);

    const struct OpenFileDescription* (*entry)(int id_ofd);

    /* set offset
     * @return errcode, 0 ok, -1 not found*/
    int (*set_offset)(int id_ofd, off_t offset );

    /* set opened file flags
     * @return errcode, 0 ok, -1 not found*/
    int (*set_flags)(int id_ofd, int flags );
};


struct OpenFilesPool* get_open_files_pool();

#endif //__OPEN_FILE_DESCRIPTION_H__
