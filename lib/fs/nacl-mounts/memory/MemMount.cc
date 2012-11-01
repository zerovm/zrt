/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>
#include <fcntl.h>

extern "C" {
#include "zrtlog.h"
#include "zrt_helper_macros.h"
#include "fs/channels_readdir.h"
#include "enum_strings.h"
}
#include "MemMount.h"

#define LOG_INTERMEDIATE_ERROR

MemMount::MemMount() {
    // Don't use the zero slot
    slots_.Alloc();
    int slot = slots_.Alloc();
    root_ = slots_.At(slot);
    root_->set_slot(slot);
    root_->set_mount(this);
    root_->set_is_dir(true);
    root_->set_name("/");
}

int MemMount::Creat(const std::string& path, mode_t mode, struct stat *buf) {
    MemNode *child;
    MemNode *parent;

    // Get the directory its in.
    int parent_slot = GetParentSlot(path);
    if (parent_slot == -1) {
        zrt_log_str("return -1, errno = ENOTDIR; parent_slot == -1");
        errno = ENOTDIR;
        return -1;
    }
    parent = slots_.At(parent_slot);
    zrt_log("parent slot=%d", parent_slot);
    if (!parent) {
        zrt_log_str("return -1, errno = EINVAL; parent == NULL");
        errno = EINVAL;
        return -1;
    }
    // It must be a directory.
    if (!(parent->is_dir())) {
        zrt_log_str("return -1, errno = ENOTDIR; parent not dir");
        errno = ENOTDIR;
        return -1;
    }
    // See if file exists.
    child = GetMemNode(path);
    if (child) {
        zrt_log_str("return -1, errno = EEXIST; child exist");
        errno = EEXIST;
        return -1;
    }

    // Create it.
    int slot = slots_.Alloc();
    zrt_log("created slot=%d", slot);
    child = slots_.At(slot);
    child->set_slot(slot);
    child->set_is_dir(false);
    child->set_mode(mode);
    child->set_mount(this);
    Path p(path);
    child->set_name(p.Last());
    child->set_parent(parent_slot);
    parent->AddChild(slot);
    child->IncrementUseCount();

    if (!buf) {
        return 0;
    }
    return Stat(slot, buf);
}

int MemMount::Mkdir(const std::string& path, mode_t mode, struct stat *buf) {
    zrt_log( "path=%s", path.c_str() );
    MemNode *parent;
    MemNode *child;

    // Make sure it doesn't already exist.
    child = GetMemNode(path);
    if (child) {
        zrt_log_str("errno=EEXIST");
        errno = EEXIST;
        return -1;
    }
    // Get the parent node.
    int parent_slot = GetParentSlot(path);
    if (parent_slot == -1) {
        zrt_log_str("errno=ENOENT, parent slot error");
        errno = ENOENT;
        return -1;
    }
    parent = slots_.At(parent_slot);
    if (!parent->is_dir()) {
        zrt_log_str("errno=ENOTDIR");
        errno = ENOTDIR;
        return -1;
    }

    /*retrieve directory name, and compare name length with max available*/
    Path path_name(path);
    if ( path_name.Last().length() > NAME_MAX ){
        zrt_log("dirnamelen=%d, NAME_MAX=%d, errno=ENAMETOOLONG", path_name.Last().length(), NAME_MAX );
        errno=ENAMETOOLONG;
        return -1;
    }

    zrt_log_str("create node");
    // Create a new node
    int slot = slots_.Alloc();
    child = slots_.At(slot);
    child->set_slot(slot);
    child->set_mount(this);
    child->set_is_dir(true);
    child->set_mode(mode);
    Path p(path);
    child->set_name(p.Last());
    child->set_parent(parent_slot);
    parent->AddChild(slot);
    if (!buf) {
        zrt_log_str("created");
        return 0;
    }

    return Stat(slot, buf);
}

int MemMount::GetNode(const std::string& path, struct stat *buf) {
    zrt_log("path=%s", path.c_str());
    Path path_name(path);
    path_name.Last();
    /*if name too long*/
    if ( path_name.Last().length() > NAME_MAX ){
        zrt_log("namelen=%d, NAME_MAX=%d, errno=ENAMETOOLONG", path_name.Last().length(), NAME_MAX );
        errno=ENAMETOOLONG;
        return -1;
    }
    /*if path too long*/
    if  ( path.length() > PATH_MAX ){
        zrt_log("path.length()=%d, PATH_MAX=%d, errno=ENAMETOOLONG", path.length(), PATH_MAX );
        errno=ENAMETOOLONG;
        return -1;
    }
    // Get the directory its in.
    int parent_slot = GetParentSlot(path);
    if (parent_slot == -1) {
        zrt_log_str("return -1, errno = ENOTDIR; parent_slot == -1");
        errno = ENOTDIR;
        return -1;
    }
    errno=0; /*getslot can raise specific errno, so reset errno*/
    int slot = GetSlot(path);
    if (slot == -1) {
        if ( errno == 0 ){
            errno = ENOENT; /*set generic errno if no errno returned*/
        }
        return -1;
    }
    if (!buf) {
        return 0;
    }
    return Stat(slot, buf);
}

