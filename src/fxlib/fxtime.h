#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <array>

namespace fxlib {

#pragma pack(push, 1)

struct alignas(1) fxtime {
  std::array<uint8_t, 8> data;  // iso time string (like 20020131T235959) packed with BCD
};

#pragma pack(pop)

// Returns time period for certain date when the market is opened.
boost::posix_time::time_period ForexOpenHours(const boost::gregorian::date& date) noexcept;

}  // namespace fxlib
