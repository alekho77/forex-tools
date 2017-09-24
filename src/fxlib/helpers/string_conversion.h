#pragma once

#include <boost/filesystem.hpp>

#include <string>

namespace fxlib {
namespace conversion {

template <typename From, typename To>
To str_convert(const From& str) {
  To result;
  boost::filesystem::path_traits::convert(str.c_str(), result);
  return result;
}

std::wstring string_widen(const std::string& str) {
  return str_convert<std::string, std::wstring>(str);
}

std::string string_narrow(const std::wstring& str) {
  return str_convert<std::wstring, std::string>(str);
}

}  // namespace fxlib
}  // namespace conversion
