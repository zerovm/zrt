/*
 *
 * Copyright 2008 The Native Client Authors. All rights reserved.
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

#ifndef PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_SLOTALLOCATOR_H_
#define PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_SLOTALLOCATOR_H_

#include <stdint.h>
#include <algorithm>
#include <set>
#include <vector>
#include "macros.h"

// The slot allocator class is a memory management tool.
// This class allocates memory for the templated class
// and uses a slot index to direct the user to that
// memory.
template <class T>
class SlotAllocator {
 public:
  SlotAllocator() {}
  // The destructor loops through each slot.  If the slot
  // contains an object, that object is deleted (freeing
  // the memory).
  virtual ~SlotAllocator();

  // Alloc() allocates memory for an object of type T
  // and returns the slot index in this SlotAllocator
  // that corresponds to that memory.
  int Alloc();

  // AllocAt() allocates memory for an object of type T
  // at specified index (this is required for dup2 syscall)
  // and returns the slot index in this SlotAllocator
  // that corresponds to that memory.
  // Returns -1 on error.
  int AllocAt(int fd);

  // Free() frees the memory in the given slot.  If
  // no memory has been allocated to the slot, this
  // method does nothing.
  void Free(int slot);

  // At() returns the pointer to the allocated memory
  // at the given slot.  At() returns NULL if:
  // (1) the slot is out of range
  // (2) no memory has been allocated at slot
  T *At(int slot);

 private:
  std::vector<T*> slots_;
  std::set<int> free_fds_;

  static bool comp(int i, int j) { return i > j; }

  DISALLOW_COPY_AND_ASSIGN(SlotAllocator);
};

// template implementations
template <class T>
SlotAllocator<T>::~SlotAllocator() {
  for (uint32_t i = 0; i < slots_.size(); ++i) {
    if (slots_[i]) {
      delete slots_[i];
      slots_[i] = NULL;
    }
  }
}

template <class T>
int SlotAllocator<T>::Alloc() {
  if (free_fds_.size() == 0) {
    slots_.push_back(new T);
    return slots_.size()-1;
  }
  int index = *(free_fds_.begin());
  free_fds_.erase(free_fds_.begin());
  slots_[index] = new T;
  return index;
}

template <class T>
int SlotAllocator<T>::AllocAt(int fd) {
  if (slots_.size() < (unsigned)(fd + 1)) {
    int prev_size = slots_.size();
    slots_.resize(fd + 1);
    for (int i = prev_size; i < fd + 1; ++i) free_fds_.insert(i);
  }

  if (free_fds_.erase(fd)) {
    slots_[fd] = new T;
    return fd;
  }
  return -1;
}

template <class T>
void SlotAllocator<T>::Free(int slot) {
  if (slot < 0 ||
      static_cast<uint32_t>(slot) >= slots_.size() ||
      !slots_[slot]) {
    return;
  }
  delete slots_[slot];
  slots_[slot] = NULL;
  free_fds_.insert(slot);
}

template <class T>
T *SlotAllocator<T>::At(int slot) {
  if (slot < 0 || static_cast<uint32_t>(slot) >= slots_.size()) {
    return NULL;
  }
  return slots_[slot];
}

#endif  // PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_SLOTALLOCATOR_H_
