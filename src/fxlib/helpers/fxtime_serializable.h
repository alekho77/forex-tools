#pragma once

#include <array>
#include <iostream>

namespace fxlib {
#pragma pack(push, 1)

using fxtime_bin = std::array<uint8_t, 8>;
struct fxtime {
  fxtime_bin data;  // iso time string (like 20020131T235959) packed with BCD
};

#pragma pack(pop)

static inline std::ostream& operator << (std::ostream& out, const fxtime_bin& data) noexcept {
  out.write(reinterpret_cast<const char*>(data.data()), data.size());
  return out;
}

static inline std::istream& operator >> (std::istream& in, fxtime_bin& data) noexcept {
  in.read(reinterpret_cast<char*>(data.data()), data.size());
  return in;
}

}  // namespace fxlib
