#include "fxquote.h"
#include "helpers/fxquote_serializable.h"
#include "helpers/fxtime_conversion.h"

namespace fxlib {

void WriteSequence(std::ostream& out, const fxsequence& seq) noexcept(false) {
  using namespace boost::posix_time;
  detail::fxsequence_header_bin header;
  header.periodicity = static_cast<detail::fxperiodicity_bin>(seq.periodicity);
  header.count = static_cast<uint32_t>(seq.candles.size());
  header.period.start = conversion::from_iso_string(to_iso_string(ptime(seq.period.begin())));
  header.period.end = conversion::from_iso_string(to_iso_string(ptime(seq.period.end())));
  out << header;
  for (size_t i = 0; i < seq.candles.size() && out; i++) {
    detail::fxcandle_bin candle;
    candle.time = conversion::from_iso_string(to_iso_string(seq.candles[i].time));
    candle.open = static_cast<uint32_t>(seq.candles[i].open * 1e6);
    candle.close = static_cast<uint32_t>(seq.candles[i].close * 1e6);
    candle.high = static_cast<uint32_t>(seq.candles[i].high * 1e6);
    candle.low = static_cast<uint32_t>(seq.candles[i].low * 1e6);
    candle.volume = static_cast<uint16_t>(seq.candles[i].volume);
    out << candle;
  }
  out.flush();
}

fxsequence ReadSequence(std::istream& in) noexcept(false) {
  using namespace boost::gregorian;
  using namespace boost::posix_time;
  detail::fxsequence_header_bin header;
  in >> header;
  if (in) {
    fxsequence seq = {static_cast<fxperiodicity>(header.periodicity),
                      date_period(from_iso_string(conversion::to_iso_string(header.period.start)).date(),
                                  from_iso_string(conversion::to_iso_string(header.period.end)).date()),
                      {}};
    seq.candles.reserve(header.count);
    for (size_t i = 0; i < header.count; i++) {
      detail::fxcandle_bin candle;
      in >> candle;
      seq.candles.push_back({from_iso_string(conversion::to_iso_string(candle.time)), 
                            static_cast<double>(candle.open) * 1e-6,
                            static_cast<double>(candle.close) * 1e-6,
                            static_cast<double>(candle.high) * 1e-6,
                            static_cast<double>(candle.low) * 1e-6,
                            candle.volume});
    }
    return seq;
  }
  return {fxperiodicity::tick, date_period(date(not_a_date_time), date(not_a_date_time)), {}};
}

}  // namespace fxlib
