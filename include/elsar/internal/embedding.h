#pragma once

#include "globals.h"

namespace elsar {
class Embedding {
 public:
  record_t record;
  converted_t converted_key;

  Embedding() : record(nullptr), converted_key(0) {}
  Embedding(record_t record, converted_t converted_key)
      : record(record), converted_key(converted_key) {}
  Embedding(const Embedding& other) noexcept
      : record(other.record), converted_key(other.converted_key) {}

  // Move constructor
  Embedding(Embedding&& other) noexcept
      : record(other.record), converted_key(other.converted_key) {
    other.record = nullptr;
    other.converted_key = 0;
  }

  // Copy assignment
  Embedding& operator=(const Embedding& other) noexcept {
    if (this != &other) {
      // Copy new data
      converted_key = other.converted_key;
      record = other.record;
    }
    return *this;
  }

  // Move assignment
  Embedding& operator=(Embedding&& other) noexcept {
    if (this != &other) {
      record = other.record;
      converted_key = other.converted_key;

      other.record = nullptr;
      other.converted_key = 0;
    }
    return *this;
  }

  bool operator<(const Embedding& other) const noexcept {
    return converted_key < other.converted_key;
  }
};
}  // namespace elsar
