#pragma once

#include "laf_algorithm.h"
#include "laf_algorithm_impl.h"

#include "fxanalysis.h"

#include "math/mathlib/trainingset.h"
#include "math/mathlib/bp_trainer.h"

namespace fxlib {

namespace details {
struct laf_trainer_cfg : details::laf_cfg {
  struct {
    int epochs;
    double rate;
    double momentum;
  } learning;
};

laf_trainer_cfg laftrainer_from_ptree(const boost::property_tree::ptree& settings);
}  // namespace details

class LafTrainer::Impl {
  using InputLayer = mathlib::input_layer<double, 12>;
  using Neuron = mathlib::neuron<double, 12>;
  using IndexPack = mathlib::index_pack<11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>;
  using Map = mathlib::type_pack<IndexPack>;
  using Network = mathlib::nnetwork<InputLayer, std::tuple<Neuron>, Map>;
  using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;

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
  Network network_;
  Trainer trainer_;
};

}  // namespace fxlib
