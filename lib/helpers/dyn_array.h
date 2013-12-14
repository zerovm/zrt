/*
 * Implementation of dynamic arrays.
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

/* @file
 *
 * NaCl utility for a dynamically sized array where the first unused
 * position can be discovered quickly.  This is used for I/O
 * descriptors -- the number of elements in an array is typically not
 * large, and the common access is finding an entry by its index, with
 * occasional need to allocate the last numbered slot for a new I/O
 * descriptor.
 *
 * A dynamic array containing void pointers is created using
 * DynArrayCtor in a placement-new manner with the initial size as
 * paramter.  The caller is responsible for the memory.  The dynamic
 * array is destroyed with DynArrayDtor, which destroys the contents
 * of the dynamic array; again, the caller is responsible for
 * destroying the DynArray object itself.  Since the array contains
 * void pointers, if these pointers should be in turn freed or have
 * destructors invoked on them, it is the responsible of the user to
 * do so.
 *
 * Accessors DynArrayGet and DynArraySet does what you would expect.
 * DynArrayFirstAvail returns the index of the first slot that is
 * unused.  Note that DynArraySet will grow the array as needed to set
 * the element, even if the value is a NULL pointer.  Such an entry is
 * still considerd to be unused.
 */

#ifndef _DYN_ARRAY_H__
#define _DYN_ARRAY_H__

#include <stddef.h> //size_t
#include <stdint.h> //uint32_t

struct DynArray {
  /* public */
  size_t    num_entries;
  /*
   * note num_entries has a somewhat unusual property.  if valid
   * entries are written to index 10, the num_entries is 11 even if
   * there have never been any writes to indices 0...9.
   */

  /* protected */
  void      **ptr_array;  /* we *could* sort/bsearch this */
  size_t    ptr_array_space;
  uint32_t  *available;
  size_t    avail_ix;
};

int DynArrayCtor(struct DynArray  *dap,
                 size_t           initial_size);

void DynArrayDtor(struct DynArray *dap);

void *DynArrayGet(struct DynArray *dap,
                  size_t          idx);

int DynArraySet(struct DynArray *dap,
                size_t          idx,
                void            *ptr);

#endif
