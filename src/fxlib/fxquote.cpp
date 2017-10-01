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
  for (size_t i = 0; i < seq.candles.size() && out; i++) {
    detail::fxcandle_bin candle;
    candle.time = conversion::from_iso_string(boost::posix_time::to_iso_string(seq.candles[i].time));
    candle.open = static_cast<uint32_t>(seq.candles[i].open * 1e6);
    candle.close = static_cast<uint32_t>(seq.candles[i].close * 1e6);
    candle.high = static_cast<uint32_t>(seq.candles[i].high * 1e6);
    candle.low = static_cast<uint32_t>(seq.candles[i].low * 1e6);
    candle.volume = static_cast<uint16_t>(seq.candles[i].volume);
    out << candle;
  }
  return out;
}

std::istream& operator >> (std::istream& in, fxsequence& seq) noexcept(false) {
  detail::fxsequence_header_bin header;
  in >> header;
  if (in) {
    seq.periodicity = static_cast<fxperiodicity>(header.periodicity);
    seq.period = boost::gregorian::date_period(boost::posix_time::from_iso_string(conversion::to_iso_string(header.period.start)).date(),
                                               boost::posix_time::from_iso_string(conversion::to_iso_string(header.period.end)).date());
    //seq.candles.
  }
  return in;
}

}  // namespace fxlib
