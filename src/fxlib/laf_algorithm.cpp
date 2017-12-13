#include "laf_algorithm.h"

#include "laf_algorithm_trainer_impl.h"
#include "laf_algorithm_impl.h"

namespace fxlib {

LafTrainer::LafTrainer(const boost::property_tree::ptree& settings, std::ostream& headline, std::ostream& log)
  : impl_(std::make_unique<Impl>(settings, headline, log)) {
}

LafTrainer::~LafTrainer() = default;

void LafTrainer::PrepareTrainingSet(const fxsequence& seq, std::ostream& out) const {
  impl_->prepare_training_set(seq, out);
}

void LafTrainer::LoadTrainingSet(std::istream& in) {
  impl_->load_training_set(in);
}

void LafTrainer::Train() {
  impl_->train();
}

void LafTrainer::SaveResult(boost::property_tree::ptree& settings) const {
  impl_->result(settings);
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
