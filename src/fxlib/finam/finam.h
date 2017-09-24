#pragma once

#include <boost/regex.hpp>

namespace fxlib {
namespace detail {

// Finam is a Russian website that allows you to get at least two months worth of one-minute Forex data.
extern const boost::regex FinamExportFormat;

}  // namespace fxlib
}  // namespace detail
