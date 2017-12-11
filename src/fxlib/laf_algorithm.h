#pragma once

/*
    Single layer network for modeling linear adaptive filtering (LAF).
*/

#include "fxforecast.h"

namespace fxlib {

class laf_trainer;

class LafTrainer : public ITrainer {
public:
  explicit LafTrainer(const boost::property_tree::ptree& settings);
  std::vector<double> PrepareTraningSet(const fxsequence& seq) const override;
private:
  laf_trainer* trainer_;
};


}  // namespace fxlib
