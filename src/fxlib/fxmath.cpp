#include "fxmath.h"
#include "math/mathlib/approx.h"
#include "math/mathlib/fapprox.h"

#include <cmath>

namespace fxlib {

void MarginStats(const fxmargin_samples& samples, double& mean, double& variance) {
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

fxmargin_distribution MarginDistribution(const fxmargin_samples& samples, size_t distr_size, const double from, const double step) {
  using citer = fxmargin_samples::const_iterator;
  auto worker = [end = samples.end()](citer& iter, fxdensity_sample& density) {
    while ((iter < end) && (iter->margin <= density.bound)) {
      ++density.count;
      ++iter;
    }
  };

  fxmargin_distribution distrib;
  distrib.reserve(distr_size + 3);  // there are two extra data and (distr_size+1) values
  citer iter = samples.cbegin();
  
  distrib.push_back({from - step, 0});
  worker(iter, distrib.back());  // checking data before from

  for (size_t i = 0; i <= distr_size; i++) {
    distrib.push_back({from + i * step, 0});
    worker(iter, distrib.back());
  }

  distrib.push_back({from + (distr_size + 1) * step, static_cast<size_t>(samples.cend() - iter)});  // remaining data beyond the interval

  return distrib;
}

fxmargin_probability MarginProbability(const fxmargin_samples& samples, size_t distr_size, const double from, const double step) {
  using citer = fxmargin_samples::const_iterator;
  auto worker = [end = samples.end(), N = samples.size()](citer& iter, fxprobab_sample& sample) {
    while ((iter < end) && (iter->margin < sample.bound)) {
      --sample.count;
      ++iter;
    }
    sample.prob = static_cast<double>(sample.count) / static_cast<double>(N);
  };

  fxmargin_probability probab;
  probab.reserve(distr_size + 3);  // there are two extra data and (distr_size+1) values
  citer iter = samples.cbegin();

  probab.push_back({from - step, samples.size()});
  worker(iter, probab.back());  // checking data before from

  for (size_t i = 0; i <= distr_size; i++) {
    probab.push_back({from + i * step, probab.back().count});
    worker(iter, probab.back());
  }

  const size_t rem_count = samples.cend() - iter;  // remaining data beyond the interval
  probab.push_back({from + (distr_size + 1) * step, rem_count, static_cast<double>(rem_count) / static_cast<double>(samples.size())});

  return probab;
}

fxprobab_coefs ApproxMarginProbability(const fxmargin_probability& probab) {
  const size_t good_interval = (probab.size() - 3) / 3 + 1;
  mathlib::approx<double, 2> appx;
  for (size_t i = 0; i < good_interval; i++) {
    const double t = probab[i + 1].bound;
    appx(t, t * t, - std::log(probab[i + 1].prob));
  }
  auto res = appx.approach().get_as_tuple();
  return {std::get<0>(res), std::get<1>(res)};
}

fxdurat_distribution MarginDurationDistribution(const fxmargin_samples& samples, size_t distr_size, const double from, const double step) {
  using citer = fxmargin_samples::const_iterator;
  auto worker = [end = samples.end()](citer& iter, fxduration_sample& sample) {
    std::vector<double> time_collector;
    while ((iter < end) && (iter->margin < sample.bound)) {
      time_collector.push_back(iter->period);
      ++iter;
    }
    if (!time_collector.empty()) {
      for (const double& t : time_collector) {
        sample.durat += t;
      }
      sample.durat /= time_collector.size();
      if (time_collector.size() > 1) {
        for (const double& t : time_collector) {
          const double d = t - sample.durat;
          sample.error += d * d;
        }
        sample.error = sqrt(sample.error / (time_collector.size() - 1));
      }
      sample.count = time_collector.size();
    }
  };

  fxdurat_distribution distrib;
  distrib.reserve(distr_size + 3);  // there are two extra data and (distr_size+1) values
  citer iter = samples.cbegin();

  distrib.push_back(fxduration_sample{from - step});
  worker(iter, distrib.back());  // checking data before from

  for (size_t i = 0; i <= distr_size; i++) {
    distrib.push_back(fxduration_sample{from + i * step});
    worker(iter, distrib.back());
  }

  distrib.push_back(fxduration_sample{from + (distr_size + 1) * step, static_cast<size_t>(samples.cend() - iter), 0, 0});  // remaining data beyond the interval

  return distrib;
}

fxdurat_coefs ApproxDurationDistribution(const fxdurat_distribution& distrib) {
  using namespace std;
  const size_t good_interval = (distrib.size() - 3) / 2 + 1;
  const size_t magic_number = (distrib.size() - 3) / 10;
  mathlib::fapprox<double(double,double)> appx;
  for (size_t i = 0; i < good_interval; i++) {
    appx([t = distrib[i + 1].bound, D = distrib[i + 1].durat](double T, double lambda) {
      return T * (1 - exp(- (lambda * t))) - D; });
  }
  double To = 0;
  for (size_t i = distrib.size() - magic_number - 1; i < distrib.size() - 1; i++) {
    To += distrib[i + 1].durat;
  }
  To /= magic_number;
  const double lambda = -log(1 - distrib[magic_number].durat / To) / distrib[magic_number].bound;
  auto res = appx.approach(To, lambda).get_as_tuple();
  return{get<0>(res), get<1>(res)};
}

}  // namespace fxlib
