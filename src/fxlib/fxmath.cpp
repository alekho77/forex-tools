#include "fxmath.h"
#include "math/mathlib/approx.h"

#include <cmath>

namespace fxlib {

void RateStats(const fxrate_samples& samples, double& mean, double& variance) {
  mean = 0.0;
  for (const auto& s: samples) {
    mean += s.margin;
  }
  mean /= samples.size();
  variance = 0.0;
  for (const auto& s: samples) {
    const double delta = s.margin - mean;
    variance += delta * delta;
  }
  variance = std::sqrt(variance / (samples.size() - 1));
}

fxrate_distribution RateDistribution(const fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step) {
  using citer = fxrate_samples::const_iterator;
  auto counter = [end = samples.end()](citer& iter, fxdensity_sample& density) {
    while ((iter < end) && (iter->margin <= density.bound)) {
      ++density.count;
      ++iter;
    }
  };

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

fxrate_probability RateProbability(const fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step) {
  using citer = fxrate_samples::const_iterator;
  auto counter = [end = samples.end(), N = samples.size()](citer& iter, fxprobab_sample& sample) {
    while ((iter < end) && (iter->margin < sample.bound)) {
      --sample.count;
      ++iter;
    }
    sample.prob = static_cast<double>(sample.count) / static_cast<double>(N);
  };

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

fxprobab_coefs ApproxRateProbability(const fxrate_probability& probab) {
  const size_t good_interval = (probab.size() - 3) / 3 + 1;
  mathlib::approx<double, 2> appx;
  for (size_t i = 0; i < good_interval; i++) {
    const double t = probab[i + 1].bound;
    appx(t * t, t, - std::log(probab[i + 1].prob));
  }
  auto res = appx.approach().get_as_tuple();
  return {std::get<0>(res), std::get<1>(res)};
}

//fxdurat_distribution DurationDistribution(const fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step) {
//  using citer = fxrate_samples::const_iterator;
//  auto counter = [end = samples.end()](citer& iter, const double bound, std::vector<double>& time_collector) {
//    while ((iter < end) && (iter->margin < bound)) {
//      time_collector.push_back(iter->period);
//      ++iter;
//    }
//  };
//
//  sort(samples.begin(), samples.end(), [](const fxrate_sample& lhs, const fxrate_sample& rhs) { return lhs.margin < rhs.margin; });
//
//}

}  // namespace fxlib
