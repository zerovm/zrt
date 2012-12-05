/*
 * fstab_observer.h
 *
 *  Created on: 03.12.2012
 *      Author: yaroslav
 */

#ifndef FSTAB_OBSERVER_H_
#define FSTAB_OBSERVER_H_


struct FstabLoader;

struct MFstabObserver{
    int (*handle_fstab_record)(struct FstabLoader* fstab_loader,
            const char* channel_alias, const char* mount_path);
};


struct MFstabObserver* get_fstab_observer();

#endif /* FSTAB_OBSERVER_H_ */
