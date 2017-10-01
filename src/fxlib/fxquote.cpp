#include "fxquote.h"
#include "helpers/fxquote_serializable.h"
#include "helpers/fxtime_conversion.h"

namespace fxlib {

std::ostream& operator << (std::ostream& out, const fxsequence& seq) noexcept(false) {
  detail::fxsequence_header_bin header;
  header.periodicity = static_cast<detail::fxperiodicity_bin>(seq.periodicity);
  header.period.start = conversion::from_iso_string(boost::posix_time::to_iso_string(boost::posix_time::ptime(seq.period.begin())));
  header.period.end = conversion::from_iso_string(boost::posix_time::to_iso_string(boost::posix_time::ptime(seq.period.end())));
  out << header;
  return out;
}

std::istream& operator >> (std::istream& in, fxsequence& seq) noexcept(false) {
  bool valid = false;

  if (!valid) {
    in.setstate(std::ios::failbit);
  }
  return in;
}

}  // namespace fxlib
