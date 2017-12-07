#pragma once

#include "fxmath.h"
#include "fxquote.h"

namespace fxlib {

using markers = std::vector<boost::posix_time::ptime>;

double MaxMargin(const fxprobab_coefs& pcoefs, const fxdurat_coefs& dcoefs, double tadust);

markers GeniunePositions(const fxsequence& seq, const boost::posix_time::time_duration& timeout, fprofit_t profit, double expected_margin, double& adjust);

}  // namespace fxlib
