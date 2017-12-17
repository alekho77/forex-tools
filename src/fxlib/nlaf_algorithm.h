#pragma once

/*
Single layer network for modeling non-linear adaptive filtering (NLAF).
*/

#include "fxforecast.h"

#include <memory>

namespace fxlib {

class NLafTrainer : public ITrainer {
public:
  explicit NLafTrainer(const boost::property_tree::ptree& settings, std::ostream& headline, std::ostream& log);
  ~NLafTrainer();

  void PrepareTrainingSet(const fxsequence& seq, std::ostream& out) const override;
  boost::property_tree::ptree LoadAndTrain(std::istream&) override;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

class NLafAlgorithm : public IForecaster {
public:
  explicit NLafAlgorithm(const boost::property_tree::ptree& settings);
  ~NLafAlgorithm();

  double Feed(const fxcandle&) override;
  void Reset() override;
  ForecastInfo Info() const override;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace fxlib
