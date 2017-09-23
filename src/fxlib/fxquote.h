#pragma once

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>

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
  daily = 24 * hourly,
  weekly = 5 * daily,
  monthly = 4 * weekly + 2 * daily
};

struct fxsequence {
  // Periodicity of quotes in minutes, zero means tick value that is undefined for candles.
  fxperiodicity periodicity;
  // Date of period that may not be equal a time of the first/last candle due a possible gap by a weekend or a holiday.
  boost::gregorian::date_period period;
  std::vector<fxcandle> candles;
};

// Finam is a Russian website that allows you to get at least two months worth of one-minute Forex data.
extern const boost::regex FinamExportFormat; // ("^([A-Z]{6}'TICKER'expression)\\s+([0-9]'PER'expression)\\s+([0-9]{6}'DATE'expression)\\s+([0-9]{4}'TIME'expression) 104.6400000 104.6600000 104.5400000 104.6500000 0");

}  // namespace fxlib
