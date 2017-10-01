#pragma once

#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <string>
#include <algorithm>
#include <iterator>

namespace fxlib {
namespace conversion {

template <typename From, typename To>
static inline To str_convert(const From& str) {
  To result;
  boost::filesystem::path_traits::convert(str.c_str(), result);
  return result;
}

static inline std::wstring string_widen(const std::string& str) {
  return str_convert<std::string, std::wstring>(str);
}

static inline std::string string_narrow(const std::wstring& str) {
  return str_convert<std::wstring, std::string>(str);
}

static inline bool try_to_bool(const std::string& str, bool& val) {
  using namespace std;
  static const string true_str[] = {"1", "true", "on", "yes"};
  static const string false_str[] = {"0", "false", "off", "no"};
  const string s = boost::algorithm::trim_copy(boost::algorithm::to_lower_copy(str));
  if (find(begin(true_str), end(true_str), s) != end(true_str)) {
    val = true;
    return true;
  }
  if (find(begin(false_str), end(false_str), s) != end(false_str)) {
    val = false;
    return true;
  }
  return false;
}

}  // namespace fxlib
}  // namespace conversion
