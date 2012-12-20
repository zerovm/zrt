/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef __MEM_MOUNT_H__
#define __MEM_MOUNT_H__

#include <assert.h>
#include <errno.h>
#include <list>
#include <string>
#include "../util/macros.h"
#include "../util/Path.h"
#include "../util/SlotAllocator.h"
#include "MemNode.h"

#ifdef __native_client__
    #include "../../../nacl_struct.h"
    #define DIRENT struct nacl_abi_dirent
#else
    #define DIRENT struct dirent
#endif


// MemMount is a storage mount representing local memory.  The MemMount uses
// the MemNode to represent an inode.
class MemMount {
 public:
  MemMount();
  virtual ~MemMount() {}

  // Ref() increments the use count of the MemNode corresponding to the inode.
  void Ref(ino_t node);

  // Unref() decrements the use count of the MemNode corresponding to the inode.
  void Unref(ino_t node);

  // Mkdir() makes a directory at path with the given mode and stores the
  // information of that node in st.  0 is returned if the directory is
  // successfully created. -1 is returned on failure.
  int Mkdir(const std::string& path, mode_t mode, struct stat *st);

  // Open file, actually it's a wrapper around Create, 0 is returned if the 
  // node is file successfully opened. -1 is returned on failure. 
  int Open(const std::string& path, int oflag, uint32_t mode);

  // Return the node corresponding to path.
  int GetNode(const std::string& path, struct stat *st);

  // Remove the node at path.
  int Unlink(const std::string& path);

  // Remove the node directory.  Returns -1 (failure) if node is not
  // a directory.
  int Rmdir(ino_t node);

  int Chown(ino_t slot, uid_t owner, gid_t group);
  int Chmod(ino_t slot, mode_t mode);
  int Stat(ino_t node, struct stat *buf);
  int Getdents(ino_t node, off_t offset, void *buf, unsigned int count);
  ssize_t Read(ino_t node, off_t offset, void *buf, size_t count);
  ssize_t Write(ino_t node, off_t offset, const void *buf, size_t count);

  // Return the node at path.  If the path is invalid, NULL is returned.
  MemNode *GetMemNode(std::string path);

  // Get the MemNode corresponding to the inode.
  MemNode *ToMemNode(ino_t node) {
    return slots_.At(node);
  }

 private:
  // Creat() creates a node at path with the given mode and stores the
  // information of that node in st.  0 is returned if the node is
  // successfully created. -1 is returned on failure.
  int Creat(const std::string& path, mode_t mode, struct stat *st);

  // Return the MemNode corresponding to the inode.
  // Return the node that is a parent of the node at path.
  // If the path is not valid or if the node has no parent,
  // NULL is returned.
  MemNode *GetParentMemNode(std::string path);

  // Get the slot number of the node at path.  If the path
  // is invalid, -1 is returned.
  int GetSlot(std::string path);

  // Get the slot number of the parent of the node at path.
  // If the path is invalid or the node has no parent, -1
  // is returned.
  int GetParentSlot(std::string path);

  MemNode *root_;

  SlotAllocator<MemNode> slots_;

  DISALLOW_COPY_AND_ASSIGN(MemMount);
};

#endif  // __MEM_MOUNT_H__
