/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_MACROS_H_
#define PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_MACROS_H_

// A macro to disallow the evil copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName)      \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)

#endif  // PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_MACROS_H_
