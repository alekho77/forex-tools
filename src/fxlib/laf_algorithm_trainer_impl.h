#pragma once

#include "laf_algorithm.h"
#include "laf_algorithm_impl.h"

#include "fxanalysis.h"

namespace fxlib {

namespace details {
struct laf_trainer_cfg : laf_cfg {
  struct {
    int epochs;
    double rate;
    double momentum;
  } learning;
};

laf_trainer_cfg laftrainer_from_ptree(const boost::property_tree::ptree& settings);
}  // namespace details

class LafTrainer::Impl {
public:
  Impl(const boost::property_tree::ptree& settings, std::ostream& headline, std::ostream& log);

  void prepare_training_set(const fxsequence& seq, std::ostream& out) const;
  boost::property_tree::ptree load_and_train(std::istream&);

private:
  bool check_pos(const boost::posix_time::ptime pos, const fxlib::markers& marks, const boost::posix_time::time_duration window) const;

  const details::laf_trainer_cfg cfg_;
  std::shared_ptr<details::ilaf_impl> laf_impl_;
  std::ostream& headline_;
  std::ostream& log_;
  double mean_;
  double var_;
};

}  // namespace fxlib
