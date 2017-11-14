#include "fxanalysis.h"
#include "math/mathlib/nonlsyseq.h"

namespace fxlib {

double MaxMargin(const fxprobab_coefs& pcoefs, const fxdurat_coefs& dcoefs) {
  using namespace std;
  auto delta_1 = [&pcoefs, &dcoefs](double m) { 
    return (2 * pcoefs.lambda2 * m * m + pcoefs.lambda1 * m - 1) * exp(pcoefs.lambda2 * m * m + pcoefs.lambda1 * m) - dcoefs.T; };
  mathlib::nonlinear_equations<double(double)> syseq1({delta_1});
  const double m1 = syseq1.solve((sqrt(8 * pcoefs.lambda2 + pcoefs.lambda1 * pcoefs.lambda1) - pcoefs.lambda1) / (4 * pcoefs.lambda2))[0][0];

  auto delta_2 = [&pcoefs, &dcoefs, &delta_1](double m) { 
    return delta_1(m) + dcoefs.T * (1 + dcoefs.lambda * m) * exp(-dcoefs.lambda * m); };
  mathlib::nonlinear_equations<double(double)> syseq2({delta_2});

  return syseq2.solve(m1)[0][0];
}

}  // namespace fxlib
