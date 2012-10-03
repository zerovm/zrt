/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef PACKAGES_LIBRARIES_NACL_MOUNTS_MEMORY_MEMNODE_H_
#define PACKAGES_LIBRARIES_NACL_MOUNTS_MEMORY_MEMNODE_H_

#include <string.h>
#include <sys/stat.h>
#include <list>
#include <string>
#include "../util/macros.h"
#include "../util/SlotAllocator.h"

class MemMount;

// MemNode is the node object for the MemoryMount class
class MemNode {
 public:
  MemNode();
  ~MemNode();

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
  void ReallocData(int len);

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

  // increase the use count by one
  void IncrementUseCount(void) { ++use_count_; }

  // decrease the use count by one
  void DecrementUseCount(void) { --use_count_; }

  // returns the use count of this node
  int use_count(void) { return use_count_; }

  // capacity() returns the capcity (in bytes) of this node
  int capacity(void) { return capacity_; }

  // set the capacity of this node to capacity bytes
  void set_capacity(int capacity) { capacity_ = capacity; }

  // data() returns a pointer to the data of this node
  char *data(void) { return data_; }

  // set_data() sets the length of this node to len
  void set_len(size_t len) { len_ = len; }

  // len() returns the length of this node
  size_t len(void) { return len_; }

  // set_mount() sets the mount to which this node belongs
  void set_mount(MemMount *mount) { mount_ = mount; }

  // is_dir() returns whether or not this node represents a directory
  bool is_dir(void) { return is_dir_; }
  void set_is_dir(bool is_dir) { is_dir_ = is_dir; }

  // slot() returns this nodes slot number in the MemMount's slots
  int slot(void) { return slot_; }
  void set_slot(int slot) { slot_ = slot; }

  /*added by YaroslavLitvinov*/
  mode_t mode()const { return mode_; }
  void set_mode(mode_t mode) { mode_ = mode; }

 private:
  int slot_;
  std::string name_;
  int parent_;
  char *data_;
  size_t len_;
  size_t capacity_;
  int use_count_;
  bool is_dir_;
  MemMount *mount_;
  std::list<int> children_;
  /*added by YaroslavLitvinov
   *Permissions should be supported by stat*/
  mode_t mode_;

  DISALLOW_COPY_AND_ASSIGN(MemNode);
};

#endif  // PACKAGES_LIBRARIES_NACL_MOUNTS_MEMORY_MEMNODE_H_
