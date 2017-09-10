#pragma once

#include <string>
#include <vector>

namespace fxlib {

extern const std::vector<std::string> currencies;

bool IsCurrency(const std::string& curr);
bool IsPair(const std::string& pair);

}  // namespace fxlib
