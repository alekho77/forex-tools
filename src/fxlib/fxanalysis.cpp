#include "fxanalysis.h"
#include "math/mathlib/nonlsyseq.h"

#include <boost/optional.hpp>

namespace fxlib {

double MaxMargin(const fxprobab_coefs& pcoefs, const fxdurat_coefs& dcoefs, double tadust) {
  using namespace std;
  auto delta_1 = [&pcoefs, &dcoefs, tadust](double m) { 
    return tadust * (2 * pcoefs.lambda2 * m * m + pcoefs.lambda1 * m - 1) * exp(pcoefs.lambda2 * m * m + pcoefs.lambda1 * m) - dcoefs.T; };
  mathlib::nonlinear_equations<double(double)> syseq1({delta_1});
  const double m1 = syseq1.solve((sqrt(8 * pcoefs.lambda2 + pcoefs.lambda1 * pcoefs.lambda1) - pcoefs.lambda1) / (4 * pcoefs.lambda2))[0][0];

  auto delta_2 = [&pcoefs, &dcoefs, &delta_1](double m) { 
    return delta_1(m) + dcoefs.T * (1 + dcoefs.lambda * m) * exp(-dcoefs.lambda * m); };
  mathlib::nonlinear_equations<double(double)> syseq2({delta_2});

  return syseq2.solve(m1)[0][0];
}

markers GeniunePositions(const fxsequence& seq, const boost::posix_time::time_duration& timeout, fprofit_t profit, double expected_margin, double& adjust, double& probab) {
  using namespace std;
  fxlib::markers marks;
  adjust = 0;
  boost::optional<boost::posix_time::ptime> prev_time;
  size_t count = 0;
  const auto& rates = seq.candles;
  for (auto iopen = rates.cbegin(); (iopen < rates.cend()) && (rates.back().time - iopen->time >= timeout); ++iopen, ++count) {
    for (auto iclose = iopen + 1; (iclose < rates.cend()) && (iclose->time - iopen->time <= timeout); ++iclose) {
      if (profit(*iclose, *iopen) >= expected_margin) {
        marks.emplace_back(iopen->time);
        break;
      }
    }
    if (prev_time.is_initialized()) {
      const boost::posix_time::time_duration dt = iopen->time - *prev_time;
      adjust += dt.total_seconds() / 60.0;
    }
    prev_time = iopen->time;
  }
  adjust /= (count - 1);
  probab = double(marks.size()) / double(count);
  return marks;
}

}  // namespace fxlib
