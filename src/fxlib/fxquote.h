#pragma once

namespace fxlib {

enum class periodicity {
  tick      = 0,
  minutely  = 1,
  hourly    = 60 * minutely,
  dayly     = 24 * hourly,
  monthly   = 30 * dayly
};

#pragma pack(push, 1)

struct alignas(1) candle {
  double    open;
  double    close;
  double    high;
  double    low;
};

#pragma pack(pop)

}  // namespace fxlib
