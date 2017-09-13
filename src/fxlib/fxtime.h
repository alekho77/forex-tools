#pragma once

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

#include <array>
#include <iostream>

namespace fxlib {

#pragma pack(push, 1)

struct alignas(1) fxtime {
  std::array<uint8_t, 8> data = {0};  // iso time string (like 20020131T235959) packed with BCD
};

#pragma pack(pop)

// Returns time period for certain date when the market is opened.
boost::posix_time::time_period ForexOpenHours(const boost::gregorian::date& date);

static inline std::ostream& operator << (std::ostream& out, const fxtime& time) {
  bool valid = false;
  for (const uint8_t b: time.data) {
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

static inline std::istream& operator >> (std::istream& in, fxtime& time) {
  bool valid = false;
  std::string str;
  in >> str;
  if (str.length() == 15 && str[8] == 'T') {
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

}  // namespace fxlib
