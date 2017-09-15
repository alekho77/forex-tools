#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <array>
#include <algorithm>
#include <iterator>

namespace fxlib {

#pragma pack(push, 1)

struct fxtime {
  std::array<uint8_t, 8> data;  // iso time string (like 20020131T235959) packed with BCD

  inline uint64_t to_int() const noexcept {
    fxtime res;
    std::reverse_copy(std::cbegin(data), std::cend(data), std::begin(res.data));
    return *reinterpret_cast<uint64_t*>(res.data.data());
  }
};

static_assert(sizeof(fxtime) == 8, "fxtime must be 8 bytes");

#pragma pack(pop)

// Returns time period for certain date when the market is opened.
boost::posix_time::time_period ForexOpenHours(const boost::gregorian::date& date) noexcept;

}  // namespace fxlib
