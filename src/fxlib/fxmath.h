#pragma once

#include <vector>
#include <algorithm>

namespace fxlib {

struct fxmargin_sample {
  double margin;  // Expected margin or stop loss, in rate units
  double period;  // Time of awaiting that margin, min
};

using fxmargin_samples = std::vector<fxmargin_sample>;

struct fxdensity_sample {
  double bound;  // Bound of margin
  size_t count;  // Number of samples that are match the margin bound
};

using fxmargin_distribution = std::vector<fxdensity_sample>;

struct fxprobab_sample : fxdensity_sample {
  fxprobab_sample(double b, size_t c, double p) : fxdensity_sample({b, c}), prob(p) {}
  fxprobab_sample(double b, size_t c) : fxprobab_sample(b, c, 0) {}
  double prob;
};

using fxmargin_probability = std::vector<fxprobab_sample>;

struct fxduration_sample : fxdensity_sample {
  fxduration_sample(double b, size_t c, double d, double e) : fxdensity_sample({b, c}), durat(d), error(e) {}
  explicit fxduration_sample(double b) : fxduration_sample(b, 0, 0, 0) {}
  double durat;  // Mean time of waiting for the margin
  double error;  // Variance of the mean value
};

using fxdurat_distribution = std::vector<fxduration_sample>;

/// Approximation coefficients for margin probability.
/**
  Approximating function is P(m) = exp(-(lambda2*m^2 + lambda1*m)).
*/
struct fxprobab_coefs {
  double lambda1;
  double lambda2;
};

/// Approximation coefficients for duration distribution.
/**
  Approximating function is D(m) = T*[1 - exp(-lambda*m)].
*/
struct fxdurat_coefs {
  double T;
  double lambda;
};

static inline fxmargin_samples& fxsort(fxmargin_samples& samples) {
  std::sort(samples.begin(), samples.end(), [](const fxmargin_sample& lhs, const fxmargin_sample& rhs) { return lhs.margin < rhs.margin; });
  return samples;
}

/// Calculate the mean and the variance values for sequence of samples.
void MarginStats(const fxmargin_samples& samples, double& mean, double& variance);

/// Build probability distribution for sequence of margin samples.
/**
  The sequence of margin samples must be sorted.
*/
fxmargin_distribution MarginDistribution(const fxmargin_samples& samples, size_t distr_size, const double from, const double step);

/// Build probability of samples margin.
/**
  The sequence of margin samples must be sorted.
*/
fxmargin_probability MarginProbability(const fxmargin_samples& samples, size_t distr_size, const double from, const double step);

/// Approximate the probability of samples margin.
fxprobab_coefs ApproxMarginProbability(const fxmargin_probability& probab);

/// Build duration distribution for margin waitings.
/**
  The sequence of margin samples must be sorted.
*/
fxdurat_distribution MarginDurationDistribution(const fxmargin_samples& samples, size_t distr_size, const double from, const double step);

/// Approximate margin duration distribution.
fxdurat_coefs ApproxDurationDistribution(const fxdurat_distribution& distrib);

}  // namespace fxlib
