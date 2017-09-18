#pragma once

#include <array>

namespace fxlib {
#pragma pack(push, 1)

struct fxtime {
  std::array<uint8_t, 8> data;  // iso time string (like 20020131T235959) packed with BCD
};

#pragma pack(pop)
}  // namespace fxlib
