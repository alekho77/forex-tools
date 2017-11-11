#include "fxmath.h"

#include <cmath>
#include <algorithm>

namespace fxlib {

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

fxrate_distribution RateDistribution(fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step) {
  using citer = fxrate_samples::const_iterator;
  auto counter = [end = samples.end()](citer& iter, fxdensity_sample& density) {
    while ((iter < end) && (iter->rate <= density.bound)) {
      ++density.count;
      ++iter;
    }
  };

  sort(samples.begin(), samples.end(), [](const fxrate_sample& lhs, const fxrate_sample& rhs) { return lhs.rate < rhs.rate; });

  fxrate_distribution distrib;
  distrib.reserve(distr_size + 3);  // there are two extra data and (distr_size+1) values
  citer iter = samples.cbegin();
  
  distrib.push_back({rate_from - rate_step, 0});
  counter(iter, distrib.back());  // checking data before rate_from

  for (size_t i = 0; i <= distr_size; i++) {
    distrib.push_back({rate_from + i * rate_step, 0});
    counter(iter, distrib.back());
  }

  distrib.push_back({rate_from + (distr_size + 1) * rate_step, static_cast<size_t>(samples.cend() - iter)});  // remaining data beyond the interval

  return distrib;
}

fxrate_probability RateProbability(fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step) {
  using citer = fxrate_samples::const_iterator;
  auto counter = [end = samples.end(), N = samples.size()](citer& iter, fxprobab_sample& sample) {
    while ((iter < end) && (iter->rate < sample.bound)) {
      --sample.count;
      ++iter;
    }
    sample.prob = static_cast<double>(sample.count) / static_cast<double>(N);
  };

  sort(samples.begin(), samples.end(), [](const fxrate_sample& lhs, const fxrate_sample& rhs) { return lhs.rate < rhs.rate; });

  fxrate_probability probab;
  probab.reserve(distr_size + 3);  // there are two extra data and (distr_size+1) values
  citer iter = samples.cbegin();

  probab.push_back({rate_from - rate_step, samples.size()});
  counter(iter, probab.back());  // checking data before rate_from

  for (size_t i = 0; i <= distr_size; i++) {
    probab.push_back({rate_from + i * rate_step, probab.back().count});
    counter(iter, probab.back());
  }

  const size_t rem_count = samples.cend() - iter;  // remaining data beyond the interval
  probab.push_back({rate_from + (distr_size + 1) * rate_step, rem_count, static_cast<double>(rem_count) / static_cast<double>(samples.size())});

  return probab;
}

}  // namespace fxlib
