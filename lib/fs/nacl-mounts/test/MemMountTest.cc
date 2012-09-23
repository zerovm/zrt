/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "../base/KernelProxy.h"
#include "../memory/MemMount.h"
#include "gtest/gtest.h"

#define CHECK(x) { \
    ASSERT_NE(reinterpret_cast<void*>(NULL), x); \
} while (0)

static MemNode* MountCreat(MemMount* mount, const std::string& path,
                           mode_t mode, bool is_dir) {
  struct stat st;
  if (0 != mount->Creat(path, mode, &st)) {
    return NULL;
  }

  MemNode* node = mount->ToMemNode(st.st_ino);
  if (node == NULL) {
    return NULL;
  }
  node->set_is_dir(is_dir);
  return node;
}


TEST(MemMountTest, ConstructDestruct) {
  MemMount *mount = new MemMount();
  delete mount;
}

TEST(MemMountTest, GetMemNode) {
  MemNode *node1, *node2, *node3, *node4;
  MemMount *mount = new MemMount();
  mode_t mode = 0755;

  CHECK(node1 = MountCreat(mount, "/node1", mode, true));
  CHECK(node2 = MountCreat(mount, "/node2", mode, true));
  CHECK(node3 = MountCreat(mount, "/node1/node3", mode, false));
  CHECK(node4 = MountCreat(mount, "/node2/node4", mode, false));

  EXPECT_EQ(node1, mount->GetMemNode("/node1"));
  EXPECT_NE(node2, mount->GetMemNode("/node1"));
  EXPECT_NE(node3, mount->GetMemNode("/node1"));
  EXPECT_NE(node4, mount->GetMemNode("/node1"));

  EXPECT_EQ(node2, mount->GetMemNode("/node2"));
  EXPECT_NE(node1, mount->GetMemNode("/node2"));
  EXPECT_NE(node3, mount->GetMemNode("/node2"));
  EXPECT_NE(node4, mount->GetMemNode("/node2"));

  EXPECT_EQ(node3, mount->GetMemNode("/node1/node3"));
  EXPECT_NE(node1, mount->GetMemNode("/node1/node3"));
  EXPECT_NE(node2, mount->GetMemNode("/node1/node3"));
  EXPECT_NE(node4, mount->GetMemNode("/node1/node3"));

  EXPECT_EQ(node4, mount->GetMemNode("/node2/node4"));
  EXPECT_NE(node1, mount->GetMemNode("/node2/node4"));
  EXPECT_NE(node2, mount->GetMemNode("/node2/node4"));
  EXPECT_NE(node3, mount->GetMemNode("/node2/node4"));

  EXPECT_EQ(-1, mount->GetNode("hi/there", NULL));
  EXPECT_EQ(-1, mount->GetNode("/hi", NULL));
  EXPECT_EQ(-1, mount->GetNode("/node2/node4/node5", NULL));
  EXPECT_EQ(-1, mount->GetNode("", NULL));

  delete mount;
}

TEST(MemMountTest, Mkdir) {
  MemMount *mount = new MemMount();
  mode_t mode = 0755;

  EXPECT_EQ(0, mount->Mkdir("/hello/", 0, NULL));
  EXPECT_EQ(-1, mount->Mkdir("/hello/", 0, NULL));
  EXPECT_EQ(0, mount->Mkdir("/hello/world", 0, NULL));

  struct stat st;
  EXPECT_EQ(0, mount->GetNode("/hello/world", &st));
  MemNode* node1 = mount->ToMemNode(st.st_ino);
  CHECK(node1);
  node1->set_is_dir(false);

  EXPECT_EQ(-1, mount->Mkdir("/hello/world/again/", 0, NULL));
  node1->set_is_dir(true);
  EXPECT_EQ(0, mount->Mkdir("/hello/world/again/", 0, NULL));

  CHECK(MountCreat(mount, "/hello/world/again/../../world/again/again", mode,
                   false));
  delete mount;
}

