#pragma once

#include "fxtime.h"

namespace fxlib {

enum class fxperiodicity : uint32_t {
  tick      = 0,
  minutely  = 1,
  hourly    = 60 * minutely,
  dayly     = 24 * hourly,
  monthly   = 30 * dayly
};

#pragma pack(push, 1)

struct fxcandle {
  double    open;
  double    close;
  double    high;
  double    low;
};

struct fxsequence {
  fxsequence() = delete;
  ~fxsequence() = delete;
  fxsequence(const fxsequence&) = delete;
  fxsequence(fxsequence&&) = delete;
  fxsequence& operator = (const fxsequence&) = delete;
  fxsequence& operator = (fxsequence&&) = delete;
  
  struct {
    uint32_t count;        // total number of candles.
    uint32_t periodicity;  // periodicity of quotes in minutes, zero means tick value that is undefined for candles.
    fxtime start;          // fxtime of period start that may not be equal a time of the first candle due a possible gap by a weekend or a holiday.
    fxtime end;            // fxtime of period end that may not be equal a time of the last candle due a possible gap by a weekend or a holiday.
  } header;
  struct {
    fxtime time;
    fxcandle candle;
  } entries[1];
};

#pragma pack(pop)

}  // namespace fxlib
