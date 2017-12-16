#pragma once

#include "laf_algorithm.h"

#include "math/mathlib/trainingset.h"
#include "math/mathlib/bp_trainer.h"

#include <boost/optional.hpp>

#include <array>

namespace fxlib {

namespace details {
struct laf_cfg : ForecastInfo {
  double pip;
  int inputs;  //* Number of inputs: 6, 12, 24 ...
  boost::posix_time::time_duration step;  //* Number of minutes that are used for each input.
  double mean;
  double var;
};

laf_cfg laf_from_ptree(const boost::property_tree::ptree& settings);

struct laf12_algorithm {
  using InputLayer = mathlib::input_layer<double, 12>;
  using Neuron = mathlib::neuron<double, 12>;
  using IndexPack = mathlib::index_sequence_pack_t<12>;
  using Map = mathlib::type_pack<IndexPack>;
  using Network = mathlib::nnetwork<InputLayer, std::tuple<Neuron>, Map>;
  using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;
};
}  // namespace details

class LafAlgorithm::Impl {
public:
  Impl(const boost::property_tree::ptree& settings);

  double feed(const fxcandle& candle);
  void reset();
  ForecastInfo info() const;

private:

  template <size_t... I>
  double apply_network(std::index_sequence<I...>) {
    return std::get<0>(network_(inputs_[I]...));
  }

  const details::laf_cfg cfg_;
  details::laf12_algorithm::Network network_;
  std::array<double, details::laf12_algorithm::Network::input_size> inputs_;
  size_t curr_idx_ = 0;
  boost::optional<boost::posix_time::time_iterator> time_bound_;
  boost::optional<fxcandle> aggr_candle_;
};

}  // namespace fxlib
