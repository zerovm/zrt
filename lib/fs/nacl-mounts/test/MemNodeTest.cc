/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include <string>
#include "gtest/gtest.h"
#include "../memory/MemMount.h"
#include "../memory/MemNode.h"

MemNode *CreateMemNode(std::string name, int parent,
                       MemMount *mount, bool is_dir, int slot) {
  MemNode *node = new MemNode();
  node->set_name(name);
  node->set_parent(parent);
  node->set_mount(mount);
  node->set_is_dir(is_dir);
  node->set_slot(slot);
  return node;
}


TEST(MemNodeTest, AddChildren) {
  MemMount *mnt = new MemMount();
  MemNode *node1 = CreateMemNode("node1", 0, mnt, true, 1);
  MemNode *node2 = CreateMemNode("node2", node1->slot(), mnt, true, 2);
  MemNode *node3 = CreateMemNode("node3", node1->slot(), mnt, false, 3);
  MemNode *node4 = CreateMemNode("node4", node1->slot(), mnt, false, 4);
  MemNode *node5 = CreateMemNode("node5", node2->slot(), mnt, false, 5);
  node1->AddChild(node2->slot());
  node1->AddChild(node3->slot());
  node1->AddChild(node4->slot());
  node2->AddChild(node5->slot());

  std::list<int> *children;
  std::list<int> *children2;
  std::list<int> *children3;
  children = node1->children();
  EXPECT_EQ(3, static_cast<int>(children->size()));

  std::list<int>::iterator it;
  it = children->begin();
  EXPECT_EQ(node2->slot(), *it);
  children2 = node2->children();
  EXPECT_EQ(1, static_cast<int>(children2->size()));

  ++it;
  EXPECT_EQ(node3->slot(), *it);
  children3 = node3->children();
  EXPECT_EQ(NULL, children3);

  it = children2->begin();
  EXPECT_EQ(node5->slot(), *it);
  children2 = node5->children();
  EXPECT_EQ(NULL, children2);

  // can't add children to non-directory
  children = node4->children();
  EXPECT_EQ(NULL, children);
  node4->AddChild(0);
  node4->AddChild(1);
  node4->AddChild(2);
  children = node4->children();
  EXPECT_EQ(NULL, children);

  delete mnt;
  delete node1;
  delete node2;
  delete node3;
  delete node4;
  delete node5;
}

TEST(MemNodeTest, RemoveChildren) {
  MemMount *mnt = new MemMount();
  MemNode *node1 = CreateMemNode("node1", 0, mnt, true, 1);
  MemNode *node2 = CreateMemNode("node2", node1->slot(), mnt, true, 2);
  MemNode *node3 = CreateMemNode("node3", node1->slot(), mnt, true, 3);
  MemNode *node4 = CreateMemNode("node4", node1->slot(), mnt, true, 4);
  MemNode *node5 = CreateMemNode("node5", node2->slot(), mnt, false, 5);
  MemNode *node6 = CreateMemNode("node6", node4->slot(), mnt, false, 6);

  node1->AddChild(node2->slot());
  node1->AddChild(node3->slot());
  node1->AddChild(node4->slot());
  node2->AddChild(node5->slot());

  node1->RemoveChild(node2->slot());
  node1->RemoveChild(node3->slot());

  std::list<int> *children;
  std::list<int>::iterator it;

  children = node1->children();
  EXPECT_EQ(1, static_cast<int>(children->size()));
  it = children->begin();
  EXPECT_EQ(node4->slot(), *it);

  node2->RemoveChild(node4->slot());
  children = node2->children();
  EXPECT_EQ(1, static_cast<int>(children->size()));

  node2->RemoveChild(node5->slot());
  children = node2->children();
  EXPECT_EQ(0, static_cast<int>(children->size()));

  // can't remove child from non-directory
  node4->AddChild(node6->slot());
  node4->set_is_dir(false);
  node4->RemoveChild(node6->slot());
  children = node4->children();
  EXPECT_EQ(NULL, children);
  node4->set_is_dir(true);
  node4->RemoveChild(node6->slot());
  children = node4->children();
  EXPECT_EQ(0, static_cast<int>(children->size()));

  node4->set_is_dir(true);

  delete mnt;
  delete node1;
  delete node2;
  delete node3;
  delete node4;
  delete node5;
  delete node6;
}

TEST(MemNodeTest, Stat) {
  MemMount *mnt = new MemMount();
  struct stat *buf = (struct stat *)malloc(sizeof(struct stat));
  MemNode *node1 = CreateMemNode("node", 0, mnt, false, 1);
  int size = 128;
  node1->ReallocData(size);
  EXPECT_EQ(size, node1->capacity());
  node1->stat(buf);
  EXPECT_EQ(0, buf->st_size);
  delete mnt;
  delete node1;
}

TEST(MemNodeTest, UseCount) {
  MemMount *mnt = new MemMount();
  MemNode *node1 = CreateMemNode("node1", 0, mnt, false, 1);
  int i;

  for (i = 0; i < 10; i++) {
    node1->IncrementUseCount();
  }

  for (i = 0; i < 8; i++) {
    node1->DecrementUseCount();
  }

  EXPECT_EQ(2, node1->use_count());

  delete mnt;
  delete node1;
}

TEST(MemNodeTest, ReallocData) {
  MemMount *mnt = new MemMount();
  MemNode *node1 = CreateMemNode("node1", 0, mnt, false, 1);

  node1->ReallocData(10);
  EXPECT_EQ(10, node1->capacity());
  node1->ReallocData(5);
  EXPECT_EQ(5, node1->capacity());
  node1->ReallocData(400);
  EXPECT_EQ(400, node1->capacity());

  delete mnt;
  delete node1;
}
