#include "nlaf_algorithm.h"
#include "nlaf_algorithm_trainer_impl.h"
#include "nlaf_algorithm_impl.h"

#include <boost/property_tree/ptree.hpp>

namespace fxlib {

NLafTrainer::NLafTrainer(const boost::property_tree::ptree& settings, std::ostream& headline, std::ostream& log)
  : impl_(std::make_unique<Impl>(settings, headline, log)) {
}

NLafTrainer::~NLafTrainer() = default;

void NLafTrainer::PrepareTrainingSet(const fxsequence& seq, std::ostream& out) const {
  impl_->prepare_training_set(seq, out);
}

boost::property_tree::ptree NLafTrainer::LoadAndTrain(std::istream& in) {
  return impl_->load_and_train(in);
}

NLafAlgorithm::NLafAlgorithm(const boost::property_tree::ptree& settings)
  : impl_(std::make_unique<Impl>(settings)) {
}

NLafAlgorithm::~NLafAlgorithm() = default;

double NLafAlgorithm::Feed(const fxcandle& candle) {
  return impl_->feed(candle);
}

void NLafAlgorithm::Reset() {
  impl_->reset();
}

ForecastInfo NLafAlgorithm::Info() const {
  return impl_->info();
}

}  // namespace fxlib
