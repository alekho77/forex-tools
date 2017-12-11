#include "laf_algorithm.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>

namespace fxlib {

namespace {

struct laf_trainer_cfg {
  fxposition position;
  boost::posix_time::time_duration window;  //* Window size
  boost::posix_time::time_duration timeout;  //* Timeout of wait
  double margin;  //* Expected margin, in rate units
  int inputs;  //* Number of inputs: 6, 12, 24 ...
  int step;  //* Number of minutes that are used for each input.
};

//laf_trainer_cfg from_cfg(const boost::property_tree::ptree& settings) {
//  const boost::regex rx_period("(\\d+)([mhdw])");
//  laf_trainer_cfg cfg{};
//  cfg.position = settings.get<std::string>("position") == "long" ? fxposition::fxlong : fxposition::fxshort;
//  cfg.window = settings.get<int>("window");
//  cfg.timeout = settings.get<int>("timeout");
//  cfg.margin = settings.get<double>("margin");
//  cfg.inputs = settings.get<int>("inputs");
//  std::string sstep = settings.get<std::string>("step");
//
//  return cfg;
//}

}  // namespace


LafTrainer::LafTrainer(const boost::property_tree::ptree& /*settings*/) {
}

std::vector<double> LafTrainer::PrepareTraningSet(const fxsequence& /*seq*/) const {
  return std::vector<double>();
}

}  // namespace fxlib
