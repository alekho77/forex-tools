#pragma once

#include <vector>
#include <algorithm>

namespace fxlib {

struct fxrate_sample {
  double margin;  // Expected margin or stop loss, in rate units
  double period;  // Time of awaiting that margin, min
};

using fxrate_samples = std::vector<fxrate_sample>;

struct fxdensity_sample {
  double bound;  // Bound of margin
  size_t count;  // Number of samples that are match the margin bound
};

using fxrate_distribution = std::vector<fxdensity_sample>;

struct fxprobab_sample : fxdensity_sample {
  fxprobab_sample(double b, size_t c, double p) : fxdensity_sample({b, c}), prob(p) {}
  fxprobab_sample(double b, size_t c) : fxprobab_sample(b, c, 0) {}
  double prob;
};

using fxrate_probability = std::vector<fxprobab_sample>;

struct fxduration_sample : fxdensity_sample {
  fxduration_sample(double b, size_t c, double d, double e) : fxdensity_sample({b, c}), durat(d), error(e) {}
  explicit fxduration_sample(double b) : fxduration_sample(b, 0, 0, 0) {}
  double durat;  // Mean time of waiting for the margin
  double error;  // Variance of the mean value
};

using fxdurat_distribution = std::vector<fxduration_sample>;

/// Approximation coefficients for margin probability.
struct fxprobab_coefs {
  double lambda1;
  double lambda2;
};

static inline fxrate_samples& fxsort(fxrate_samples& samples) {
  std::sort(samples.begin(), samples.end(), [](const fxrate_sample& lhs, const fxrate_sample& rhs) { return lhs.margin < rhs.margin; });
  return samples;
}

/// Calculate the mean and the variance values for sequence of samples.
void RateStats(const fxrate_samples& samples, double& mean, double& variance);

/// Build probability distribution for sequence of margin samples.
/**
  The sequence of margin samples must be sorted.
*/
fxrate_distribution RateDistribution(const fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step);

/// Build probability of samples margin.
/**
  The sequence of margin samples must be sorted.
*/
fxrate_probability RateProbability(const fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step);

/// Approximate the probability of samples margin.
fxprobab_coefs ApproxRateProbability(const fxrate_probability& probab);

/// Build duration distribution for margin waitings.
/**
  The sequence of margin samples must be sorted.
*/
fxdurat_distribution DurationDistribution(const fxrate_samples& samples, size_t distr_size, const double rate_from, const double rate_step);

}  // namespace fxlib
