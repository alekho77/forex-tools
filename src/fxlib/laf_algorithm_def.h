#pragma once

#include "math/mathlib/trainingset.h"
#include "math/mathlib/bp_trainer.h"

namespace fxlib {

namespace details {

template <size_t N>
struct laf1xx_def {
    using InputLayer = mathlib::input_layer<double, N>;
    using Neuron = mathlib::neuron<double, N>;
    using IndexPack = mathlib::index_sequence_pack_t<N>;
    using Map = mathlib::type_pack<IndexPack>;
    using Network = mathlib::nnetwork<InputLayer, std::tuple<Neuron>, Map>;
    using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;
    static constexpr size_t sample_size = std::tuple_size<std::tuple_element_t<0, Trainer::sample_t>>::value +
                                          std::tuple_size<std::tuple_element_t<1, Trainer::sample_t>>::value;
};

template <size_t N>
struct laf2xx_def {
    using InputLayer = mathlib::input_layer<double, N>;
    using Neuron1 = mathlib::neuron<double, N>;
    using IndexPack1 = mathlib::index_sequence_pack_t<N>;
    using Map1 = typename mathlib::make_type_pack<IndexPack1, 2>::type;
    using HiddenLayer = mathlib::nnetwork<InputLayer, std::tuple<Neuron1, Neuron1>, Map1>;
    using Neuron2 = mathlib::neuron<double, 2, mathlib::NOBIAS<double>>;
    using Map2 = mathlib::type_pack<mathlib::index_pack<0, 1>>;
    using Network = mathlib::nnetwork<HiddenLayer, std::tuple<Neuron2>, Map2>;
    using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;
    static constexpr size_t sample_size = std::tuple_size<std::tuple_element_t<0, Trainer::sample_t>>::value +
                                          std::tuple_size<std::tuple_element_t<1, Trainer::sample_t>>::value;
};

template <size_t N>
struct laf2xxb_def {
    using InputLayer = mathlib::input_layer<double, N>;
    using Neuron1 = mathlib::neuron<double, N>;
    using IndexPack1 = mathlib::index_sequence_pack_t<N>;
    using Map1 = typename mathlib::make_type_pack<IndexPack1, 2>::type;
    using HiddenLayer = mathlib::nnetwork<InputLayer, std::tuple<Neuron1, Neuron1>, Map1>;
    using Neuron2 = mathlib::neuron<double, 2>;
    using Map2 = mathlib::type_pack<mathlib::index_pack<0, 1>>;
    using Network = mathlib::nnetwork<HiddenLayer, std::tuple<Neuron2>, Map2>;
    using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;
    static constexpr size_t sample_size = std::tuple_size<std::tuple_element_t<0, Trainer::sample_t>>::value +
                                          std::tuple_size<std::tuple_element_t<1, Trainer::sample_t>>::value;
};

template <size_t N>
struct laf3xx_def {
    using InputLayer = mathlib::input_layer<double, N>;
    using Neuron1 = mathlib::neuron<double, N>;
    using IndexPack1 = mathlib::index_sequence_pack_t<N>;
    using Map1 = typename mathlib::make_type_pack<IndexPack1, N>::type;
    using Layer1 = mathlib::nnetwork<InputLayer, typename mathlib::make_tuple_type<Neuron1, N>::type, Map1>;
    using Map2 = typename mathlib::make_type_pack<IndexPack1, 2>::type;
    using Layer2 = mathlib::nnetwork<Layer1, std::tuple<Neuron1, Neuron1>, Map2>;
    using Neuron2 = mathlib::neuron<double, 2, mathlib::NOBIAS<double>>;
    using Map3 = mathlib::type_pack<mathlib::index_pack<0, 1>>;
    using Network = mathlib::nnetwork<Layer2, std::tuple<Neuron2>, Map3>;
    using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;
    static constexpr size_t sample_size = std::tuple_size<std::tuple_element_t<0, Trainer::sample_t>>::value +
                                          std::tuple_size<std::tuple_element_t<1, Trainer::sample_t>>::value;
};

}  // namespace details

}  // namespace fxlib
