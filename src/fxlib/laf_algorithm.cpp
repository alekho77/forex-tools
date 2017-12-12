#include "laf_algorithm.h"
#include "fxanalysis.h"
#include "helpers/string_conversion.h"

#include "math/mathlib/trainingset.h"
#include "math/mathlib/bp_trainer.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>

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
  Impl(const boost::property_tree::ptree& settings);
  
  void prepare_training_set(const fxsequence& seq, std::ostream& out) const;

  boost::signals2::signal<void(const std::string&)> on_titling;

private:
  bool check_pos(const boost::posix_time::ptime pos, const fxlib::markers& marks, const boost::posix_time::time_duration window) const;

  const laf_trainer_cfg cfg_;
  Network nework_;
  Trainer trainer_;
};

LafTrainer::LafTrainer(const boost::property_tree::ptree& settings) : impl_(std::make_unique<Impl>(settings)) {
  impl_->on_titling.connect([this](const std::string& str) { this->onTitling(str); });
}

LafTrainer::~LafTrainer() = default;

void LafTrainer::PrepareTraningSet(const fxsequence& seq, std::ostream& out) const {
  impl_->prepare_training_set(seq, out);
}

LafTrainer::Impl::Impl(const boost::property_tree::ptree & settings)
  : cfg_(from_cfg(settings)), trainer_(nework_) {
}

void LafTrainer::Impl::prepare_training_set(const fxsequence& seq, std::ostream& out) const {
  on_titling("Estimating genuine positions...");
  double time_adjust;
  double probab;
  double durat;
  auto marks = fxlib::GenuinePositions(seq, cfg_.timeout, cfg_.position == fxposition::fxlong ? fxprofit_long : fxprofit_short, cfg_.margin * cfg_.pip, time_adjust, probab, durat);
  on_titling("Genuine positions: " + std::to_string(marks.size()));
  on_titling("Pack quotes to " + boost::posix_time::to_simple_string(cfg_.step));
  const auto pack_seq = PackSequence(seq, cfg_.step);
  on_titling("New size of the sequence: " + std::to_string(pack_seq.candles.size()));
  if (pack_seq.candles.size() > cfg_.inputs) {
    size_t count = 0;
    size_t positive_count = 0;
    for (auto iter = pack_seq.candles.cbegin() + (cfg_.inputs - 1); iter < pack_seq.candles.cend(); ++iter, ++count) {
      for (auto aux_iter = iter - (cfg_.inputs - 1); aux_iter <= iter; ++aux_iter) {
        out << fxmean(*aux_iter);
      }
      double genuine_out = 0.0;
      if (check_pos(iter->time, marks, cfg_.window)) {
        positive_count++;
        genuine_out = 1.0;
      }
      out << genuine_out;
    }
    on_titling("Prepared " + std::to_string(count) + " training samples including " + std::to_string(positive_count) + " positive");
  } else {
    throw std::logic_error("The size of packed sequence is too small.");
  }
}

bool LafTrainer::Impl::check_pos(const boost::posix_time::ptime pos, const fxlib::markers & marks, const boost::posix_time::time_duration window) const {
  auto icandidate = std::lower_bound(marks.cbegin(), marks.cend(), pos);
  if (icandidate != marks.cend() && *icandidate - pos < window) {
    return true;
  }
  return false;
}

}  // namespace fxlib
