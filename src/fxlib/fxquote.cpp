#include "fxquote.h"
#include "helpers/fxquote_serializable.h"
#include "helpers/fxtime_conversion.h"

namespace fxlib {

void WriteSequence(std::ostream& out, const fxsequence& seq) noexcept(false) {
  using namespace boost::posix_time;
  detail::fxsequence_header_bin header;
  header.periodicity = static_cast<detail::fxperiodicity_bin>(seq.periodicity.total_seconds() / 60);
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
    fxsequence seq = {minutes(header.periodicity),
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
  return {minutes(0), date_period(date(not_a_date_time), date(not_a_date_time)), {}};
}

fxsequence PackSequence(const fxsequence& minseq, const boost::posix_time::time_duration& new_period) {
  using namespace boost::posix_time;
  if (minseq.periodicity != minutes(1)) {
    throw std::logic_error("Wrong source sequence periodicity");
  }
  if (new_period <= minseq.periodicity) {
    throw std::logic_error("Invalid new periodicity");
  }
  if (new_period.total_seconds() % 60 != 0) {
    throw std::logic_error("Periodicity should be multiple minutes.");
  }
  fxsequence resseq = {new_period, minseq.period, {}};
  if (!minseq.candles.empty()) {
    ptime start = minseq.candles.front().time - minutes(1);  // 00:00 means last quote of previous day.
    if (new_period < hours(1)) {
      const auto minper = new_period.minutes();
      if (60 % minper != 0) {
        throw std::logic_error("Periodicity less one hour should divide hour on integer number of portions.");
      }
      time_duration time = start.time_of_day();
      auto mins = (time.minutes() / minper) * minper;
      start = ptime(start.date(), time_duration(time.hours(), mins, 0));
    } else if (new_period < hours(24)) {
      if (new_period.total_seconds() % 3600 != 0) {
        throw std::logic_error("Periodicity should be multiple hours.");
      }
      const auto hper = new_period.hours();
      if (24 % hper != 0) {
        throw std::logic_error("Periodicity less one day should divide 24h on integer number of portions.");
      }
      time_duration time = start.time_of_day();
      auto hs = (time.hours() / hper) * hper;
      start = ptime(start.date(), time_duration(hs, 0, 0));
    } else {
      throw std::logic_error("Not inplemented!");
    }
    time_iterator titr(start, new_period);
    for (auto iter = minseq.candles.cbegin(); iter < minseq.candles.cend(); ++iter) {
      if (iter->time > *titr) {
        while (iter->time > *titr) {
          ++titr;
        }
        resseq.candles.push_back({*titr, iter->open, iter->close, iter->high, iter->low, iter->volume});
        continue;
      }
      fxcandle& curr = resseq.candles.back();
      curr.close = iter->close;
      curr.high = std::max(curr.high, iter->high);
      curr.low = std::min(curr.low, iter->low);
      curr.volume += iter->volume;
    }
  }
  return resseq;
}

}  // namespace fxlib
