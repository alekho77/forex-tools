#pragma once

#include "math/mathlib/nnetwork.h"

#include <boost/property_tree/ptree.hpp>

namespace fxlib {

// Store neural network parameters
template <typename Network>
class network_saver {
public:
  network_saver(const Network& net) : network_(net) {}

  boost::property_tree::ptree operator ()() {
    boost::property_tree::ptree params;
    save_layer<0>(params);
    return params;
  }

private:
  template <size_t L>
  void save_layer(boost::property_tree::ptree& params) {
    using Layer = mathlib::network_layer_t<L, Network>;
    auto layer_params = (layer_saver<Layer>(network_.layer<L>()))();
    params.put_child("layer_" + std::to_string(L), layer_params);
    save_layer<L + 1>(params);
  }
  template <>
  void save_layer<Network::num_layers>(boost::property_tree::ptree&) {}

  template <typename Layer>
  struct layer_saver {
    layer_saver(const Layer& layer) : layer_(layer) {}

    boost::property_tree::ptree operator ()() {
      boost::property_tree::ptree params;
      save_neuron<0>(params);
      return params;
    }

    template <size_t N>
    void save_neuron(boost::property_tree::ptree& params) {
      using Neuron = std::tuple_element_t<N, Layer>;
      auto neuron_params = (neuron_saver<Neuron, Neuron::use_bias>(std::get<N>(layer_)))();
      params.put_child("neuron_" + std::to_string(N), neuron_params);
      save_neuron<N + 1>(params);
    }
    template <>
    void save_neuron<std::tuple_size<Layer>::value>(boost::property_tree::ptree&) {}

    const Layer& layer_;
  };

  template <typename Neuron>
  struct neuron_saver_base {
    neuron_saver_base(const Neuron& n) : neuron_(n) {}

    boost::property_tree::ptree operator ()() {
      boost::property_tree::ptree params;
      boost::property_tree::ptree weights;
      save_weight<0>(weights);
      params.put_child("weights", weights);
      return params;
    }

    template <size_t I>
    void save_weight(boost::property_tree::ptree& params) {
      boost::property_tree::ptree weight;
      weight.put_value(std::get<I>(neuron_.weights()));
      params.push_back(std::make_pair("", weight));
      save_weight<I + 1>(params);
    }
    template <>
    void save_weight<Neuron::num_synapses>(boost::property_tree::ptree&) {}

    const Neuron& neuron_;
  };

  template <typename Neuron, bool use_bias>
  struct neuron_saver : neuron_saver_base<Neuron> {
    neuron_saver(const Neuron& n) : neuron_saver_base<Neuron>(n) {}
    boost::property_tree::ptree operator ()() {
      auto params = neuron_saver_base<Neuron>::operator ()();
      params.put("bias", neuron_.bias());
      return params;
    }
  };
  template <typename Neuron>
  struct neuron_saver<Neuron, false> : neuron_saver_base<Neuron> {
    neuron_saver(const Neuron& n) : neuron_saver_base<Neuron>(n) {}
  };

  const Network& network_;
};


}  // namespace fxlib
