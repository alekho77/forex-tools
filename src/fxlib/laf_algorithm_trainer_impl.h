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
  void load_training_set(std::istream& in);
  void train();
  void result(boost::property_tree::ptree& settings) const;

private:
  bool check_pos(const boost::posix_time::ptime pos, const fxlib::markers& marks, const boost::posix_time::time_duration window) const;

  const details::laf_trainer_cfg cfg_;
  std::ostream& headline_;
  std::ostream& log_;
  details::laf12_algorithm::Network network_;
  details::laf12_algorithm::Trainer trainer_;
};

}  // namespace fxlib
