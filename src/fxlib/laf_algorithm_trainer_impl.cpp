#include "laf_algorithm_trainer_impl.h"

#include "helpers/string_conversion.h"
#include "helpers/nnetwork_helpers.h"

namespace fxlib {

namespace details {

laf_trainer_cfg laftrainer_from_ptree(const boost::property_tree::ptree& settings) {
  laf_trainer_cfg cfg;
  *(laf_cfg*)(&cfg) = laf_from_ptree(settings);
  cfg.learning.rate = settings.get<double>("learning.rate");
  cfg.learning.momentum = settings.get<double>("learning.momentum");
  cfg.learning.epochs = settings.get<int>("learning.epochs");
  return cfg;
}

}  // namespace details


LafTrainer::Impl::Impl(const boost::property_tree::ptree & settings, std::ostream& headline, std::ostream& log)
  : cfg_(details::laftrainer_from_ptree(settings))
  , headline_(headline)
  , log_(log)
  , trainer_(network_) {}

void LafTrainer::Impl::prepare_training_set(const fxsequence& seq, std::ostream& out) const {
  using namespace std;
  headline_ << "Estimating genuine positions..." << endl;
  double time_adjust;
  double probab;
  double durat;
  auto marks = fxlib::GenuinePositions(seq, cfg_.timeout, cfg_.position == fxposition::fxlong ? fxprofit_long : fxprofit_short, cfg_.margin * cfg_.pip, time_adjust, probab, durat);
  headline_ << "Genuine positions: " << marks.size() << endl;
  headline_ << "Pack quotes to " << cfg_.step << "..." << endl;
  const auto pack_seq = PackSequence(seq, cfg_.step);
  headline_ << "New size of the sequence: " << pack_seq.candles.size() << endl;
  if (pack_seq.candles.size() > cfg_.inputs) {
    size_t count = 0;
    size_t positive_count = 0;
    for (auto iter = pack_seq.candles.cbegin() + (cfg_.inputs - 1); iter < pack_seq.candles.cend(); ++iter, ++count) {
      log_ << setw(6) << count;
      for (auto aux_iter = iter - (cfg_.inputs - 1); aux_iter <= iter; ++aux_iter) {
        const double val = fxmean(*aux_iter) / (10000 * cfg_.pip);
        out.write(reinterpret_cast<const char*>(&val), sizeof(val));
        log_ << setw(10) << val;
      }
      double genuine_out = 0.0;
      if (check_pos(iter->time, marks, cfg_.window)) {
        positive_count++;
        genuine_out = 1.0;
      }
      out.write(reinterpret_cast<const char*>(&genuine_out), sizeof(genuine_out));
      log_ << setw(10) << genuine_out << endl;
    }
    headline_ << "Prepared " << count << " training samples including " << positive_count << " positive" << endl;
  } else {
    throw std::logic_error("The size of packed sequence is too small.");
  }
}

void LafTrainer::Impl::load_training_set(std::istream& in) {
  using namespace std;
  headline_ << "Loading training set..." << endl;
  size_t samples_number = trainer_.load(in);
  headline_ << "Loaded " << samples_number << " samples" << endl;
}

void LafTrainer::Impl::train() {
  using namespace std;
  headline_ << "Randomizing weights..." << endl;
  trainer_.randomize_network();
  headline_ << "Training set with " << cfg_.learning.rate << " learning rate and " << cfg_.learning.momentum << " momentum..." << endl;
  trainer_.set_learning_rate(cfg_.learning.rate);
  trainer_.set_momentum(cfg_.learning.momentum);
  headline_ << "Number of epochs " << cfg_.learning.epochs << endl;
  headline_ << "----------------------------------" << endl;
  for (int e = 0; e < cfg_.learning.epochs; e++) {
    headline_ << "Shuffle training set..." << endl;
    trainer_.shuffle();
    headline_ << "Epoch " << e + 1 << ", training..." << endl;
    auto sum_err = trainer_([this](size_t idx, const auto&, const auto&, const auto& errs) {
      this->log_ << setw(8) << idx << setw(15) << get<1>(errs) << endl;
    });
    headline_ << "Mean errors for epoch: [" << get<0>(sum_err) << ", " << get<1>(sum_err) << "]" << endl;
    headline_ << "----------------------------------" << endl;
  }
}

void LafTrainer::Impl::result(boost::property_tree::ptree& settings) const {
  settings.erase("network");
  auto net_params = network_saver<Network>(network_)();
  settings.put_child("network", net_params);
}

bool LafTrainer::Impl::check_pos(const boost::posix_time::ptime pos, const fxlib::markers & marks, const boost::posix_time::time_duration window) const {
  auto icandidate = std::lower_bound(marks.cbegin(), marks.cend(), pos);
  if (icandidate != marks.cend() && *icandidate - pos < window) {
    return true;
  }
  return false;
}

}  // namespace fxlib
