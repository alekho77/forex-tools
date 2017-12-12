#include "laf_algorithm.h"
#include "fxanalysis.h"
#include "helpers/string_conversion.h"

#include "math/mathlib/trainingset.h"
#include "math/mathlib/bp_trainer.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>

#include <iomanip>

namespace fxlib {

namespace {

struct laf_trainer_cfg {
  fxposition position;
  boost::posix_time::time_duration window;  //* Window size
  boost::posix_time::time_duration timeout;  //* Timeout of wait
  double margin;  //* Expected margin, in rate units
  double pip;
  int inputs;  //* Number of inputs: 6, 12, 24 ...
  boost::posix_time::time_duration step;  //* Number of minutes that are used for each input.
  struct {
    double rate;
    double momentum;
  } learning;
};

laf_trainer_cfg from_cfg(const boost::property_tree::ptree& settings) {
  laf_trainer_cfg cfg{};
  cfg.position = settings.get<std::string>("position") == "long" ? fxposition::fxlong : fxposition::fxshort;
  cfg.window = conversion::duration_from_string(settings.get<std::string>("window"));
  cfg.timeout = conversion::duration_from_string(settings.get<std::string>("timeout"));
  cfg.margin = settings.get<double>("margin");
  cfg.pip = settings.get<double>("pip");
  cfg.inputs = settings.get<int>("inputs");
  cfg.step = conversion::duration_from_string(settings.get<std::string>("step"));
  cfg.learning.rate = settings.get<double>("learning.rate");
  cfg.learning.momentum = settings.get<double>("learning.momentum");
  return cfg;
}

}  // namespace

class LafTrainer::Impl {
  using InputLayer = mathlib::input_layer<double, 12>;
  using Neuron = mathlib::neuron<double, 12>;
  using IndexPack = mathlib::index_pack<11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0>;
  using Map = mathlib::type_pack<IndexPack>;
  using Network = mathlib::nnetwork<InputLayer, std::tuple<Neuron>, Map>;
  using Trainer = mathlib::training_set<mathlib::bp_trainer, Network>;

public:
  Impl(const boost::property_tree::ptree& settings, std::ostream& headline, std::ostream& log);
  
  void prepare_training_set(const fxsequence& seq, std::ostream& out) const;
  void load_training_set(std::istream& in);
  void train();

private:
  bool check_pos(const boost::posix_time::ptime pos, const fxlib::markers& marks, const boost::posix_time::time_duration window) const;

  const laf_trainer_cfg cfg_;
  std::ostream& headline_;
  std::ostream& log_;
  Network nework_;
  Trainer trainer_;
};

LafTrainer::LafTrainer(const boost::property_tree::ptree& settings, std::ostream& headline, std::ostream& log)
  : impl_(std::make_unique<Impl>(settings, headline, log)) {
}

LafTrainer::~LafTrainer() = default;

void LafTrainer::PrepareTrainingSet(const fxsequence& seq, std::ostream& out) const {
  impl_->prepare_training_set(seq, out);
}

void LafTrainer::LoadTrainingSet(std::istream & in) {
  impl_->load_training_set(in);
}

void LafTrainer::Train() {
  impl_->train();
}

LafTrainer::Impl::Impl(const boost::property_tree::ptree & settings, std::ostream& headline, std::ostream& log)
  : cfg_(from_cfg(settings))
  , headline_(headline)
  , log_(log)
  , trainer_(nework_) {
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
  if (pack_seq.candles.size() > cfg_.inputs) {
    size_t count = 0;
    size_t positive_count = 0;
    for (auto iter = pack_seq.candles.cbegin() + (cfg_.inputs - 1); iter < pack_seq.candles.cend(); ++iter, ++count) {
      log_ << setw(6) << count;
      for (auto aux_iter = iter - (cfg_.inputs - 1); aux_iter <= iter; ++aux_iter) {
        const double val = fxmean(*aux_iter);
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
    headline_ << "Prepared " << count << " training samples including "  << positive_count << " positive" << endl;
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
  headline_ << "Shuffle training set and weights..." << endl;
  trainer_.shuffle();
  trainer_.randomize_network();
  headline_ << "Training set with " << cfg_.learning.rate << " learning rate and " << cfg_.learning.momentum << " momentum..." << endl;
  trainer_.set_learning_rate(cfg_.learning.rate);
  trainer_.set_momentum(cfg_.learning.momentum);
  trainer_([this](size_t idx, const auto&, const auto&, const auto& errs) {
    this->log_ << setw(8) << idx << setw(20) << get<1>(errs) << endl;
  });
}

bool LafTrainer::Impl::check_pos(const boost::posix_time::ptime pos, const fxlib::markers & marks, const boost::posix_time::time_duration window) const {
  auto icandidate = std::lower_bound(marks.cbegin(), marks.cend(), pos);
  if (icandidate != marks.cend() && *icandidate - pos < window) {
    return true;
  }
  return false;
}

}  // namespace fxlib
