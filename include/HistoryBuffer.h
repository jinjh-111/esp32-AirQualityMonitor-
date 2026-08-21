#pragma once

#include <array>
#include <cstddef>

template <typename T, size_t Capacity>
class HistoryBuffer {
 public:
  static_assert(Capacity > 0, "HistoryBuffer capacity must be positive");

  void push(const T& value) {
    values_[next_] = value;
    next_ = (next_ + 1) % Capacity;
    if (size_ < Capacity) {
      ++size_;
    }
  }

  const T& at(size_t chronologicalIndex) const {
    const size_t oldest = size_ == Capacity ? next_ : 0;
    return values_[(oldest + chronologicalIndex) % Capacity];
  }

  size_t size() const { return size_; }
  constexpr size_t capacity() const { return Capacity; }
  bool empty() const { return size_ == 0; }

  void clear() {
    size_ = 0;
    next_ = 0;
  }

 private:
  std::array<T, Capacity> values_{};
  size_t size_ = 0;
  size_t next_ = 0;
};

