#include "dummy_algorithm.h"

#include <random>

namespace fxlib {

DummyAlgorithm::DummyAlgorithm(const ForecastInfo& info) : info_(info) {
  std::random_device rd;
  std::mt19937 gen(rd());
}

}  // namespace fxlib
