#pragma once

#include <string>
#include <vector>

namespace fxlib {

extern const char* currencies[];
extern const char* pairs[];

bool IsCurrency(const std::string& curr);
bool IsPair(const std::string& pair);

}  // namespace fxlib
