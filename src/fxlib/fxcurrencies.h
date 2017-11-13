#pragma once

#include <string>
#include <vector>

namespace fxlib {

extern const char* fxcurrencies[];
extern const char* fxpairs[];

bool IsCurrency(const std::string& curr);
bool IsPair(const std::string& pair);

}  // namespace fxlib
