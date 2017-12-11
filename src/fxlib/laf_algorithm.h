#pragma once

/*
    Single layer network for modeling linear adaptive filtering (LAF).
*/

#include "fxforecast.h"

#include <memory>

namespace fxlib {

class LafTrainer : public ITrainer {
public:
  explicit LafTrainer(const boost::property_tree::ptree& settings);
  ~LafTrainer();
  std::vector<double> PrepareTraningSet(const fxsequence& seq) const override;
private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace fxlib
