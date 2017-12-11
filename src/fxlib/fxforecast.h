#pragma once

#include "fxquote.h"
#include "fxmath.h"

#include <memory>

#include <boost/property_tree/ptree_fwd.hpp>

namespace fxlib {

//enum class fxforecast : int {
//  unready,   //* An algorithm has not been ready to make forecast, need new data.
//  positive,  //* An algorithm makes a good forecast regarding data that has been already fed.
//  negative,  //* An algorithm makes a bad forecast regarding data that has been already fed.
//  error      //* Data have been incorrect.
//};

enum class fxposition : int {
  fxlong,
  fxshort
};

struct ForecastInfo {
  fxposition position;
  boost::posix_time::time_duration window;  //* Window size
  boost::posix_time::time_duration timeout;  //* Timeout of wait
  double margin;  //* Expected margin, in rate units
};

/// Interface for making forecasts.
struct IForecaster {
  /// Continuously feeding data into an algorithm.
  /**
    Return estimation in the range [0,1):
    0 - means absolutely negative cast;
    1 - means 100% positive cast.
  */
  virtual double Feed(const fxcandle&) = 0;
  /// Reset an algorithm to the begin condition in order to start new feeding.
  virtual void Reset() = 0;
  /// Get info about an algorithm.
  virtual ForecastInfo Info() const = 0;

  virtual ~IForecaster() {}
};

struct ITrainer {
  virtual std::vector<double> PrepareTraningSet(const fxsequence&) const = 0;
};

std::shared_ptr<IForecaster> CreateForecaster(std::string name, const boost::property_tree::ptree* settings = nullptr);
std::shared_ptr<ITrainer> CreateTrainer(std::string name, const boost::property_tree::ptree* settings = nullptr);

}  // namespace fxlib
