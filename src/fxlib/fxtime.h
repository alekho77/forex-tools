#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace fxlib {

// Returns time period for certain date when the market is opened (GMT).
boost::posix_time::time_period ForexOpenHours(const boost::gregorian::date& date) noexcept;

}  // namespace fxlib
