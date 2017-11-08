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

fxrate_distribution BuildDistribution(fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step) {
  using namespace std;
  using citer = fxrate_samples::const_iterator;
  auto count = [](citer& iter, const citer& end, fxdensity_range& density) {
    while ((iter < end) && (iter->rate < density.rate_up)) {
      ++density.count;
      ++iter;
    }
  };

  sort(samples.begin(), samples.end(), fxrate_sample_less);

  fxrate_distribution distrib;
  distrib.reserve(distr_size + 3);  // there are two extra data and (distr_size+1) values
  citer iter = samples.cbegin();
  
  distrib.push_back({-numeric_limits<double>::infinity(), rate_from, 0});
  count(iter, samples.cend(), distrib.back());  // checking data before rate_from

  for (size_t i = 0; i <= distr_size; i++) {
    distrib.push_back({rate_from + i * rate_step, rate_from + (i + 1) * rate_step, 0});
    count(iter, samples.cend(), distrib.back());
  }

  distrib.push_back({rate_from + (distr_size + 1) * rate_step, numeric_limits<double>::infinity(), static_cast<size_t>(samples.cend() - iter)});  // remaining data beyond the interval

  return distrib;
}

}  // namespace fxlib