MemNode *MemMount::GetMemNode(std::string path) {
    int slot = GetSlot(path);
    if (slot == -1) {
        return NULL;
    }
    return slots_.At(slot);
}

int MemMount::GetSlot(std::string path) {
    int slot;
    std::list<std::string> path_components;
    std::list<int>::iterator it;
    std::list<int> *children;

    // Get in canonical form.
    if (path.length() == 0) {
#ifdef LOG_INTERMEDIATE_ERROR
        zrt_log_str("return -1, path.length() ==0");
#endif
        return -1;
    }
    // Check if it is an absolute path
    Path p(path);
    path_components = p.path();

    // Walk up from root.
    slot = root_->slot();
    std::list<std::string>::iterator path_it;
    // loop through path components
    for (path_it = path_components.begin();
            path_it != path_components.end(); ++path_it) {
        // check if we are at a non-directory
        if (!(slots_.At(slot)->is_dir())) {
#ifdef LOG_INTERMEDIATE_ERROR
            zrt_log_str("return -1, errno = ENOTDIR");
#endif
            errno = ENOTDIR;
            return -1;
        }
        // loop through children
        children = slots_.At(slot)->children();
        for (it = children->begin(); it != children->end(); ++it) {
            if ((slots_.At(*it)->name()).compare(*path_it) == 0) {
                break;
            }
        }
        // check for failure
        if (it == children->end()) {
#ifdef LOG_INTERMEDIATE_ERROR
            zrt_log_str("return -1, errno = ENOENT");
#endif
            errno = ENOENT;
            return -1;
        } else {
            slot = *it;
        }
    }
    // We should now have completed the walk.
    if (slot == 0 && path_components.size() > 1) {
#ifdef LOG_INTERMEDIATE_ERROR
        zrt_log_str("return -1, path_components.size() > 1");
#endif
        return -1;
    }
    return slot;
}

MemNode *MemMount::GetParentMemNode(std::string path) {
    return GetMemNode(path + "/..");
}

int MemMount::GetParentSlot(std::string path) {
    return GetSlot(path + "/..");
}

int MemMount::Chown(ino_t slot, uid_t owner, gid_t group){
    MemNode *node = slots_.At(slot);
    if (node == NULL) {
        zrt_log_str("errno = ENOENT");
        errno = ENOENT;
        return -1;
    }
    else{
        node->set_chown( owner, group );
        return 0;
    }
}

int MemMount::Chmod(ino_t slot, mode_t mode) {
    MemNode *node = slots_.At(slot);
    if (node == NULL) {
        errno = ENOENT;
        return -1;
    }
    else{
        node->set_mode(mode);
        return 0;
    }
}

int MemMount::Stat(ino_t slot, struct stat *buf) {
    MemNode *node = slots_.At(slot);
    if (node == NULL) {
        zrt_log_str("errno=ENOENT");
        errno = ENOENT;
        return -1;
    }

    return node->stat(buf);
}

int MemMount::Unlink(const std::string& path) {
    MemNode *node = GetMemNode(path);
    if (node == NULL) {
        zrt_log_str("errno=ENOENT");
        errno = ENOENT;
        return -1;
    }
    MemNode *parent = GetParentMemNode(path);
    if (parent == NULL) {
        // Can't delete root
        zrt_log_str("errno=EBUSY");
        errno = EBUSY;
        return -1;
    }
    // Check that it's a file.
    if (node->is_dir()) {
        zrt_log_str("errno=EISDIR");
        errno = EISDIR;
        return -1;
    }
    parent->RemoveChild(node->slot());
    Unref(node->slot());
    return 0;
}

int MemMount::Rmdir(ino_t slot) {
    MemNode *node = slots_.At(slot);
    if (node == NULL) {
        return ENOENT;
    }
    // Check if it's a directory.
    if (!node->is_dir()) {
        errno = ENOTDIR;
        return -1;
    }
    // Check if it's empty.
    if (node->children()->size() > 0) {
        errno = ENOTEMPTY;
        return -1;
    }
    zrt_log( "node->name()=%s", node->name().c_str() );

    // if this isn't the root node, remove from parent's
    // children list
    if (slot != 0) {
        slots_.At(node->parent())->RemoveChild(slot);
    }
    slots_.Free(slot);
    return 0;
}

void MemMount::Ref(ino_t slot) {
    MemNode *node = slots_.At(slot);
    if (node == NULL) {
        return;
    }
    node->IncrementUseCount();
}

