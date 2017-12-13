#pragma once

#include "laf_algorithm.h"

namespace fxlib {

class LafAlgorithm::Impl {

  struct laf_cfg : ForecastInfo {
    double pip;
    int inputs;  //* Number of inputs: 6, 12, 24 ...
    boost::posix_time::time_duration step;  //* Number of minutes that are used for each input.
  };

  static laf_cfg from_cfg(const boost::property_tree::ptree& settings);

public:
  Impl(const boost::property_tree::ptree& settings);

  double feed(const fxcandle& candle);
  void reset();
  ForecastInfo info() const;

private:
  const laf_cfg cfg_;
};

}  // namespace fxlib
