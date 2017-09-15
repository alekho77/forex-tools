#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <array>

namespace fxlib {

#pragma pack(push, 1)

struct fxtime {
  std::array<uint8_t, 8> data;  // iso time string (like 20020131T235959) packed with BCD

  static inline uint64_t to_int(const fxtime& t) noexcept {
    return *reinterpret_cast<const uint64_t*>(t.data.data());
  }
  static inline fxtime from_int(const uint64_t& n) noexcept {
    const uint8_t* dptr = reinterpret_cast<const uint8_t*>(&n);
    return fxtime{{dptr[0], dptr[1], dptr[2], dptr[3], dptr[4], dptr[5], dptr[6], dptr[7]}};
  }
};

#pragma pack(pop)

// Returns time period for certain date when the market is opened.
boost::posix_time::time_period ForexOpenHours(const boost::gregorian::date& date) noexcept;

}  // namespace fxlib