TEST(MemMountTest, Creat) {
  MemMount *mount = new MemMount();
  mode_t mode = 0755;

  EXPECT_EQ(0, mount->Mkdir("/node1", 0755, NULL));
  EXPECT_NE(-1, mount->Creat("/node2", mode, NULL));
  EXPECT_NE(-1, mount->Creat("/node1/node3", mode, NULL));
  EXPECT_NE(-1, mount->Creat("/node1/node4/", mode, NULL));
  EXPECT_EQ(-1, mount->GetNode("/node5/", NULL));
  EXPECT_NE(-1, mount->Creat("/node1/node4/../../node1/./node5", mode, NULL));
  EXPECT_NE(-1, mount->GetNode("/node1/node3/../../node1/./", NULL));

  delete mount;
}

static const char* kTestFileName = "/MemMountTest_DefaultMount";

static void test_write() {
  KernelProxy* kp = KernelProxy::KPInstance();
  int fd = kp->open(kTestFileName, O_WRONLY | O_CREAT, 0644);
  if (fd == -1) {
    perror("mm->open: ");
  }
  ASSERT_LE(0, fd);
  ASSERT_EQ(5, kp->write(fd, "hello", 5));
  ASSERT_EQ(0, kp->close(fd));
}

static void test_read(int* out) {
  KernelProxy* kp = KernelProxy::KPInstance();
  int fd = kp->open(kTestFileName, O_RDONLY, 0);
  if (fd == -1) {
    perror("mm->open: ");
  }
  ASSERT_LE(0, fd);
  char buf[6];
  buf[5] = 0;
  ASSERT_EQ(5, kp->read(fd, buf, 5));
  ASSERT_STREQ("hello", buf);
  *out = fd;
}

static void test_close(int fd) {
  ASSERT_EQ(0, KernelProxy::KPInstance()->close(fd));
}

TEST(MemMountTest, DefaultMount) {
  int fds[1];
  for (int i = 0; i < 1; i++) {
    test_write();
    test_read(&fds[i]);
  }
  for (int i = 0; i < 1; i++) {
    test_close(fds[i]);
  }
}

TEST(MemMountTest, Stat) {
  MemMount mount;
  struct stat st;
  ASSERT_EQ(0, mount.Mkdir("/node1", 0755, &st));
  ASSERT_EQ((ino_t)2, st.st_ino);
  ASSERT_EQ(0, mount.Mkdir("/node2", 0755, &st));
  ASSERT_EQ((ino_t)3, st.st_ino);
}

TEST(MemMountTest, Unlink) {
  MemMount mount;
  struct stat st;
  struct stat st2;
  ASSERT_EQ(-1, mount.Unlink("/lala"));
  ASSERT_EQ(0, mount.Creat("/lala", 0644, &st));
  ASSERT_EQ(0, mount.GetNode("/lala", &st2));
  ASSERT_EQ(st.st_ino, st2.st_ino);
  ASSERT_EQ(0, mount.Unlink("/lala"));
  ASSERT_EQ(-1, mount.GetNode("/lala", &st2));
}

TEST(MemMountTest, Unlink2) {
  MemMount mount;
  struct stat st;
  struct stat st2;

  // Create a file
  ASSERT_EQ(0, mount.Creat("/lala", 0644, &st));

  // Mark as used (emulate an open file)
  mount.Ref(st.st_ino);
  // Check we can see it
  ASSERT_EQ(0, mount.GetNode("/lala", &st2));
  ASSERT_EQ(0, mount.Stat(st.st_ino, &st2));

  // Unlink and check that we don't see the file via name
  // but can see via inode.
  ASSERT_EQ(0, mount.Unlink("/lala"));
  ASSERT_EQ(-1, mount.GetNode("/lala", &st2));
  ASSERT_EQ(0, mount.Stat(st.st_ino, &st2));
  ASSERT_EQ(st.st_ino, st2.st_ino);

  // Mark as unused and check that Stat fails.
  mount.Unref(st.st_ino);
  ASSERT_EQ(-1, mount.Stat(st.st_ino, &st2));
}
