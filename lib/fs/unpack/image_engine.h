/*
 * image_engine.h
 *
 *  Created on: 25.09.2012
 *      Author: yaroslav
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
    struct MountsInterface* mounts;
    struct UnpackObserver* observer_implementation;
};

struct ImageInterface* alloc_image_loader( struct MountsInterface* );

void free_image_loader( struct ImageInterface* );

#endif /* IMAGE_ENGINE_H_ */
