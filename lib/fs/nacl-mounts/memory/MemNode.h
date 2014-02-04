/*
 *
 * Copyright 2011 The Native Client Authors. All rights reserved.
 * Copyright 2008, Google Inc.
 * All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the
distribution.
    * Neither the name of Google Inc. nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

#ifndef PACKAGES_LIBRARIES_NACL_MOUNTS_MEMORY_MEMNODE_H_
#define PACKAGES_LIBRARIES_NACL_MOUNTS_MEMORY_MEMNODE_H_

#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <list>
#include <string>

#include "../util/macros.h"
#include "../util/SlotAllocator.h"

class MemMount;

/*Node data that can be shared between hardlinks*/
class MemData {
 public:
    MemData();
    ~MemData();

    char *data_;
    size_t len_;
    bool is_dir_;
    size_t capacity_;
    int use_count_; 
    int nlink_;      //nlink_ is hardlinks count
    int want_unlink_;//want_unlink_ flag indicates file waiting for remove if close
    mode_t mode_;    //mode_ used by stat indicate permissions
    uint32_t uid_;
    uint32_t gid_;
    int hardinode_; //inode the same for all hardlinks
    struct flock flock_;
    std::list<int> children_;
};

// MemNode is the node object for the MemoryMount class
class MemNode {
 public:
    MemNode();
    ~MemNode();

    //it's should be called after allocation
    //nodedata can be NULL, if creating hardlink then it should be a valid nodedata
    void second_phase_construct(MemData* nodedata);

    // System calls
    int stat(struct stat *buf);

    // Add child to this node's children.  This method will do nothing
    // if this node is a directory or if child points to a child that is
    // not in the children list of this node
    void AddChild(int slot);

    // Remove child from this node's children.  This method will do
    // nothing if the node is not a directory
    void RemoveChild(int slot);

    // Reallocate the size of data to be len bytes.  Copies the
    // current data to the reallocated memory.
    void ReallocData(size_t len);

    // children() returns a list of MemNode pointers
    // which represent the children of this node.
    // If this node is a file or a directory with no children,
    // a NULL pointer is returned.
    std::list<int> *children(void);

    // set_name() sets the name of this node.  This is not the
    // path but rather the name of the file or directory
    void set_name(std::string name) { name_ = name; }

    // name() returns the name of this node
    std::string name(void) { return name_; }

    // set_parent() sets the parent node of this node to
    // parent
    void set_parent(int parent) { parent_ = parent; }

    // parent() returns a pointer to the parent node of
    // this node.  If this node is the root node, 0
    // is returned.
    int parent(void) { return parent_; }

    // set_mount() sets the mount to which this node belongs
    void set_mount(MemMount *mount) { mount_ = mount; }

    // is_dir() returns whether or not this node represents a directory
    bool is_dir(void) { return nodedata_->is_dir_; }
    void set_is_dir(bool is_dir) { nodedata_->is_dir_ = is_dir; }

    // slot() returns this nodes slot number in the MemMount's slots
    int slot(void) { return slot_; }
    void set_slot(int slot) { slot_ = slot; }

    //////////////////////////////////////////////////////
    // shared data 
    //////////////////////////////////////////////////////

    MemData* hardlink_data() { set_hardinode(slot_); return nodedata_; }

    // increase the hardlink count by one
    void increment_nlink(void) { ++nodedata_->nlink_; }

    // decrease the hardlinks count by one
    void decrement_nlink(void) { 
	if(nodedata_->nlink_>0) --nodedata_->nlink_; 
    }

    //hard link count, file opened and not closed
    //returns the use count of this node
    int nlink_count(void) { return nodedata_->nlink_; }
    
    // increase the use count by one
    void increment_use_count(void) { ++nodedata_->use_count_; }

    // decrease the use count by one
    void decrement_use_count(void) { 
	if(nodedata_->use_count_>0)
	    --nodedata_->use_count_; 
    }

    //File refer count, file opened and not closed
    //returns the use count of this node
    int use_count(void) { return nodedata_->use_count_; }

    // capacity() returns the capcity (in bytes) of this node
    int capacity(void) { return nodedata_->capacity_; }

    // set the capacity of this node to capacity bytes
    void set_capacity(int capacity) { nodedata_->capacity_ = capacity; }

    // data() returns a pointer to the data of this node
    char *data(void) { return nodedata_->data_; }

    // set_data() sets the length of this node to len
    void set_len(size_t len) { nodedata_->len_ = len; }

    // len() returns the length of this node
    size_t len(void) { return nodedata_->len_; }

    /*added by YaroslavLitvinov*/
    mode_t mode()const { return nodedata_->mode_; }
    void set_mode(mode_t mode) { nodedata_->mode_ = mode; }

    /*added by YaroslavLitvinov*/
    uint32_t uid()const { return nodedata_->uid_; }
    uint32_t gid()const { return nodedata_->gid_; }
    void set_chown(uint32_t uid, uint32_t gid) { nodedata_->uid_=uid; nodedata_->gid_=gid; }

    /*added by YaroslavLitvinov*/
    const struct flock* flock()const { return &nodedata_->flock_; }
    void set_flock(const struct flock* flock);

    /*added by YaroslavLitvinov*/
    int hardinode()const { return nodedata_->hardinode_; }
    void set_hardinode(int hardinode) { nodedata_->hardinode_ = hardinode; }

    void TryUnlink(){ nodedata_->want_unlink_=1; }
    int  UnlinkisTrying()const{ return nodedata_->want_unlink_; }
    void UnlinkOkResetFlag(){ nodedata_->want_unlink_ = 0; }

 private:
    int slot_;
    std::string name_;
    int parent_;
    MemMount *mount_;
    MemData*  nodedata_;  //can be shared between nodes

    DISALLOW_COPY_AND_ASSIGN(MemNode);
};

#endif  // PACKAGES_LIBRARIES_NACL_MOUNTS_MEMORY_MEMNODE_H_
