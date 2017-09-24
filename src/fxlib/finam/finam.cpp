#include "finam.h"

namespace fxlib {
namespace detail {

const boost::regex FinamExportFormat(
  "^(?'TICKER'[A-Z]{6})\\s+(?'PER'[0-9]+)\\s+(?'DATE'[0-9]{6})\\s+(?'TIME'[0-9]{4})\\s+"\
  "(?'OPEN'[0-9]+\\.[0-9]*)\\s+(?'HIGH'[0-9]+\\.[0-9]*)\\s+"\
  "(?'LOW'[0-9]+\\.[0-9]*)\\s+(?'CLOSE'[0-9]+\\.[0-9]*)\\s+(?'VOL'[0-9]+)$");

}  // namespace fxlib
}  // namespace detail