#include "laf_algorithm.h"
#include "fxanalysis.h"
#include "helpers/string_conversion.h"

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
public:
  Impl(const boost::property_tree::ptree& settings);
  std::vector<double> prepare_training_set(const fxsequence& seq) const;

  boost::signals2::signal<void(const std::string&)> on_preparing;
private:
  const laf_trainer_cfg cfg_;
};

LafTrainer::LafTrainer(const boost::property_tree::ptree& settings) : impl_(std::make_unique<Impl>(settings)) {
  impl_->on_preparing.connect([this](const std::string& str) { this->onPreparing(str); });
}

LafTrainer::~LafTrainer() = default;

std::vector<double> LafTrainer::PrepareTraningSet(const fxsequence& seq) const {
  return impl_->prepare_training_set(seq);
}

LafTrainer::Impl::Impl(const boost::property_tree::ptree & settings)
  : cfg_(from_cfg(settings)) {
}

std::vector<double> LafTrainer::Impl::prepare_training_set(const fxsequence& seq) const {
  on_preparing("Estimating genuine positions...");
  double time_adjust;
  double probab;
  double durat;
  auto marks = fxlib::GeniunePositions(seq, cfg_.timeout, cfg_.position == fxposition::fxlong ? fxprofit_long : fxprofit_short, cfg_.margin * cfg_.pip, time_adjust, probab, durat);
  on_preparing("Geniune positions: " + std::to_string(marks.size()));
  return std::vector<double>();
}

}  // namespace fxlib
