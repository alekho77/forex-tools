#pragma once

#include "fxtime_serializable.h"

#include <boost/lexical_cast.hpp>

#include <iostream>

namespace fxlib {

static inline std::ostream& operator << (std::ostream& out, const fxtime& time) noexcept {
  bool valid = false;
  for (const uint8_t b : time.data) {
    if (b == 0xDD) {
      out << "T";
      valid = true;
    } else {
      const char dh = (b >> 4) + '0';
      const char dl = (b & 0xF) + '0';
      if (dh > '9' || dl > '9') {
        valid = false;
        break;
      }
      out << dh << dl;
    }
  }
  if (!valid) {
    out.setstate(std::ios::failbit);
  }
  return out;
}

static inline std::istream& operator >> (std::istream& in, fxtime& time) noexcept {
  bool valid = false;
  std::string str;
  in >> str;
  if (str.length() == 15 && str[8] == 'T') {
    valid = true;
    auto ib = std::begin(time.data);
    for (size_t i = 0; i < 15; ++i, ++ib) {
      if (i == 8) {
        *ib = 0xDD;
      } else {
        const char dh = str[i] - '0';
        const char dl = str[++i] - '0';
        if ((dh < 0 || dh > 9) || (dl < 0 || dl > 9)) {
          valid = false;
          break;
        }
        *ib = (dh << 4) | dl;
      }
    }
  }
  if (!valid) {
    in.setstate(std::ios::failbit);
  }
  return in;
}

namespace conversion {

static inline std::string to_iso_string(const fxtime& t) noexcept(false) {
  try {
    return boost::lexical_cast<std::string>(t);
  } catch (const std::exception& e) {
    throw std::bad_cast::__construct_from_string_literal(e.what());
  }
}

static inline fxtime from_iso_string(const std::string& str) noexcept(false) {
  try {
    return boost::lexical_cast<fxtime>(str);
  } catch (const std::exception& e) {
    throw std::bad_cast::__construct_from_string_literal(e.what());
  }
}

static inline bool try_to_iso_string(const fxtime& t, std::string& str) noexcept {
  return boost::conversion::try_lexical_convert(t, str);
}

static inline bool try_from_iso_string(const std::string& str, fxtime& t) noexcept {
  return boost::conversion::try_lexical_convert(str, t);
}

}  // namespace conversion
}  // namespace fxlib

