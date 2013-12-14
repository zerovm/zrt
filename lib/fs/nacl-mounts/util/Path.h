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

#ifndef PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_PATH_H_
#define PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_PATH_H_

#include <list>
#include <string>

// Path is a class for manipulating file paths.  This is
// a utility class for the nacl-mounts pluggable file I/O.
// Fundamentally, Path assumes directories are split
// by the character '/' in a path name.  In addition,
// Path assumes that the character '/' is not part of
// a directory name.  The class Path is immutable.
class Path {
 public:
  Path() {}
  // This constructor splits path by '/' as a starting
  // point for this Path.  If the path begins
  // with the character '/', the path is considered
  // to be absolute.
  explicit Path(const std::string& path);
  ~Path() {}

  // FormulatePath() returns a string representation
  // of this Path.
  std::string FormulatePath(void);

  // Last() returns the last path component in the path
  // represented by this Path.  This can represent
  // the file or directory name of the path.
  std::string Last(void) {
      /*YaroslavLitvinov std::list::back() for empty container is falls*/
      if ( !path_.empty() ) return path_.back();
      else return "/"; }

  // AppendPath returns a Path that represents the
  // path of this Path with the argument path appended.
  Path AppendPath(const std::string& path);

  bool is_absolute() { return is_absolute_; }
  std::list<std::string> path(void) { return path_; }

 private:
  // internal representation of the path
  std::list<std::string> path_;
  // whether or not this path is considered to be absolute
  bool is_absolute_;

  // MergePaths will take two paths and create a list of strings
  // of the components of these paths.  Strings will be split
  // by '/' in order to merge into a a single list.
  static std::list<std::string> MergePaths(std::list<std::string> path1,
                                    std::list<std::string> path2);

  static std::list<std::string> MergePaths(std::list<std::string> path1,
                                    const std::string& path2);

  static std::list<std::string> MergePaths(const std::string& path1,
                                    std::list<std::string> path2);

  static std::list<std::string> MergePaths(const std::string& path1,
                                    const std::string& path2);

  // Split() divides a string into pieces separated by c.  Each occurrence
  // of c is not included in any member of the returned list.
  static std::list<std::string> Split(const std::string& path, char c);

  // PathCollapse() puts path_ into its canonical form. This is
  // done in place.
  void PathCollapse(void);

  // IsDot() checks if s is "."
  static bool IsDot(const std::string& s) { return s == "."; }

  // IsDotDot() checks if s is ".."
  static bool IsDotDot(const std::string& s) { return s == ".."; }

  // IsSlash() checks if s is "/"
  static bool IsSlash(const std::string& s) { return s == "/"; }
};

#endif  // PACKAGES_LIBRARIES_NACL_MOUNTS_UTIL_PATH_H_
