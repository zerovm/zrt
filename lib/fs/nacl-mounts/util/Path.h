/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

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
  std::string Last(void) { return path_.back(); }

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
