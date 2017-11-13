#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"
#include "math/mathlib/approx.h"

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/math/distributions/students_t.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <exception>
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>
#include <tuple>
#include <numeric>

using boost::posix_time::time_duration;
using boost::posix_time::minutes;

using fxlib::fxrate_samples;
using fxlib::fxrate_distribution;

namespace {
boost::filesystem::path g_srcbin;
boost::filesystem::path g_outpath;
double g_pip = 0.0001;
double g_alpha = 0.1;
size_t g_distr_size;
}

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm) {
  using namespace std;
  options_description basic_desc("Basic options", 200);
  basic_desc.add_options()
    ("help,h", "Show help");
  options_description generic_desc("Generic analyze options", 200);
  bool quick_mode = false;
  generic_desc.add_options()
    ("source,s", value<string>()->required()->value_name("pair-bin")->notifier(
      [](const string& srcname) { g_srcbin = boost::filesystem::canonical(srcname); }), "Path to compiled (binary) quotes.")
    ("quick,q", bool_switch(&quick_mode), "Start a quick (simple) analyze.");
  options_description quick_desc("Quick analyze options", 200);
  quick_desc.add_options()
    ("position,p", value<string>()->required()->value_name("{long|short}"), "What position to be analyzed.")
    ("timeout,t", value<string>()->required()->value_name("n{m,h,d,w}"), "How far to look into future (minutes, hours, days, weeks).")
    ("out,o", value<string>()->value_name("[path]")->implicit_value("")->notifier(
      [](const string& outname) { g_outpath = boost::filesystem::canonical(outname); }), "Optionally distributions and probabilities can be written into output files.")
    ("distsize,d", value<size_t>(&g_distr_size)->default_value(150)->value_name("size"), "Number of intervals to build a distribution.");
  options_description additional_desc("Additional options", 200);
  additional_desc.add_options()
    ("pip,z", value<double>(&g_pip)->value_name("size"), "Pip size, usually 0.0001 or 0.01.")
    ("alpha,a", value<double>(&g_alpha)->value_name("alpha"), "Risk level (probability: 0.1, 0.01, 0.001 ...).");
  const auto list_desc = {basic_desc, generic_desc, quick_desc, additional_desc};
  try {
    store(command_line_parser(argc, argv).options(basic_desc).options(generic_desc).allow_unregistered().run(), vm);
    notify(vm);
    if (vm.count("help")) {
      cout << list_desc;
      return false;
    }
    if (quick_mode) {
      options_description desc;
      store(parse_command_line(argc, argv, desc.add(generic_desc).add(quick_desc).add(additional_desc)), vm);
      notify(vm);
    } else {
      throw error("No one mode of analyze has been found!");
    }
  } catch (const exception& e) {
    cout << "[ERROR] Command line: " << e.what() << endl;
    cout << list_desc;
    return false;
  }
  return true;
}

fxrate_distribution BuildDistribution(fxrate_samples& samples, const double from, const double step, const std::string& name) {
  using namespace std;
  fxrate_distribution distrib = fxlib::RateDistribution(samples, g_distr_size, from, step);
  if (distrib.size() != g_distr_size + 3) {
    throw logic_error("Invalid size of " + name + " distribution!");
  }
  if (distrib.front().count != 0) {
    cout << "[ERROR] There are " << distrib.front().count << " extra data in " << name << " before " << fixed << setprecision(3) << (distrib.front().bound / g_pip) << " value" << endl;
    throw logic_error("Something has gone wrong!");
  }
  if (distrib.back().count != 0) {
    cout << "[NOTE] There are " << distrib.back().count << " extra data in " << name << " beyond " << fixed << setprecision(3) << (distrib.back().bound / g_pip) << " value" << endl;
  }
  if (accumulate(distrib.cbegin(), distrib.cend(), size_t(0), [](size_t a, const auto& b) { return a + b.count; }) != samples.size()) {
    throw logic_error("Sum of " + name + " distribution is not equal the total number!");
  }
  return distrib;
}

