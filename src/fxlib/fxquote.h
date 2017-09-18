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

class FxSequence {
public:
  enum fxperiodicity : int {
    tick = 0,
    minutely = 1,
    hourly = 60 * minutely,
    dayly = 24 * hourly,
    monthly = 30 * dayly
  };

  explicit FxSequence(const boost::gregorian::date_period& period);

  // Total number of candles.
  inline size_t Count() const noexcept {
    return candles_.size();
  }
  
  // Periodicity of quotes in minutes, zero means tick value that is undefined for candles.
  inline fxperiodicity Periodicity() const noexcept {
    return periodicity_;
  }

  // Date of period that may not be equal a time of the first/last candle due a possible gap by a weekend or a holiday.
  inline boost::gregorian::date_period Period() const noexcept {
    return period_;
  }

private:
  const fxperiodicity periodicity_ = minutely;
  const boost::gregorian::date_period period_;
  std::vector<fxcandle> candles_;
};

#pragma pack(push, 1)


// struct fxsequence {
//   fxsequence() = delete;
//   ~fxsequence() = delete;
//   fxsequence(const fxsequence&) = delete;
//   fxsequence(fxsequence&&) = delete;
//   fxsequence& operator = (const fxsequence&) = delete;
//   fxsequence& operator = (fxsequence&&) = delete;
//   
//   struct {
//     uint32_t count;        // 
//     uint32_t periodicity;  // 
//     fxtime start;          // 
//     fxtime end;            // fxtime of period end that may not be equal a time of the last candle due a possible gap by a weekend or a holiday.
//   } header;
//   struct {
//     fxtime time;
//     fxcandle candle;
//   } entries[1];
// };

#pragma pack(pop)

}  // namespace fxlib
