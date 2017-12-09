#include "laf_algorithm.h"

#include <boost/property_tree/ptree.hpp>

namespace fxlib {

//namespace {
//ForecastInfo from_cfg(const boost::property_tree::ptree& settings) {
//  ForecastInfo info{};
//  info.position = settings.get<std::string>("position") == "long" ? fxposition::fxlong : fxposition::fxshort;
//  info.window = settings.get<int>("window");
//  info.timeout = settings.get<int>("timeout");
//  info.margin = settings.get<double>("margin");
//  return info;
//}
//}


LafTrainer::LafTrainer(const boost::property_tree::ptree& /*settings*/) {
}

std::vector<double> LafTrainer::PrepareTraningSet(const fxsequence& /*seq*/) const {
  return std::vector<double>();
}

}  // namespace fxlib
