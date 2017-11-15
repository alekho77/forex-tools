#pragma once

#include "fxforecast.h"

namespace fxlib {

class DummyAlgorithm : public IForecaster {
public:
  explicit DummyAlgorithm(const ForecastInfo& info);
private:
  const ForecastInfo info_;
};

}  // namespace fxlib