struct duration_data {
  double durat;  // mean time of duration, min.
  double error;
  size_t count;
};
struct probab_data {
  double rate;
  double prof;
  double loss;
  duration_data prof_d;
  duration_data loss_d;
};
using simple_probability = std::vector<probab_data>;
simple_probability BuildProbability(fxrate_samples& limits, fxrate_samples& losses,
                                    const double from, const double step,
                                    fxlib::fxprobab_coefs& lambda_prof, fxlib::fxprobab_coefs& lambda_loss) {
  using namespace std;
  simple_probability probab(g_distr_size + 1, probab_data());
  
  auto lim_probab = fxlib::RateProbability(limits, g_distr_size, from, step);
  auto los_probab = fxlib::RateProbability(losses, g_distr_size, from, step);

  if (lim_probab.front().count != limits.size()) {
    cout << "[ERROR] There are extra data in limits before " << fixed << setprecision(3) << (lim_probab.front().bound / g_pip) << endl;
    throw logic_error("Something has gone wrong!");
  }
  if (los_probab.front().count != losses.size()) {
    cout << "[NOTE] There are extra data in losses before " << fixed << setprecision(3) << (los_probab.front().bound / g_pip) << endl;
    throw logic_error("Something has gone wrong!");
  }
  if (lim_probab.back().count > 0) {
    cout << "[NOTE] There are " << lim_probab.back().count << " extra data in limits beyond " << fixed << setprecision(3) << (lim_probab.back().bound / g_pip) << " value" << endl;
  }
  if (los_probab.back().count > 0) {
    cout << "[NOTE] There are " << los_probab.back().count << " extra data in losses beyond " << fixed << setprecision(3) << (los_probab.back().bound / g_pip) << " value" << endl;
  }

  auto limit_iter = limits.cbegin();
  auto loss_iter = losses.cbegin();
  using citer = fxrate_samples::const_iterator;
  auto count = [](citer& iter, const citer& end, const double bound, vector<double>& time_collector) {
    while ((iter < end) && (iter->margin < bound)) {
      time_collector.push_back(iter->period);
      ++iter;
    }
  };
  {
    const double bottom_bound = from - step;
    vector<double> tcollect;
    count(limit_iter, limits.cend(), bottom_bound, tcollect);
    count(loss_iter, losses.cend(), bottom_bound, tcollect);
  }
  for (size_t i = 0; i < probab.size(); i++) {
    probab[i].rate = lim_probab[i + 1].bound;
    {
      vector<double> tcollect;
      count(limit_iter, limits.cend(), probab[i].rate, tcollect);
      probab[i].prof = lim_probab[i + 1].prob;
      if (!tcollect.empty()) {
        for (const double& t: tcollect) {
          probab[i].prof_d.durat += t;
        }
        probab[i].prof_d.durat /= tcollect.size();
        if (tcollect.size() > 1) {
          for (const double& t : tcollect) {
            const double d = t - probab[i].prof_d.durat;
            probab[i].prof_d.error += d * d;
          }
          probab[i].prof_d.error = sqrt(probab[i].prof_d.error / (tcollect.size() - 1));
        }
        probab[i].prof_d.count = tcollect.size();
      }
    }
    {
      vector<double> tcollect;
      count(loss_iter, losses.cend(), probab[i].rate, tcollect);
      probab[i].loss = los_probab[i + 1].prob;
    }
  }
  lambda_prof = fxlib::ApproxRateProbability(lim_probab);
  lambda_loss = fxlib::ApproxRateProbability(los_probab);
  return probab;
}

// tuple<double,double,double> => mean, confidence width, variance.
std::vector<std::string> PrepareOutputStrings(const size_t N, const std::tuple<double,double,double>& limit, const std::tuple<double,double,double>& loss) {
  using namespace std;
  vector<string> strs;
  ostringstream ostr;
  constexpr char first_str[] = "Sample size (N) = ";
  ostr << first_str << N;
  strs.push_back(ostr.str());
  ostr = ostringstream();
  for (auto v : {tuple_cat(make_tuple("Take Profit Limit"), limit), tuple_cat(make_tuple("Stop-Loss"), loss)}) {
    ostr << get<0>(v) << ":";
    strs.push_back(ostr.str());
    ostr = ostringstream();
    ostr << setw(strlen(first_str)) << setfill(' ') << right << "Mean = ";
    ostr << fixed << setprecision(1) << get<1>(v) << " [+/-" << setprecision(3) << get<2>(v) << " " << setprecision(10) << defaultfloat << (1 - g_alpha) << "]";
    strs.push_back(ostr.str());
    ostr = ostringstream();
    ostr << setw(strlen(first_str)) << setfill(' ') << "Variance = " << fixed << setprecision(1) << get<3>(v);
    strs.push_back(ostr.str());
    ostr = ostringstream();
  }
  return strs;
}

