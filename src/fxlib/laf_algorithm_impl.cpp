#include "laf_algorithm_impl.h"

#include "helpers/string_conversion.h"
#include "helpers/nnetwork_helpers.h"

namespace fxlib {

namespace details {

laf_cfg laf_from_ptree(const boost::property_tree::ptree& settings) {
  laf_cfg cfg{};
  cfg.position = settings.get<std::string>("position") == "long" ? fxposition::fxlong : fxposition::fxshort;
  cfg.window = conversion::duration_from_string(settings.get<std::string>("window"));
  cfg.timeout = conversion::duration_from_string(settings.get<std::string>("timeout"));
  cfg.margin = settings.get<double>("margin");
  cfg.pip = settings.get<double>("pip");
  cfg.inputs = settings.get<int>("inputs");
  cfg.step = conversion::duration_from_string(settings.get<std::string>("step"));
  return cfg;
}

}  // namespace details


LafAlgorithm::Impl::Impl(const boost::property_tree::ptree& settings) : cfg_(details::laf_from_ptree(settings)) {}

double LafAlgorithm::Impl::feed(const fxcandle& /*candle*/) {
  return 0;
}

void LafAlgorithm::Impl::reset() {

}

ForecastInfo LafAlgorithm::Impl::info() const {
  return cfg_;
}

}  // namespace fxlib
