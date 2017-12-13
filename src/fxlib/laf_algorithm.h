#pragma once

/*
    Single layer network for modeling linear adaptive filtering (LAF).
*/

#include "fxforecast.h"

#include <memory>

namespace fxlib {

class LafTrainer : public ITrainer {
public:
  explicit LafTrainer(const boost::property_tree::ptree& settings, std::ostream& headline, std::ostream& log);
  ~LafTrainer();
  
  void PrepareTrainingSet(const fxsequence& seq, std::ostream& out) const override;
  void LoadTrainingSet(std::istream& in) override;
  void Train() override;
  void SaveResult(boost::property_tree::ptree& settings) const override;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

class LafAlgorithm : public IForecaster {
public:
  explicit LafAlgorithm(const boost::property_tree::ptree& settings);
  double Feed(const fxcandle&) override;
  void Reset() override;
  ForecastInfo Info() const override;
private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace fxlib
