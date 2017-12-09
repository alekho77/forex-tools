#include "fxforecast.h"
#include "fxlib/dummy_algorithm.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/algorithm/string.hpp>

namespace fxlib {

std::shared_ptr<IForecaster> CreateForecaster(std::string name, const boost::property_tree::ptree* settings /*= nullptr*/) {
  boost::algorithm::to_lower(name);
  if (name == "dummy") {
    return std::make_shared<DummyAlgorithm>(*settings);
  }
  return std::shared_ptr<IForecaster>();
}

std::shared_ptr<ITrainer> CreateTrainer(std::string /*name*/, const boost::property_tree::ptree* /*settings*/ /*= nullptr*/) {
  //boost::algorithm::to_lower(name);
  //if (name == "dummy") {
  //  return std::make_shared<DummyAlgorithm>(*settings);
  //}
  return std::shared_ptr<ITrainer>();
}

}  // namespace fxlib
