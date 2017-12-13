#pragma once

#include "laf_algorithm.h"

#include "fxanalysis.h"

#include "math/mathlib/trainingset.h"
#include "math/mathlib/bp_trainer.h"

namespace fxlib {

class LafTrainer::Impl {
  using InputLayer = mathlib::input_layer<double, 12>;
  using Neuron = mathlib::neuron<double, 12>;
  using IndexPack = mathlib::index_pack<11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>;
  using Map = mathlib::type_pack<IndexPack>;
  using Network = mathlib::nnetwork<InputLayer, std::tuple<Neuron>, Map>;
  using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;

  struct laf_trainer_cfg {
    fxposition position;
    boost::posix_time::time_duration window;  //* Window size
    boost::posix_time::time_duration timeout;  //* Timeout of wait
    double margin;  //* Expected margin, in rate units
    double pip;
    int inputs;  //* Number of inputs: 6, 12, 24 ...
    boost::posix_time::time_duration step;  //* Number of minutes that are used for each input.
    struct {
      int epochs;
      double rate;
      double momentum;
    } learning;
  };

  static laf_trainer_cfg from_cfg(const boost::property_tree::ptree& settings);

public:
  Impl(const boost::property_tree::ptree& settings, std::ostream& headline, std::ostream& log);

  void prepare_training_set(const fxsequence& seq, std::ostream& out) const;
  void load_training_set(std::istream& in);
  void train();
  void result(boost::property_tree::ptree& settings) const;

private:
  bool check_pos(const boost::posix_time::ptime pos, const fxlib::markers& marks, const boost::posix_time::time_duration window) const;

  const laf_trainer_cfg cfg_;
  std::ostream& headline_;
  std::ostream& log_;
  Network network_;
  Trainer trainer_;
};

}  // namespace fxlib
