#pragma once

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <vector>

namespace fxlib {

struct fxcandle {
    boost::posix_time::ptime time;
    double open;
    double close;
    double high;
    double low;
    size_t volume;
};

static inline double fxmean(const fxcandle& c) {
    return (c.open + c.close) / 2.0;
}

static inline double fxprofit(const fxcandle& high, const fxcandle& low) {
    return fxmean(high) - fxmean(low);
}

static inline double fxprofit_long(const fxcandle& close, const fxcandle& open) {
    return fxprofit(close, open);
}

static inline double fxprofit_short(const fxcandle& close, const fxcandle& open) {
    return fxprofit(open, close);
}

using fprofit_t = double (*)(const fxlib::fxcandle& /*close*/, const fxlib::fxcandle& /*open*/);

// enum class fxperiodicity : int {
//  tick = 0,
//  minutely = 1,
//  hourly = 60 * minutely,
//  daily = 24 * hourly,
//  weekly = 5 * daily + 6 * hourly,
//  monthly = 4 * weekly + 2 * daily
//};

struct fxsequence {
    // Periodicity of quotes in minutes, zero means tick value that is undefined for candles.
    boost::posix_time::time_duration periodicity;
    // Date of period that may not be equal a time of the first/last candle due a possible gap by a weekend or a
    // holiday.
    boost::gregorian::date_period period;
    std::vector<fxcandle> candles;
};

// Binary writing/reading quote sequence into a stream.
void WriteSequence(std::ostream& out, const fxsequence& seq) noexcept(false);
fxsequence ReadSequence(std::istream& in) noexcept(false);

fxsequence PackSequence(const fxsequence& min_seq, const boost::posix_time::time_duration& new_period);
}  // namespace fxlib