void QuickAnalyze(const variables_map& vm, const fxlib::fxsequence seq) {
  using namespace std;
  const string str_tm = vm["timeout"].as<string>();
  boost::regex rx_timeout("(\\d+)([mhdw])");
  boost::smatch what_tm;
  if (!boost::regex_match(str_tm, what_tm, rx_timeout)) {
    throw invalid_argument("Wrong timeout '" + str_tm + "'");
  }
  const int tm_val = stoi(what_tm[1]);
  const map<string, int> fxperiods = {{"m", static_cast<int>(fxlib::fxperiodicity::minutely)},
                                      {"h", static_cast<int>(fxlib::fxperiodicity::hourly)},
                                      {"d", static_cast<int>(fxlib::fxperiodicity::daily)},
                                      {"w", static_cast<int>(fxlib::fxperiodicity::weekly)}};
  const time_duration timeout = minutes(tm_val * fxperiods.at(what_tm[2]));
  const string positon = boost::algorithm::to_lower_copy(vm["position"].as<string>());
  double(*profit)(const fxlib::fxcandle& /*current*/, const fxlib::fxcandle& /*open*/);
  if (positon == "long") {
    profit = [](const fxlib::fxcandle& cc, const fxlib::fxcandle& co) { return fxlib::fxmean(cc) - fxlib::fxmean(co); };
  } else if (positon == "short") {
    profit = [](const fxlib::fxcandle& cc, const fxlib::fxcandle& co) { return fxlib::fxmean(co) - fxlib::fxmean(cc); };
  } else {
    throw invalid_argument("Wrong position '" + positon + "'");
  }
  cout << "Analyzing near " << seq.candles.size() << " " << positon << " positions with " << timeout << " timeout..." << endl;
  fxrate_samples max_limits;
  fxrate_samples max_losses;
  max_limits.reserve(seq.candles.size());
  max_losses.reserve(seq.candles.size());
  size_t curr_idx = 0;
  int progress = 1;
  size_t progress_idx = (progress * seq.candles.size()) / 10;
  for (auto piter = seq.candles.begin(); piter < seq.candles.end() && piter->time <= (seq.candles.back().time - timeout); ++piter, ++curr_idx) {
    if (curr_idx == progress_idx) {
      cout << piter->time << " processed " << (progress * 10) << "%" << endl;
      progress_idx = (++progress * seq.candles.size()) / 10;
    }
    const double po = profit(*piter, *piter);
    max_limits.push_back({po, 0});
    max_losses.push_back({-po, 0});
    for (auto citer = piter + 1; citer < seq.candles.end() && citer->time <= (piter->time + timeout); ++citer) {
      const double p = profit(*citer, *piter);
      if (max_limits.back().margin < p) {
        max_limits.back() = {p, (citer->time - piter->time).total_seconds() / 60.0};
      }
      if (max_losses.back().margin < -p) {
        max_losses.back() = {-p, (citer->time - piter->time).total_seconds() / 60.0};
      }
    }
    if (max_limits.back().margin < 0 || max_losses.back().margin < 0) {
      throw logic_error("Something has gone wrong!");
    }
  }  // for seq.candles
  if (max_limits.size() < 2 || max_losses.size() < 2 || max_limits.size() != max_losses.size()) {
    throw logic_error("No result");
  }
  double mean_limit = 0;
  double var_limit = 0;
  fxlib::RateStats(max_limits, mean_limit, var_limit);
  double mean_loss = 0;
  double var_loss = 0;
  fxlib::RateStats(max_losses, mean_loss, var_loss);
  const size_t N = max_limits.size();
  boost::math::students_t dist(static_cast<double>(N - 1));
  const double T = boost::math::quantile(boost::math::complement(dist, g_alpha / 2));
  const double w_limit = T * var_limit / sqrt(static_cast<double>(N));
  const double w_loss  = T * var_loss / sqrt(static_cast<double>(N));
  cout << "Done" << endl;
  cout << "----------------------------------" << endl;
  const auto out_strs = PrepareOutputStrings(N, make_tuple(mean_limit / g_pip, w_limit / g_pip, var_limit / g_pip), make_tuple(mean_loss / g_pip, w_loss / g_pip, var_loss / g_pip));
  for (const auto& s: out_strs) {
    cout << s << endl;
  }
  if (vm.count("out")) {
    cout << "----------------------------------" << endl;
    cout << "Preparing distributions..." << endl;
    const double vo = 0;
    const double dv = 6 * (max)(var_limit, var_loss) / g_distr_size;
    const auto lim_distrib = BuildDistribution(max_limits, vo, dv, "limits");
    const auto los_distrib = BuildDistribution(max_losses, vo, dv, "losses");
    if (lim_distrib.size() != los_distrib.size()) {
      throw logic_error("Size of limits distribution is not equal losses one!");
    }
    cout << "done" << endl;
    cout << "Preparing probabilities..." << endl;
    fxlib::fxprobab_coefs lambda_prof = {0};
    fxlib::fxprobab_coefs lambda_loss = {0};
    const auto probab = BuildProbability(max_limits, max_losses, vo, dv, lambda_prof, lambda_loss);
    cout << "done" << endl;
    boost::filesystem::path disp_file = g_outpath;
    disp_file.append(g_srcbin.filename().stem().string() + "-quick-" + positon + "-" + str_tm + ".gpl");
    cout << "Writing " << disp_file << "..." << endl;
    ofstream fout(disp_file.string());
    if (!fout) {
      throw ios_base::failure("Could not open '" + g_outpath.string() + "'");
    }
    fout << "# Profit limits and stop-losses for " << positon << " positon with " << str_tm << " timeout." << endl;
    for (const auto& s: out_strs) {
      fout << "# " << s << endl;
    }
    fout << "N=" << N << endl;
    fout << "lambda1_prof=" << lambda_prof.lambda1 * g_pip * g_pip << "  # " << 1.0 / (sqrt(abs(lambda_prof.lambda1)) * g_pip) << endl;
    fout << "lambda2_prof=" << lambda_prof.lambda2 * g_pip << "  # " << 1.0 / (lambda_prof.lambda2 * g_pip) << endl;
    fout << "Pprof(t)=exp(-(lambda1_prof*t**2 + lambda2_prof*t))" << endl;
    fout << "lambda1_loss=" << lambda_loss.lambda1 * g_pip * g_pip << "  # " << 1.0 / (sqrt(abs(lambda_loss.lambda1)) * g_pip) << endl;
    fout << "lambda2_loss=" << lambda_loss.lambda2 * g_pip << "  # " << 1.0 / (lambda_loss.lambda2 * g_pip) << endl;
    fout << "Ploss(t)=exp(-(lambda1_loss*t**2 + lambda2_loss*t))" << endl;
    fout << "# Distribution of maximum profit limits and stop-losses." << endl;
    fout << "$Distrib << EOD" << endl;
    for (size_t i = 0; i <= g_distr_size; i++) {
      if (lim_distrib[i + 1].bound != los_distrib[i + 1].bound) {
        throw logic_error("Something has gone wrong!");
      }
      fout << setw(3) << setfill('0') << i << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << lim_distrib[i + 1].bound / g_pip << " ";
      fout << setw(6) << setfill(' ') << lim_distrib[i + 1].count << " ";
      fout << setw(6) << setfill(' ') << los_distrib[i + 1].count << endl;
    }
    fout << "EOD" << endl;
    fout << "# Probability of maximum profit limits and stop-losses." << endl;
    fout << "$Probab << EOD" << endl;
    for (size_t i = 0; i < probab.size(); i++) {
      fout << setw(3) << setfill('0') << i << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << probab[i].rate / g_pip << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(4) << probab[i].prof << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(4) << probab[i].loss << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << probab[i].prof_d.durat << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << probab[i].prof_d.error << " ";
      fout << setw(6) << setfill(' ') << probab[i].prof_d.count << endl;
    }
    fout << "EOD" << endl;
    cout << "Done" << endl;
  }
}

