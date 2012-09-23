/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include "zrtlog.h"
}
#include "MemNode.h"

MemNode::MemNode() {
    data_ = NULL;
    len_ = 0;
    capacity_ = 0;
    use_count_ = 0;
}

MemNode::~MemNode() {
    children_.clear();
}

int MemNode::stat(struct stat *buf) {
    memset(buf, 0, sizeof(struct stat));
    buf->st_ino = (ino_t)slot_;
    if (is_dir()) {
        buf->st_mode = S_IFDIR | 0777;
    } else {
        buf->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
                | S_IWOTH;
        buf->st_size = len_;
    }
    buf->st_uid = 1001;
    buf->st_gid = 1002;
    buf->st_blksize = 1024;
    zrt_log("stat ino=%d", slot_);
    return 0;
}

void MemNode::AddChild(int child) {
    if (!is_dir()) {
        return;
    }
    children_.push_back(child);
}

void MemNode::RemoveChild(int child) {
    if (!is_dir()) {
        return;
    }
    children_.remove(child);
}

void MemNode::ReallocData(int len) {
    assert(len > 0);
    // TODO(arbenson): Handle memory overflow more gracefully.
    data_ = reinterpret_cast<char *>(realloc(data_, len));
    assert(data_);
    set_capacity(len);
}

std::list<int> *MemNode::children() {
    if (is_dir()) {
        return &children_;
    } else {
        return NULL;
    }
}
