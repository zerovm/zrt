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

#ifndef IMAGE_ENGINE_H_
#define IMAGE_ENGINE_H_

struct UnpackInterface;
struct UnpackObserver;

struct ImageInterface{
    /*return files count*/
    int (*deploy_image)( const char* mount_path, struct UnpackInterface* );
    /*data*/
    const char* mount_path; /*point to extract filesystem contents*/
    struct MountsPublicInterface* mounts;
    struct UnpackObserver* observer_implementation;
};

struct ImageInterface* alloc_image_loader( struct MountsPublicInterface* );

void free_image_loader( struct ImageInterface* );

#endif /* IMAGE_ENGINE_H_ */
