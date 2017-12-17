#pragma once

#include "nlaf_algorithm.h"
#include "nlaf_algorithm_def.h"
#include "helpers/nnetwork_helpers.h"

#include <boost/optional.hpp>

namespace fxlib {

namespace details {
struct nlaf_cfg : ForecastInfo {
  std::string type;
  double pip;
  boost::posix_time::time_duration step;  //* Number of minutes that are used for each input.
  input_data mean;
  input_data var;
};

nlaf_cfg nlaf_from_ptree(const boost::property_tree::ptree& settings);

struct inlaf_impl {
  virtual size_t inputs_number() const = 0;
  virtual void restore_network(const boost::property_tree::ptree& params) = 0;
  virtual double apply_network(const std::vector<input_data>& inputs) const = 0;
  virtual void randomize_network() = 0;
  virtual void set_learning_params(double rate, double momentum) = 0;
  virtual size_t load_set(std::istream& in) = 0;
  virtual std::tuple<double, double> train(const std::function<void(size_t, double)>& cb) = 0;
  virtual boost::property_tree::ptree network_params() const = 0;
  virtual ~inlaf_impl() {}
};

template <typename defines>
class nlaf_alg : public inlaf_impl, public defines {
public:
  nlaf_alg() : trainer_(network_) {}

  size_t inputs_number() const override { return defines::Network::input_size; }
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
  std::tuple<double, double> train(const std::function<void(size_t, double)>& cb) override {
    return trainer_([&cb](size_t idx, const auto&, const auto&, const auto& errs) {
      cb(idx, std::get<1>(errs));
    });
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

std::shared_ptr<inlaf_impl> make_nlaf_impl(const std::string& /*type*/);

}  // namespace details

class NLafAlgorithm::Impl {
public:
  Impl(const boost::property_tree::ptree& settings);

  double feed(const fxcandle& candle);
  void reset();
  ForecastInfo info() const;

private:
  const details::nlaf_cfg cfg_;
  std::shared_ptr<details::inlaf_impl> laf_impl_;
  std::vector<double> inputs_;
  boost::optional<boost::posix_time::time_iterator> time_bound_;
  fxcandle aggr_candle_ = fxcandle();
};

}  // namespace fxlib
