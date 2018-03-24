#pragma once

#include "fxforecast.h"

#include <random>

namespace fxlib {

class DummyAlgorithm : public IForecaster {
 public:
    explicit DummyAlgorithm(const boost::property_tree::ptree& settings);
    double Feed(const fxcandle&) override;
    void Reset() override;
    ForecastInfo Info() const override {
        return info_;
    }

 private:
    const ForecastInfo info_;
    std::mt19937 gen_;
    std::uniform_real_distribution<> dis_;
};

}  // namespace fxlib
