// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_HASH_H_
#define BASE_HASH_H_

#include <stddef.h>
#include <stdint.h>

#include <limits>
#include <string>

#include "base/base_export.h"
#include "base/logging.h"

namespace base {

// WARNING: This hash function should not be used for any cryptographic purpose.
BASE_EXPORT uint32_t SuperFastHash(const char* data, int len);

// Computes a hash of a memory buffer |data| of a given |length|.
// WARNING: This hash function should not be used for any cryptographic purpose.
inline uint32_t Hash(const char* data, size_t length) {
  if (length > static_cast<size_t>(std::numeric_limits<int>::max())) {
    NOTREACHED();
    return 0;
  }
  return SuperFastHash(data, static_cast<int>(length));
}

// Computes a hash of a string |str|.
// WARNING: This hash function should not be used for any cryptographic purpose.
inline uint32_t Hash(const std::string& str) {
  return Hash(str.data(), str.size());
}

inline std::size_t HashInts32(uint32_t value1, uint32_t value2) {
  uint64_t value1_64 = value1;
  uint64_t hash64 = (value1_64 << 32) | value2;

  if (sizeof(std::size_t) >= sizeof(uint64_t))
    return static_cast<std::size_t>(hash64);

  uint64_t odd_random = 481046412LL << 32 | 1025306955LL;
  uint32_t shift_random = 10121U << 16;

  hash64 = hash64 * odd_random + shift_random;
  std::size_t high_bits = static_cast<std::size_t>(
      hash64 >> (8 * (sizeof(uint64_t) - sizeof(std::size_t))));
  return high_bits;
}

inline std::size_t HashInts64(uint64_t value1, uint64_t value2) {
  uint32_t short_random1 = 842304669U;
  uint32_t short_random2 = 619063811U;
  uint32_t short_random3 = 937041849U;
  uint32_t short_random4 = 3309708029U;

  uint32_t value1a = static_cast<uint32_t>(value1 & 0xffffffff);
  uint32_t value1b = static_cast<uint32_t>((value1 >> 32) & 0xffffffff);
  uint32_t value2a = static_cast<uint32_t>(value2 & 0xffffffff);
  uint32_t value2b = static_cast<uint32_t>((value2 >> 32) & 0xffffffff);

  uint64_t product1 = static_cast<uint64_t>(value1a) * short_random1;
  uint64_t product2 = static_cast<uint64_t>(value1b) * short_random2;
  uint64_t product3 = static_cast<uint64_t>(value2a) * short_random3;
  uint64_t product4 = static_cast<uint64_t>(value2b) * short_random4;

  uint64_t hash64 = product1 + product2 + product3 + product4;

  if (sizeof(std::size_t) >= sizeof(uint64_t))
    return static_cast<std::size_t>(hash64);

  uint64_t odd_random = 1578233944LL << 32 | 194370989LL;
  uint32_t shift_random = 20591U << 16;

  hash64 = hash64 * odd_random + shift_random;
  std::size_t high_bits = static_cast<std::size_t>(
      hash64 >> (8 * (sizeof(uint64_t) - sizeof(std::size_t))));
  return high_bits;
}

template<typename T1, typename T2>
inline std::size_t HashPair(T1 value1, T2 value2) {
  // This condition is expected to be compile-time evaluated and optimised away
  // in release builds.
  if (sizeof(T1) > sizeof(uint32_t) || (sizeof(T2) > sizeof(uint32_t)))
    return HashInts64(value1, value2);

  return HashInts32(value1, value2);
}

}  // namespace base

#endif  // BASE_HASH_H_
