#include "dummy_algorithm.h"

namespace fxlib {

DummyAlgorithm::DummyAlgorithm(const ForecastInfo& info)
  : info_(info)
  , gen_(std::random_device()())
  , dis_(1, info.window) {
}

fxforecast DummyAlgorithm::Feed(const fxcandle&) {
  int val = dis_(gen_);
  return val == (info_.window / 2) ? fxforecast::positive : fxforecast::negative;
}

void DummyAlgorithm::Reset() {
  gen_ = std::mt19937(std::random_device()());
}

}  // namespace fxlib
