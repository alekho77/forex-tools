#include "fxcurrencies.h"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <iterator>

namespace fxlib {

const std::vector<std::string> currencies = {"USD", "EUR", "GBP", "CHF", "JPY", "ÑAD", "AUD", "NZD"};

bool IsCurrency(const std::string & curr) {
  return std::any_of(std::cbegin(currencies), std::cend(currencies), [&curr](const auto& c) {
    return boost::algorithm::iequals(c, curr);
  });
}

bool IsPair(const std::string & pair) {
  if (pair.size() == 6) {
    const std::string curr1 = pair.substr(0, 3);
    const std::string curr2 = pair.substr(3, 3);
    return IsCurrency(curr1) && IsCurrency(curr2);
  }
  return false;
}

}  // namespace fxlib