void MemMount::Unref(ino_t slot) {
    MemNode *node = slots_.At(slot);
    if (node == NULL) {
        return;
    }
    if (node->is_dir()) {
        return;
    }
    node->DecrementUseCount();
    if (node->use_count() > 0) {
        return;
    }
    // If Ref/Unref misused by KernelProxy, it's possible
    // that parent will have a dangling inode to the deleted child
    // TODO(krasin): remove the possibility to misuse this API.
    slots_.Free(node->slot());
}

int MemMount::Getdents(ino_t slot, off_t offset, void *buf, unsigned int buf_size) {
    MemNode *node = slots_.At(slot);
    // Check that node exist and it is a directory.
    if (node == NULL || !node->is_dir()) {
        errno = ENOTDIR;
        return -1;
    }

    std::list<int> *children = node->children();
    int pos;
    int bytes_read;

    pos = 0;
    bytes_read = 0;
    assert(children);

    // Skip to the child at the current offset.
    std::list<int>::iterator it;
    for (it = children->begin(); it != children->end() && pos < offset; ++it) {
        ++pos;
    }

    for (; it != children->end() &&
    bytes_read + sizeof(DIRENT) <= buf_size;
    ++it) {
	MemNode *node = slots_.At(*it);
	/*format in buf dirent structure, of variable size, and save current file data;
	  original MemMount implementation was used dirent as having constant size */
	bytes_read += 
	    put_dirent_into_buf( ((char*)buf)+bytes_read, buf_size-bytes_read, 
				 slot, 0,
				 node->name().c_str(), node->name().length() );
        ++pos;
    }
    return bytes_read;
}

ssize_t MemMount::Read(ino_t slot, off_t offset, void *buf, size_t count) {
    MemNode *node = slots_.At(slot);
    if (node == NULL) {
        errno = ENOENT;
        return -1;
    }
    zrt_log("node->len()=%d, offset=%lld, count=%d", node->len(), offset, count);
    // Limit to the end of the file.
    ssize_t len = count;
    if (len > node->len() - offset) {
        len = node->len() - offset;
	if ( len < 0 )
	    len =0;
	zrt_log("limited len=%d", len);
    }

    // Do the read.
    memcpy(buf, node->data() + offset, len);
    return len;
}

ssize_t MemMount::Write(ino_t slot, off_t offset, const void *buf,
        size_t count) {
    MemNode *node = slots_.At(slot);
    if (node == NULL) {
        errno = ENOENT;
        return -1;
    }

    size_t len = node->capacity();
    // Grow the file if needed.
    if (offset + static_cast<off_t>(count) > static_cast<off_t>(len)) {
        len = offset + count;
        size_t next = (node->capacity() + 1) * 2;
        if (next > len) {
            len = next;
        }
        node->ReallocData(len);
    }
    // Pad any gap with zeros.
    if (offset > static_cast<off_t>(node->len())) {
        memset(node->data()+len, 0, offset);
    }

    // Write out the block.
    memcpy(node->data() + offset, buf, count);
    offset += count;
    if (offset > static_cast<off_t>(node->len())) {
        node->set_len(offset);
    }
    return count;
}

int MemMount::Fcntl(ino_t slot, int cmd, ...){
    MemNode *node = slots_.At(slot);
    zrt_log("fnctl cmd=%s", FCNTL_CMD(cmd));
    int rc=0;
    if (node == NULL) {
        SET_ERRNO(ENOENT);
        rc = -1;
    }
    else{
	if ( cmd == F_DUPFD ){
	}
	else if ( cmd == F_GETFD ){
	    zrt_log_str("get flags=0");
	}
	else if ( cmd == F_SETFD ){
	    va_list args;
	    va_start(args, cmd);
	    int new_flags = va_arg(args, int);
	    va_end(args);
	    zrt_log("new_flags=%d", new_flags);
	}
	else if ( cmd == F_GETFL || cmd == F_SETFL ){
	}
	else if ( cmd == F_SETLK || cmd == F_SETLKW || cmd == F_GETLK ){
	    va_list args;
	    va_start(args, cmd);
	    struct flock* lock_data = va_arg(args, struct flock*);
	    va_end(args);
	    if ( !lock_data ) {
		SET_ERRNO(EINVAL);
		rc = -1;
	    }
	    else{
		zrt_log("lock type=%s", LOCK_TYPE_FLAGS(lock_data->l_type));
		zrt_log("l_whence=%d", lock_data->l_whence);
		zrt_log("l_start=%d", lock_data->l_start);
		zrt_log("l_len=%d", lock_data->l_len);
	
		/*get lock*/
		if ( cmd == F_GETLK ){
		    /*return current lock flag*/
		    rc = node->flock();
		}
		/*set lock*/
		else{
		    node->set_flock(lock_data->l_type);
		}
	    }
	}
    }
    return rc;
}