int main(int argc, char* argv[]) {
  using namespace std;
  cout << "Forex Analyzer for distribution of limits and stop-losses" << endl;

  variables_map vm;
  if (!TryParseCommandLine(argc, argv, vm)) {
    return boost::system::errc::invalid_argument;
  }

  try {
    if (!vm.count("pip")) {
      throw invalid_argument("Unknown pip size for pair '" + g_srcbin.filename().stem().string() + "'");
    }
    cout << "Reading " << g_srcbin << "..." << endl;
    ifstream fbin(g_srcbin.string(), ifstream::binary);
    if (!fbin) {
      throw ios_base::failure("Could not open source file'" + g_srcbin.string() + "'");
    }
    const fxlib::fxsequence seq = fxlib::ReadSequence(fbin);
    if (!fbin) {
      throw ios_base::failure("Could not read source file'" + g_srcbin.string() + "'");
    }
    fbin.close();
    if (seq.periodicity != fxlib::fxperiodicity::minutely) {
      throw logic_error("Wrong sequence periodicity");
    }
    if (seq.period.is_null()) {
      throw logic_error("Wrong sequence period");
    }
    if (seq.candles.empty()) {
      throw logic_error("No data was found in sequence");
    }
    if (vm.count("quick")) {
      QuickAnalyze(vm, seq);
    }
  } catch (const system_error& e) {
    cout << "[ERROR] " << e.what() << endl;
    return e.code().value();
  } catch (const exception& e) {
    cout << "[ERROR] " << e.what() << endl;
    return boost::system::errc::operation_canceled;
  }

  return boost::system::errc::success;
}