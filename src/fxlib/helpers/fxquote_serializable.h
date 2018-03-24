#pragma once

#include "fxtime_serializable.h"

#include <iostream>

namespace fxlib {
namespace detail {
#pragma pack(push, 1)

struct fxcandle_bin {
    fxtime time;
    uint32_t open;   //
    uint32_t close;  // in millionth
    uint32_t high;   //
    uint32_t low;    //
    uint16_t volume;
};

using fxperiodicity_bin = uint16_t;

struct fxsequence_header_bin {
    fxperiodicity_bin periodicity;
    uint32_t count;
    struct {
        fxtime start;
        fxtime end;
    } period;
};

#pragma pack(pop)
}  // namespace detail

static inline std::ostream& operator<<(std::ostream& out, const detail::fxsequence_header_bin& header) noexcept {
    out.write(reinterpret_cast<const char*>(&header), sizeof(header));
    return out;
}

static inline std::istream& operator>>(std::istream& in, detail::fxsequence_header_bin& header) noexcept {
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    return in;
}

static inline std::ostream& operator<<(std::ostream& out, const detail::fxcandle_bin& candle) noexcept {
    out.write(reinterpret_cast<const char*>(&candle), sizeof(candle));
    return out;
}

static inline std::istream& operator>>(std::istream& in, detail::fxcandle_bin& candle) noexcept {
    in.read(reinterpret_cast<char*>(&candle), sizeof(candle));
    return in;
}

}  // namespace fxlib
