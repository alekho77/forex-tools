#include "fxmath.h"

#include <cmath>
#include <algorithm>

namespace fxlib {

static inline bool fxrate_sample_less(const fxrate_sample& lhs, const fxrate_sample& rhs) {
  return lhs.rate < rhs.rate;
}

void RateStats(const fxrate_samples& samples, double& mean, double& variance) {
  mean = 0.0;
  for (const auto& s: samples) {
    mean += s.rate;
  }
  mean /= samples.size();
  variance = 0.0;
  for (const auto& s: samples) {
    const double delta = s.rate - mean;
    variance += delta * delta;
  }
  variance = std::sqrt(variance / (samples.size() - 1));
}

fxrate_distribution BuildDistribution(fxrate_samples& samples, double mean, double variance, size_t distr_size) {
  using namespace std;
  sort(samples.begin(), samples.end(), fxrate_sample_less);
  const double ro = 0;
  const double dr = 6 * variance / distr_size;
  fxrate_distribution distrib;
  distrib.reserve(distr_size + 3);  // there are two extra data
  using citer = fxrate_samples::const_iterator;
  citer iter = samples.cbegin();
  auto count = [](citer& iter, const citer& end, fxdensity_range& density) {
    while ((iter < end) && (iter->rate <= density.rate_up)) {
      ++density.count;
      ++iter;
    }
  };
  const double bottom_bound = ro - dr;
  fxdensity_range data_before = {-numeric_limits<double>::infinity(), 0.0, 0};
  count(iter, samples.cend(), data_before);
  if (limit_iter > limits.cbegin()) {
    cout << "[ERROR] There are " << limits_data_before << " extra data in limits before " << fixed << setprecision(3) << (bottom_bound / g_pip) << " value" << endl;
    throw logic_error("Something has gone wrong!");
  }
  int losses_data_before = 0;
  count(loss_iter, losses.cend(), bottom_bound, losses_data_before);
  if (loss_iter > losses.cbegin()) {
    cout << "[ERROR] There are " << losses_data_before << " extra data in losses before " << fixed << setprecision(3) << (bottom_bound / g_pip) << " value" << endl;
    throw logic_error("Something has gone wrong!");
  }
  for (size_t i = 0; i < distrib.size(); i++) {
    get<0>(distrib[i]) = vo + i * dv;
    count(limit_iter, limits.cend(), get<0>(distrib[i]), get<1>(distrib[i]));
    count(loss_iter, losses.cend(), get<0>(distrib[i]), get<2>(distrib[i]));
  }
  if (limit_iter < limits.cend()) {
    cout << "[NOTE] There are " << (limits.cend() - limit_iter) << " extra data in limits beyond " << fixed << setprecision(3) << (distrib.size() * dv / g_pip) << " value" << endl;
  }
  if ((limits_data_before
       + accumulate(distrib.cbegin(), distrib.cend(), 0, [](auto a, const auto& b) { return a + get<1>(b); })
       + (limits.cend() - limit_iter)) != static_cast<int>(limits.size())) {
    throw logic_error("Sum of limits distribution is not equal the total limits!");
  }
  if (loss_iter < losses.cend()) {
    cout << "[NOTE] There are " << (losses.cend() - loss_iter) << " extra data in losses beyond " << fixed << setprecision(3) << (distrib.size() * dv / g_pip) << " value" << endl;
  }
  if ((losses_data_before
       + accumulate(distrib.cbegin(), distrib.cend(), 0, [](auto a, const auto& b) { return a + get<2>(b); })
       + (losses.cend() - loss_iter)) != static_cast<int>(losses.size())) {
    throw logic_error("Sum of losses distribution is not equal the total losses!");
  }
  return distrib;
}

}  // namespace fxlib
