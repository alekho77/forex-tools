#include "fxquote.h"
#include "fxtime.h"

#include <algorithm>

namespace fxlib {

FxSequence::FxSequence(const boost::gregorian::date_period& period)
  : period_(period) {
  size_t count = 0;
  for (boost::gregorian::day_iterator ditr = { period_.begin() }; ditr < period_.end(); ++ditr) {
    count += ForexOpenHours(*ditr).length().total_seconds() / 60;
  }
  candles_.reserve(count);
}

}  // namespace fxlib
