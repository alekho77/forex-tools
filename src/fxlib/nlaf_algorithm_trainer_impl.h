#pragma once

#include "nlaf_algorithm.h"
#include "nlaf_algorithm_impl.h"

#include "fxanalysis.h"

namespace fxlib {

namespace details {
struct nlaf_trainer_cfg : nlaf_cfg {
  struct {
    int epochs;
    double rate;
    double momentum;
  } learning;
};

nlaf_trainer_cfg nlaftrainer_from_ptree(const boost::property_tree::ptree& settings);
}  // namespace details

class NLafTrainer::Impl {
public:
  Impl(const boost::property_tree::ptree& settings, std::ostream& headline, std::ostream& log);

  void prepare_training_set(const fxsequence& seq, std::ostream& out) const;
  boost::property_tree::ptree load_and_train(std::istream&);

private:
  bool check_pos(const boost::posix_time::ptime pos, const fxlib::markers& marks, const boost::posix_time::time_duration window) const;

  const details::nlaf_trainer_cfg cfg_;
  std::shared_ptr<details::inlaf_impl> laf_impl_;
  std::ostream& headline_;
  std::ostream& log_;
  double mean_;
  double var_;
};

}  // namespace fxlib
