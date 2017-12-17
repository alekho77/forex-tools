#pragma once

#include "math/mathlib/trainingset.h"
#include "math/mathlib/bp_trainer.h"

namespace fxlib {

namespace details {

struct input_candle {
  double open;
  double close;
  double high;
  double low;
  double volume;
};

// time is 0 for the newest candle, 1.0 for the candle at N * candle period, here N is number of input candles.
using input_data = std::tuple<double/*time*/, input_candle>;

static constexpr size_t input_node_size = 6;

template <size_t InputNodes>
struct nlaf1xx_def {
  using InputLayer = mathlib::input_layer<double, InputNodes * input_node_size>;
  using InputNeuron = mathlib::neuron<double, input_node_size>;
  using InputIndexPack = mathlib::index_sequence_pack_t<input_node_size>;
  using InputMap = mathlib::make_type_pack<InputIndexPack, InputNodes>;

  using Neuron = mathlib::neuron<double, InputNodes>;
  using IndexPack = mathlib::index_sequence_pack_t<InputNodes>;
  using Map = mathlib::type_pack<IndexPack>;
  using Network = mathlib::nnetwork<InputLayer, std::tuple<Neuron>, Map>;
  using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;
  static constexpr size_t sample_size = std::tuple_size<std::tuple_element_t<0, Trainer::sample_t>>::value
                                      + std::tuple_size<std::tuple_element_t<1, Trainer::sample_t>>::value;
};

}  // namespace details

}  // namespace fxlib
