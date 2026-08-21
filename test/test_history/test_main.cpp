#include <unity.h>

#include "HistoryBuffer.h"

struct Point {
  int value;
};

HistoryBuffer<Point, 3> buffer;

void setUp() { buffer.clear(); }
void tearDown() {}

void test_empty_buffer() {
  TEST_ASSERT_TRUE(buffer.empty());
  TEST_ASSERT_EQUAL_UINT(0, buffer.size());
}

void test_preserves_chronological_order() {
  buffer.push({1});
  buffer.push({2});
  TEST_ASSERT_EQUAL_INT(1, buffer.at(0).value);
  TEST_ASSERT_EQUAL_INT(2, buffer.at(1).value);
}

void test_overwrites_oldest_after_capacity() {
  buffer.push({1});
  buffer.push({2});
  buffer.push({3});
  buffer.push({4});
  TEST_ASSERT_EQUAL_UINT(3, buffer.size());
  TEST_ASSERT_EQUAL_INT(2, buffer.at(0).value);
  TEST_ASSERT_EQUAL_INT(3, buffer.at(1).value);
  TEST_ASSERT_EQUAL_INT(4, buffer.at(2).value);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_empty_buffer);
  RUN_TEST(test_preserves_chronological_order);
  RUN_TEST(test_overwrites_oldest_after_capacity);
  return UNITY_END();
}
