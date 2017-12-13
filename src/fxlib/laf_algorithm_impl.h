#pragma once

#include "laf_algorithm.h"

namespace fxlib {

namespace details {
struct laf_cfg : ForecastInfo {
  double pip;
  int inputs;  //* Number of inputs: 6, 12, 24 ...
  boost::posix_time::time_duration step;  //* Number of minutes that are used for each input.
};

laf_cfg laf_from_ptree(const boost::property_tree::ptree& settings);
}  // namespace details

class LafAlgorithm::Impl {
public:
  Impl(const boost::property_tree::ptree& settings);

  double feed(const fxcandle& candle);
  void reset();
  ForecastInfo info() const;

private:
  const details::laf_cfg cfg_;
};

}  // namespace fxlib
