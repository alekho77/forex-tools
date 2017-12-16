#include "laf_algorithm_trainer_impl.h"

#include "helpers/string_conversion.h"
#include "helpers/nnetwork_helpers.h"

#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream_buffer.hpp>

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
  , log_(log) {
  laf_impl_ = details::make_laf_impl(cfg_.type);
}

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
  const size_t ninputs = laf_impl_->inputs_number();
  if (pack_seq.candles.size() > ninputs) {
    double mean = 0;
    for (const auto& c: pack_seq.candles) {
      mean += fxmean(c);
    }
    mean /= pack_seq.candles.size();
    double var = 0;
    for (const auto& c : pack_seq.candles) {
      const double dv = fxmean(c) - mean;
      var += dv * dv;
    }
    var = sqrt(var / (pack_seq.candles.size() - 1));
    headline_ << "mean: " << mean << ", variance: " << var << endl;
    out.write(reinterpret_cast<const char*>(&mean), sizeof(mean));
    out.write(reinterpret_cast<const char*>(&var), sizeof(var));
    // Separate writing positive and negative samples.
    vector<double> positives;
    vector<double> negatives;
    size_t count = 0;
    for (auto iter = pack_seq.candles.cbegin() + (ninputs - 1); iter < pack_seq.candles.cend(); ++iter, ++count) {
      log_ << setw(6) << count;
      vector<double> sample;
      for (auto aux_iter = iter - (ninputs - 1); aux_iter <= iter; ++aux_iter) {
        const double val = (fxmean(*aux_iter) - mean) / var;
        log_ << setw(10) << val;
        sample.push_back(val);
      }
      const bool positive = check_pos(iter->time, marks, cfg_.window);
      sample.push_back(positive ? 1.0 : 0.0);
      if (positive) {
        positives.insert(positives.cend(), sample.cbegin(), sample.cend());
      } else {
        negatives.insert(negatives.cend(), sample.cbegin(), sample.cend());
      }
      log_ << setw(10) << sample.back() << endl;
    }
    const size_t positive_count = positives.size() / (ninputs + 1);
    const size_t negative_count = negatives.size() / (ninputs + 1);
    if ((positive_count + negative_count) != count) {
      throw logic_error("Something has gone wrong!");
    }
    out.write(reinterpret_cast<const char*>(&positive_count), sizeof(positive_count));
    out.write(reinterpret_cast<const char*>(&negative_count), sizeof(negative_count));
    out.write(reinterpret_cast<const char*>(positives.data()), sizeof(double) * positives.size());
    out.write(reinterpret_cast<const char*>(negatives.data()), sizeof(double) * negatives.size());
    headline_ << "Prepared " << count << " training samples including " << positive_count << " positive" << endl;
  } else {
    throw logic_error("The size of packed sequence is too small.");
  }
}

boost::property_tree::ptree LafTrainer::Impl::load_and_train(std::istream& in) {
  using namespace std;
  using namespace boost::iostreams;
  headline_ << "Loading training set..." << endl;
  in.read(reinterpret_cast<char*>(&mean_), sizeof(mean_));
  in.read(reinterpret_cast<char*>(&var_), sizeof(var_));
  headline_ << "mean: " << mean_ << ", variance: " << var_ << endl;
  size_t positive_count;
  size_t negative_count;
  in.read(reinterpret_cast<char*>(&positive_count), sizeof(positive_count));
  in.read(reinterpret_cast<char*>(&negative_count), sizeof(negative_count));
  const size_t sample_size = laf_impl_->inputs_number() + 1;
  vector<double> positives(positive_count * sample_size);
  vector<double> negatives(negative_count * sample_size);
  in.read(reinterpret_cast<char*>(positives.data()), sizeof(double) * positives.size());
  in.read(reinterpret_cast<char*>(negatives.data()), sizeof(double) * negatives.size());
  headline_ << "positive: " << positive_count << ", negative: " << negative_count << ", total: " << (positive_count + negative_count) << endl;
  vector<size_t> neg_indexes;
  neg_indexes.reserve(negative_count);
  for (size_t i = 0; i < negative_count; ++i) {
    neg_indexes.emplace_back(i);
  }
  shuffle(neg_indexes.begin(), neg_indexes.end(), mt19937(random_device()()));
  headline_ << "----------------------------------" << endl;

  headline_ << "Randomizing weights..." << endl;
  laf_impl_->randomize_network();
  headline_ << "Training set with " << cfg_.learning.rate << " learning rate and " << cfg_.learning.momentum << " momentum..." << endl;
  laf_impl_->set_learning_params(cfg_.learning.rate, cfg_.learning.momentum);
  headline_ << "Number of epochs " << cfg_.learning.epochs << endl;
  headline_ << "----------------------------------" << endl;
  size_t curr_neg_idx = 0;
  for (int e = 0; e < cfg_.learning.epochs; e++) {
    headline_ << "Epoch " << (e + 1) << endl;
    headline_ << "Preparing training set..." << endl;
    vector<double> train_set;
    train_set.reserve(2 * positive_count * sample_size);
    train_set.insert(train_set.cend(), positives.cbegin(), positives.cend());
    for (size_t i = 0; i < positive_count; ++i) {
      const size_t neg_idx = neg_indexes[curr_neg_idx];
      auto niter = negatives.cbegin() + neg_idx * sample_size;
      train_set.insert(train_set.cend(), niter, niter + sample_size);
      curr_neg_idx = (curr_neg_idx + 1) % negative_count;
    }
    stream_buffer<array_source> buf(reinterpret_cast<const char*>(train_set.data()), sizeof(double) * train_set.size());
    istream trin(&buf);
    const size_t samples_number = laf_impl_->load_set(trin);
    headline_ << "Loaded " << samples_number << " samples" << endl;
    headline_ << "Training..." << endl;
    auto sum_err = laf_impl_->train([this](size_t idx, double err) {
      this->log_ << setw(8) << idx << setw(15) << err << endl;
    });
    headline_ << "Mean errors for epoch: [" << get<0>(sum_err) << ", " << get<1>(sum_err) << "]" << endl;
    headline_ << "----------------------------------" << endl;
    if (get<0>(sum_err) <= get<1>(sum_err)) {
      throw logic_error("The error has increased");
    }
  }
  
  boost::property_tree::ptree params;
  auto net_params = laf_impl_->network_params();
  params.put_child("network", net_params);
  params.put("mean", mean_);
  params.put("variance", var_);
  return params;
}

bool LafTrainer::Impl::check_pos(const boost::posix_time::ptime pos, const fxlib::markers & marks, const boost::posix_time::time_duration window) const {
  auto icandidate = std::lower_bound(marks.cbegin(), marks.cend(), pos);
  if (icandidate != marks.cend() && *icandidate - pos < window) {
    return true;
  }
  return false;
}

}  // namespace fxlib
