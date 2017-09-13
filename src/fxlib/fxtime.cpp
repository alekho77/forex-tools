#include "fxtime.h"

namespace fxlib {

boost::posix_time::time_period ForexOpenHours(const boost::gregorian::date& date) {
  return boost::posix_time::time_period(boost::posix_time::ptime(date), boost::posix_time::hours(24));
}

}  // namespace fxlib
