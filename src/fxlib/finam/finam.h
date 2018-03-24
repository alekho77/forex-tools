#pragma once

#include "fxlib/fxquote.h"

#include <boost/regex.hpp>

#include <string>

namespace fxlib {

fxcandle MakeFromFinam(const std::string& line, const std::string& pair_name) noexcept(false);

namespace detail {

// Finam is a Russian website that allows you to get at least two months worth of one-minute Forex data.
extern const boost::regex rxFinamExportFormat;

}  // namespace detail
}  // namespace fxlib
