#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace fxlib {

boost::posix_time::time_period CheckForexTime(const boost::gregorian::date& date);

}  // namespace fxlib
