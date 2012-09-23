/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#include "../util/SlotAllocator.h"
#include "gtest/gtest.h"

TEST(SlotAllocatorTest, JustAllocs) {
  SlotAllocator<std::string> slots;
  EXPECT_EQ(0, slots.Alloc());
  EXPECT_NE((std::string*)NULL, slots.At(0));
  EXPECT_EQ(1, slots.Alloc());
}

TEST(SlotAllocatorTest, AllocsAndFrees) {
  SlotAllocator<std::string> slots;
  EXPECT_EQ(0, slots.Alloc());
  EXPECT_NE((std::string*)NULL, slots.At(0));
  slots.Free(0);
  EXPECT_EQ(0, slots.Alloc());
  EXPECT_EQ(1, slots.Alloc());
  slots.Free(0);
  EXPECT_EQ(0, slots.Alloc());
}

TEST(SlotAllocatorTest, AtAccess) {
  SlotAllocator<std::string> slots;
  EXPECT_EQ((std::string*)NULL, slots.At(0));
  EXPECT_EQ(0, slots.Alloc());
  EXPECT_NE((std::string*)NULL, slots.At(0));
  EXPECT_EQ((std::string*)NULL, slots.At(1));
  EXPECT_EQ((std::string*)NULL, slots.At(-1));
}

TEST(SlotAllocatorTest, Same10) {
  SlotAllocator<std::string> slots;
  int i;
  for (i = 0; i < 10; ++i) {
    EXPECT_EQ(i, slots.Alloc());
  }
  for (i = 0; i < 10; ++i) {
    slots.Free(i);
  }
  for (i = 0; i < 10; ++i) {
    EXPECT_EQ(i, slots.Alloc());
  }
}
