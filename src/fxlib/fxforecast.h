#pragma once

#include "fxquote.h"
#include "fxmath.h"

#include <memory>

#include <boost/property_tree/ptree_fwd.hpp>

namespace fxlib {

enum class fxforecast : int {
  unready,   //* An algorithm has not been ready to make forecast, need new data.
  positive,  //* An algorithm makes a good forecast regarding data that has been already fed.
  negative,  //* An algorithm makes a bad forecast regarding data that has been already fed.
  error      //* Data have been incorrect.
};

enum class fxposition : int {
  fxlong,
  fxshort
};

struct ForecastInfo {
  fxposition position;
  int window;  //* Window size, in min
  int timeout;  //* Timeout of wait, in min
  double margin;  //* Expected margin, in rate units
  double probab;
  double durat;
};

/// Interface for making forecasts.
struct IForecaster {
  /// Continuously feeding data into an algorithm.
  virtual fxforecast Feed(const fxcandle&) = 0;
  /// Reset an algorithm to the begin condition in order to start new feeding.
  virtual void Reset() = 0;
  /// Get info about an algorithm.
  virtual ForecastInfo Info() const = 0;

  virtual ~IForecaster() {}
};

std::shared_ptr<IForecaster> CreateForecaster(std::string name, const boost::property_tree::ptree* settings = nullptr);

}  // namespace fxlib
