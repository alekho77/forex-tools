#pragma once

#include "laf_algorithm.h"
#include "laf_algorithm_def.h"
#include "helpers/nnetwork_helpers.h"

#include <boost/optional.hpp>

namespace fxlib {

namespace details {
struct laf_cfg : ForecastInfo {
    std::string type;
    double pip;
    boost::posix_time::time_duration step;  //* Number of minutes that are used for each input.
    double mean;
    double var;
    double normalize(const fxcandle& c) const {
        return (fxmean(c) - mean) / var;
    }
};

laf_cfg laf_from_ptree(const boost::property_tree::ptree& settings);

struct ilaf_impl {
    virtual size_t inputs_number() const = 0;
    virtual void restore_network(const boost::property_tree::ptree& params) = 0;
    virtual double apply_network(const std::vector<double>& inputs) const = 0;
    virtual void randomize_network() = 0;
    virtual void set_learning_params(double rate, double momentum) = 0;
    virtual size_t load_set(std::istream& in) = 0;
    virtual std::tuple<double, double> train() = 0;
    virtual boost::property_tree::ptree network_params() const = 0;
    virtual ~ilaf_impl() {}
};

template <typename defines>
class laf_alg : public ilaf_impl, public defines {
 public:
    laf_alg() : trainer_(network_) {}

    size_t inputs_number() const override {
        return defines::Network::input_size;
    }
    void restore_network(const boost::property_tree::ptree& params) override {
        (network_restorer<Network>(network_))(params);
    }
    double apply_network(const std::vector<double>& inputs) const override {
        return apply_network(inputs, std::make_index_sequence<defines::Network::input_size>());
    }
    void randomize_network() override {
        trainer_.randomize_network();
    }
    void set_learning_params(double rate, double momentum) override {
        trainer_.set_learning_rate(rate);
        trainer_.set_momentum(momentum);
    }
    size_t load_set(std::istream& in) override {
        const size_t count = trainer_.load(in);
        trainer_.shuffle();
        return count;
    }
    std::tuple<double, double> train() override {
        return trainer_();
    }
    boost::property_tree::ptree network_params() const override {
        return network_saver<typename defines::Network>(network_)();
    }

 private:
    template <size_t... I>
    double apply_network(const std::vector<double>& inputs, std::index_sequence<I...>) const {
        return std::get<0>(network_(inputs[I]...));
    }
    typename defines::Network network_;
    typename defines::Trainer trainer_;
};

std::shared_ptr<ilaf_impl> make_laf_impl(const std::string& /*type*/);

}  // namespace details

class LafAlgorithm::Impl {
 public:
    Impl(const boost::property_tree::ptree& settings);

    double feed(const fxcandle& candle);
    void reset();
    ForecastInfo info() const;

 private:
    const details::laf_cfg cfg_;
    std::shared_ptr<details::ilaf_impl> laf_impl_;
    std::vector<double> inputs_;
    boost::optional<boost::posix_time::time_iterator> time_bound_;
    fxcandle aggr_candle_ = fxcandle();
};

}  // namespace fxlib
