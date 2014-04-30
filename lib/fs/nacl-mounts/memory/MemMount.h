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
#include "nacl_struct.h"

//#define DIRENT struct nacl_abi_dirent
#define DIRENT struct dirent

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
  // @parent hardlink specify it if creating hardlink for existing directory
  int Mkdir(const std::string& path, mode_t mode, struct stat *st, MemData* hardlink=NULL);

  // Open file, actually it's a wrapper around Create, 0 is returned if the 
  // node is file successfully opened. -1 is returned on failure. 
  int Open(const std::string& path, int oflag, uint32_t mode, MemData* hardlink=NULL);

  // Return the node corresponding to path.
  int GetNode(const std::string& path, struct stat *st);

  /*Create new hardlink newpath for an oldpath, only for directories */
  int Link(const std::string& oldpath, const std::string& newpath);

  //Remove hardlink for file at path. If it's a last hardlink file data will be freed;
  //It doesn't removing directories, but it can remove directory hardlink if it's not a last
  //@return 0 if removed, -1 on error, errno=EBUSY if file not closed (referred)
  //errno=EISDIR if trying to remove directory with nlink=2
  int Unlink(const std::string& path);
  int UnlinkInternal(MemNode *node);

  // Remove the node directory.  Returns -1 (failure) if node is not
  // a directory.
  int Rmdir(ino_t node);

  int Chown(ino_t slot, uid_t owner, gid_t group);
  int Chmod(ino_t slot, mode_t mode);
  int Stat(ino_t node, struct stat *buf);
  int Getdents(ino_t node, off_t offset, off_t *newoffset, void *buf, unsigned int count);
  ssize_t __NON_INSTRUMENT_FUNCTION__
      Read(ino_t node, off_t offset, void *buf, size_t count);
  ssize_t __NON_INSTRUMENT_FUNCTION__
      Write(ino_t node, off_t offset, const void *buf, size_t count);

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
  int Creat(const std::string& path, mode_t mode, struct stat *st, MemData* hardlink=NULL);

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
