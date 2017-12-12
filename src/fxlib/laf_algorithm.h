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
  void PrepareTraningSet(const fxsequence& seq, std::ostream& out) const override;
  void LoadTraningSet(std::istream& in) override;
private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace fxlib
