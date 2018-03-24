#pragma once

#include "math/mathlib/nnetwork.h"

#include <boost/property_tree/ptree.hpp>

namespace fxlib {

// Store neural network parameters
template <typename Network>
class network_saver {
 public:
    network_saver(const Network& net) : network_(net) {}

    boost::property_tree::ptree operator()() {
        boost::property_tree::ptree params;
        save_layer<0>(params);
        return params;
    }

 private:
    template <size_t L>
    void save_layer(boost::property_tree::ptree& params) {
        using Layer = mathlib::network_layer_t<L, Network>;
        auto layer_params = (layer_saver<Layer>(network_.layer<L>()))();
        params.put_child("layer_" + std::to_string(L + 1), layer_params);
        save_layer<L + 1>(params);
    }
    template <>
    void save_layer<Network::num_layers>(boost::property_tree::ptree&) {}

    template <typename Layer>
    struct layer_saver {
        layer_saver(const Layer& layer) : layer_(layer) {}

        boost::property_tree::ptree operator()() {
            boost::property_tree::ptree params;
            save_neuron<0>(params);
            return params;
        }

        template <size_t N>
        void save_neuron(boost::property_tree::ptree& params) {
            using Neuron = std::tuple_element_t<N, Layer>;
            auto neuron_params = (neuron_saver<Neuron, Neuron::use_bias>(std::get<N>(layer_)))();
            params.put_child("neuron_" + std::to_string(N + 1), neuron_params);
            save_neuron<N + 1>(params);
        }
        template <>
        void save_neuron<std::tuple_size<Layer>::value>(boost::property_tree::ptree&) {}

        const Layer& layer_;
    };

    template <typename Neuron>
    struct neuron_saver_base {
        neuron_saver_base(const Neuron& n) : neuron_(n) {}

        boost::property_tree::ptree operator()() {
            boost::property_tree::ptree params;
            boost::property_tree::ptree weights;
            save_weight<0>(weights);
            params.put_child("weights", weights);
            return params;
        }

        template <size_t I>
        void save_weight(boost::property_tree::ptree& params) {
            params.put("weight_" + std::to_string(I + 1), neuron_.weight<I>());
            save_weight<I + 1>(params);
        }
        template <>
        void save_weight<Neuron::num_synapses>(boost::property_tree::ptree&) {}

        const Neuron& neuron_;
    };

    template <typename Neuron, bool use_bias>
    struct neuron_saver : neuron_saver_base<Neuron> {
        neuron_saver(const Neuron& n) : neuron_saver_base<Neuron>(n) {}
        boost::property_tree::ptree operator()() {
            auto params = neuron_saver_base<Neuron>::operator()();
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

// Restore neural network parameters
template <typename Network>
class network_restorer {
 public:
    network_restorer(Network& net) : network_(net) {}

    void operator()(const boost::property_tree::ptree& params) {
        restore_layer<0>(params.get_child("network"));
    }

 private:
    template <size_t L>
    void restore_layer(const boost::property_tree::ptree& params) {
        using Layer = mathlib::network_layer_t<L, Network>;
        auto layer_params = params.get_child("layer_" + std::to_string(L + 1));
        (layer_restorer<Layer>(network_.layer<L>()))(layer_params);
        restore_layer<L + 1>(params);
    }
    template <>
    void restore_layer<Network::num_layers>(const boost::property_tree::ptree&) {}

    template <typename Layer>
    struct layer_restorer {
        layer_restorer(Layer& layer) : layer_(layer) {}

        void operator()(const boost::property_tree::ptree& params) {
            restore_neuron<0>(params);
        }

        template <size_t N>
        void restore_neuron(const boost::property_tree::ptree& params) {
            using Neuron = std::tuple_element_t<N, Layer>;
            auto neuron_params = params.get_child("neuron_" + std::to_string(N + 1));
            (neuron_restorer<Neuron, Neuron::use_bias>(std::get<N>(layer_)))(neuron_params);
            restore_neuron<N + 1>(params);
        }
        template <>
        void restore_neuron<std::tuple_size<Layer>::value>(const boost::property_tree::ptree&) {}

        Layer& layer_;
    };

    template <typename Neuron>
    struct neuron_restorer_base {
        neuron_restorer_base(Neuron& n) : neuron_(n) {}

        void operator()(const boost::property_tree::ptree& params) {
            const auto weights = params.get_child("weights");
            restore_weight<0>(weights);
        }

        template <size_t I>
        void restore_weight(const boost::property_tree::ptree& params) {
            neuron_.set_weight<I>(params.get<double>("weight_" + std::to_string(I + 1)));
            restore_weight<I + 1>(params);
        }
        template <>
        void restore_weight<Neuron::num_synapses>(const boost::property_tree::ptree&) {}

        Neuron& neuron_;
    };

    template <typename Neuron, bool use_bias>
    struct neuron_restorer : neuron_restorer_base<Neuron> {
        neuron_restorer(Neuron& n) : neuron_restorer_base<Neuron>(n) {}
        void operator()(const boost::property_tree::ptree& params) {
            neuron_restorer_base<Neuron>::operator()(params);
            neuron_.set_bias(params.get<double>("bias"));
        }
    };
    template <typename Neuron>
    struct neuron_restorer<Neuron, false> : neuron_restorer_base<Neuron> {
        neuron_restorer(Neuron& n) : neuron_restorer_base<Neuron>(n) {}
    };

    Network& network_;
};

}  // namespace fxlib
