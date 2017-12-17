#pragma once

#include "math/mathlib/trainingset.h"
#include "math/mathlib/bp_trainer.h"

namespace fxlib {

namespace details {

struct laf112_def {
  using InputLayer = mathlib::input_layer<double, 12>;
  using Neuron = mathlib::neuron<double, 12>;
  using IndexPack = mathlib::index_sequence_pack_t<12>;
  using Map = mathlib::type_pack<IndexPack>;
  using Network = mathlib::nnetwork<InputLayer, std::tuple<Neuron>, Map>;
  using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;
  static constexpr size_t sample_size = std::tuple_size<std::tuple_element_t<0, Trainer::sample_t>>::value
                                      + std::tuple_size<std::tuple_element_t<1, Trainer::sample_t>>::value;
};

struct laf124_def {
  using InputLayer = mathlib::input_layer<double, 24>;
  using Neuron = mathlib::neuron<double, 24>;
  using IndexPack = mathlib::index_sequence_pack_t<24>;
  using Map = mathlib::type_pack<IndexPack>;
  using Network = mathlib::nnetwork<InputLayer, std::tuple<Neuron>, Map>;
  using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;
  static constexpr size_t sample_size = std::tuple_size<std::tuple_element_t<0, Trainer::sample_t>>::value
                                      + std::tuple_size<std::tuple_element_t<1, Trainer::sample_t>>::value;
};

}  // namespace details

}  // namespace fxlib
