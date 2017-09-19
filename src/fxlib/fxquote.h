#pragma once

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <vector>

namespace fxlib {

struct fxcandle {
  boost::posix_time::ptime time;
  double    open;
  double    close;
  double    high;
  double    low;
};

enum class fxperiodicity : int {
  tick = 0,
  minutely = 1,
  hourly = 60 * minutely,
  dayly = 24 * hourly,
  monthly = 30 * dayly
};

struct fxsequence {
  // Periodicity of quotes in minutes, zero means tick value that is undefined for candles.
  fxperiodicity periodicity;
  // Date of period that may not be equal a time of the first/last candle due a possible gap by a weekend or a holiday.
  boost::gregorian::date_period period;
  std::vector<fxcandle> candles;
};

}  // namespace fxlib
