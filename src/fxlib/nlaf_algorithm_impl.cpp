#include "nlaf_algorithm_impl.h"

#include "helpers/string_conversion.h"

namespace fxlib {

namespace details {

nlaf_cfg nlaf_from_ptree(const boost::property_tree::ptree& settings) {
  nlaf_cfg cfg{};
  cfg.position = settings.get<std::string>("position") == "long" ? fxposition::fxlong : fxposition::fxshort;
  cfg.window = conversion::duration_from_string(settings.get<std::string>("window"));
  cfg.timeout = conversion::duration_from_string(settings.get<std::string>("timeout"));
  cfg.margin = settings.get<double>("margin");
  cfg.pip = settings.get<double>("pip");
  cfg.type = settings.get<std::string>("type");
  cfg.step = conversion::duration_from_string(settings.get<std::string>("step"));
  //cfg.mean = settings.get("params.mean", 0.0);
  //cfg.var = settings.get("params.variance", 1.0);
  return cfg;
}

}  // namespace details


//NLafAlgorithm::Impl::Impl(const boost::property_tree::ptree& settings)
//  : cfg_(details::laf_from_ptree(settings)) {
//  laf_impl_ = details::make_laf_impl(cfg_.type);
//  inputs_.resize(laf_impl_->inputs_number(), 0.0);
//  laf_impl_->restore_network(settings.get_child("params"));
//}

//double LafAlgorithm::Impl::feed(const fxcandle& candle) {
//  using namespace boost::posix_time;
//  if (!time_bound_.is_initialized()) {
//    ptime start = candle.time - minutes(1);
//    if (cfg_.step < hours(1)) {
//      const auto mstep = cfg_.step.total_seconds() / 60;
//      const time_duration time = start.time_of_day();
//      const auto mins = (time.minutes() / mstep) * mstep;
//      start = ptime(start.date(), time_duration(time.hours(), mins, 0));
//    } else if (cfg_.step < hours(24)) {
//      const auto hstep = cfg_.step.total_seconds() / 3600;
//      const time_duration time = start.time_of_day();
//      const auto hs = (time.hours() / hstep) * hstep;
//      start = ptime(start.date(), time_duration(hs, 0, 0));
//    } else {
//      throw std::logic_error("not implemented");
//    }
//    time_bound_ = time_iterator(start, cfg_.step);
//  }
//  if (candle.time > **time_bound_) {
//    while (candle.time > **time_bound_) {
//      ++(*time_bound_);
//    }
//    for (size_t i = 1; i < inputs_.size(); i++) {
//      inputs_[i - 1] = inputs_[i];
//    }
//    aggr_candle_ = candle;
//    aggr_candle_.time = **time_bound_;
//  } else {
//    aggr_candle_.close = candle.close;
//    aggr_candle_.high = std::max(aggr_candle_.high, candle.high);
//    aggr_candle_.low = std::min(aggr_candle_.low, candle.low);
//    aggr_candle_.volume += candle.volume;
//  }
//  inputs_.back() = cfg_.normalize(aggr_candle_);
//  return laf_impl_->apply_network(inputs_);
//}

//void LafAlgorithm::Impl::reset() {
//  inputs_ = std::vector<double>(laf_impl_->inputs_number(), 0.0);
//  time_bound_.reset();
//  aggr_candle_ = fxcandle();
//}

//ForecastInfo LafAlgorithm::Impl::info() const {
//  return cfg_;
//}

}  // namespace fxlib
