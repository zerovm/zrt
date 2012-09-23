/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "Path.h"

Path::Path(const std::string& path) {
  path_ = Split(path, '/');
  is_absolute_ = !path.empty() && path[0] == '/';
  PathCollapse();
}

std::string Path::FormulatePath(void) {
    std::list<std::string>::iterator it;
    std::string path;

    if (is_absolute_) {
      path += "/";
    }

    for (it = path_.begin(); it != path_.end();) {
      path += *it;
      ++it;
      if (it != path_.end()) {
        path += "/";
      }
    }
    return path;
}

Path Path::AppendPath(const std::string& path) {
  return Path(FormulatePath() + "/" + path);
}

std::list<std::string> Path::MergePaths(std::list<std::string> path1,
                                  std::list<std::string> path2) {
  std::list<std::string>::iterator it;
  for (it = path2.begin(); it != path2.end(); ++it) {
    path1.push_back(*it);
  }
  return path1;
}

std::list<std::string> Path::MergePaths(std::list<std::string> path1,
                                              const std::string& path2) {
  return MergePaths(path1, Split(path2, '/'));
}


std::list<std::string> Path::MergePaths(const std::string& path1,
                                              std::list<std::string> path2) {
  return MergePaths(Split(path1, '/'), path2);
}

std::list<std::string> Path::MergePaths(const std::string& path1,
                                              const std::string& path2) {
  return MergePaths(Split(path1, '/'), Split(path2, '/'));
}

std::list<std::string> Path::Split(const std::string& path, char c) {
  std::string part;
  std::list<std::string> components;
  size_t ind = 0;
  size_t next = 0;

  while (next != std::string::npos) {
    next = path.find(c, ind);
    if (next == ind) {
      ++ind;
      continue;
    }
    // length of substring == next - ind
    part = path.substr(ind, next-ind);
    if (!part.empty()) {
      components.push_back(part);
    }
    ind = next+1;
  }
  return components;
}

void Path::PathCollapse(void) {
    std::list<std::string>::iterator it;
    std::list<std::string> new_path_;
    std::string curr, next;

    for (it = path_.begin(); it != path_.end(); ++it) {
      curr = *it;
      // Check if '/' was used excessively in the path.
      // For example, in cd Desktop/////
      if (IsSlash(curr) && it != path_.begin()) continue;

      // Check for '.' in the path.
      if (IsDot(curr)) continue;

      // Check for '..'
      if (IsDotDot(curr)) {
        // Remove the previous component (if there is one)
        if (new_path_.size() > 0) {
          new_path_.pop_back();
        }
        continue;
      }

      // By now, we should have handled everything
      new_path_.push_back(curr);
    }
    path_ = new_path_;
}
