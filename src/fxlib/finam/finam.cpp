#include "finam.h"

#include <boost/algorithm/string.hpp>

#include <stdexcept>

namespace fxlib {

fxcandle MakeFromFinam(const std::string & line, const std::string& pair_name) noexcept(false) {
  fxcandle candle{};
  // Split line
  boost::smatch what;
  if (!boost::regex_match(line, what, detail::rxFinamExportFormat)) {
    throw std::logic_error("The line is not matched Finam export format.");
  }
  // Check and cast line partions
  if (!boost::algorithm::iequals(what["TICKER"], pair_name)) {
    throw std::logic_error("Found ticker " + what["TICKER"] + " in lieu of " + pair_name);
  }
  int per;  // It assumes that only minutely quote is required.
  if (!boost::conversion::try_lexical_convert(what["PER"], per) || per != 1) {
    throw std::logic_error("Found period " + what["PER"] + " in lieu of 1");
  }
  try {
    candle.time = boost::posix_time::from_iso_string("20" + what["DATE"] + "T" + what["TIME"] + "00");
  } catch (const std::exception& e) {
    throw std::logic_error("Found wrong date-time " + what["DATE"] + " " + what["TIME"] + ": " + e.what());
  }
  if (candle.time.is_not_a_date_time()) {
    throw std::logic_error("Found wrong date-time " + what["DATE"] + " " + what["TIME"]);
  }
  if (!boost::conversion::try_lexical_convert(what["OPEN"], candle.open)) {
    throw std::logic_error("Found wrong OPEN quotation " + what["OPEN"]);
  }
  if (!boost::conversion::try_lexical_convert(what["CLOSE"], candle.close)) {
    throw std::logic_error("Found wrong CLOSE quotation " + what["CLOSE"]);
  }
  if (!boost::conversion::try_lexical_convert(what["HIGH"], candle.high)) {
    throw std::logic_error("Found wrong HIGH quotation " + what["HIGH"]);
  }
  if (!boost::conversion::try_lexical_convert(what["LOW"], candle.low)) {
    throw std::logic_error("Found wrong LOW quotation " + what["LOW"]);
  }
  if (!boost::conversion::try_lexical_convert(what["VOL"], candle.volume)) {
    throw std::logic_error("Found wrong volume field " + what["VOL"]);
  }
  return candle;
}

namespace detail {

const boost::regex rxFinamExportFormat(
  "^(?'TICKER'[A-Z]{6})\\s+(?'PER'[0-9]+)\\s+(?'DATE'[0-9]{6})\\s+(?'TIME'[0-9]{4})\\s+"\
  "(?'OPEN'[0-9]+\\.[0-9]*)\\s+(?'HIGH'[0-9]+\\.[0-9]*)\\s+"\
  "(?'LOW'[0-9]+\\.[0-9]*)\\s+(?'CLOSE'[0-9]+\\.[0-9]*)\\s+(?'VOL'[0-9]+)$");

}
// namespace fxlib
}  // namespace detail