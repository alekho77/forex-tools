#include "laf_algorithm.h"

#include "laf_algorithm_trainer_impl.h"
#include "laf_algorithm_impl.h"

#include <boost/property_tree/ptree.hpp>

namespace fxlib {

LafTrainer::LafTrainer(const boost::property_tree::ptree& settings, std::ostream& headline, std::ostream& log)
  : impl_(std::make_unique<Impl>(settings, headline, log)) {
}

LafTrainer::~LafTrainer() = default;

void LafTrainer::PrepareTrainingSet(const fxsequence& seq, std::ostream& out) const {
  impl_->prepare_training_set(seq, out);
}

boost::property_tree::ptree LafTrainer::LoadAndTrain(std::istream& in) {
  return impl_->load_and_train(in);
}

LafAlgorithm::LafAlgorithm(const boost::property_tree::ptree& settings)
  : impl_(std::make_unique<Impl>(settings)) {}

double LafAlgorithm::Feed(const fxcandle& candle) {
  return impl_->feed(candle);
}

void LafAlgorithm::Reset() {
  impl_->reset();
}

ForecastInfo LafAlgorithm::Info() const {
  return impl_->info();
}

}  // namespace fxlib
