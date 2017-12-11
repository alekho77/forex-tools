#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"
#include "fxlib/helpers/string_conversion.h"

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/math/distributions/students_t.hpp>
#include <boost/optional.hpp>

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

using fxlib::fxmargin_samples;
using fxlib::fxmargin_distribution;
using fxlib::fxmargin_probability;
using fxlib::fxprobab_coefs;
using fxlib::fxdurat_coefs;

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

fxmargin_distribution BuildDistribution(const fxmargin_samples& samples, const double from, const double step, const std::string& name) {
  using namespace std;
  fxmargin_distribution distrib = fxlib::MarginDistribution(samples, g_distr_size, from, step);
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

fxmargin_probability BuildProbability(const fxmargin_samples& samples, const double from, const double step, const std::string& name) {
  using namespace std;
  auto probab = fxlib::MarginProbability(samples, g_distr_size, from, step);
  if (probab.size() != g_distr_size + 3) {
    throw logic_error("Invalid size of " + name + " probability!");
  }
  if (probab.front().count != samples.size()) {
    cout << "[ERROR] There are extra data in " + name + " before " << fixed << setprecision(3) << (probab.front().bound / g_pip) << endl;
    throw logic_error("Something has gone wrong!");
  }
  if (probab.back().count > 0) {
    cout << "[NOTE] There are " << probab.back().count << " extra data in " + name + " beyond " << fixed << setprecision(3) << (probab.back().bound / g_pip) << " value" << endl;
  }
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
  const time_duration timeout = fxlib::conversion::duration_from_string(vm["timeout"].as<string>());
  const string positon = boost::algorithm::to_lower_copy(vm["position"].as<string>());
  fxlib::fprofit_t profit;
  if (positon == "long") {
    profit = fxlib::fxprofit_long;
  } else if (positon == "short") {
    profit = fxlib::fxprofit_short;
  } else {
    throw invalid_argument("Wrong position '" + positon + "'");
  }
  cout << "Analyzing near " << seq.candles.size() << " " << positon << " positions with " << timeout << " timeout..." << endl;
  fxmargin_samples limits;
  fxmargin_samples losses;
  limits.reserve(seq.candles.size());
  losses.reserve(seq.candles.size());
  boost::optional<boost::posix_time::ptime> prev_time;
  double min_adjust = 0;
  size_t curr_idx = 0;
  int progress = 1;
  size_t progress_idx = (progress * seq.candles.size()) / 10;
  for (auto piter = seq.candles.begin(); piter < seq.candles.end() && piter->time <= (seq.candles.back().time - timeout); ++piter, ++curr_idx) {
    if (curr_idx == progress_idx) {
      cout << piter->time << " processed " << (progress * 10) << "%" << endl;
      progress_idx = (++progress * seq.candles.size()) / 10;
    }
    if (prev_time.is_initialized()) {
      const time_duration dt = piter->time - *prev_time;
      min_adjust += dt.total_seconds() / 60.0;
    }
    prev_time = piter->time;
    const double po = profit(*piter, *piter);
    limits.push_back({po, 0});
    losses.push_back({-po, 0});
    for (auto citer = piter + 1; citer < seq.candles.end() && citer->time <= (piter->time + timeout); ++citer) {
      const double p = profit(*citer, *piter);
      if (limits.back().margin < p) {
        limits.back() = {p, (citer->time - piter->time).total_seconds() / 60.0};
      }
      if (losses.back().margin < -p) {
        losses.back() = {-p, (citer->time - piter->time).total_seconds() / 60.0};
      }
    }
    if (limits.back().margin < 0 || losses.back().margin < 0) {
      throw logic_error("Something has gone wrong!");
    }
  }  // for seq.candles
  if (limits.size() < 2 || losses.size() < 2 || limits.size() != losses.size()) {
    throw logic_error("No result");
  }
  double lim_mean = 0;
  double lim_var = 0;
  fxlib::MarginStats(fxlib::fxsort(limits), lim_mean, lim_var);
  double los_mean = 0;
  double los_var = 0;
  fxlib::MarginStats(fxlib::fxsort(losses), los_mean, los_var);
  const size_t N = limits.size();
  min_adjust /= (N - 1);
  boost::math::students_t dist(static_cast<double>(N - 1));
  const double T = boost::math::quantile(boost::math::complement(dist, g_alpha / 2));
  const double lim_w = T * lim_var / sqrt(static_cast<double>(N));
  const double los_w  = T * los_var / sqrt(static_cast<double>(N));
  cout << "Done" << endl;
  cout << "----------------------------------" << endl;
  const auto out_strs = PrepareOutputStrings(N, make_tuple(lim_mean / g_pip, lim_w / g_pip, lim_var / g_pip), make_tuple(los_mean / g_pip, los_w / g_pip, los_var / g_pip));
  for (const auto& s: out_strs) {
    cout << s << endl;
  }
  if (vm.count("out")) {
    cout << "----------------------------------" << endl;

    cout << "Preparing distributions..." << endl;
    const double mo = 0;
    const double dm = 6 * (max)(lim_var, los_var) / g_distr_size;
    const auto lim_distrib = BuildDistribution(limits, mo, dm, "limits");
    const auto los_distrib = BuildDistribution(losses, mo, dm, "losses");
    if (lim_distrib.size() != los_distrib.size()) {
      throw logic_error("Size of limits distribution is not equal losses one!");
    }
    cout << "done" << endl;
    cout << "Preparing probabilities..." << endl;
    const auto lim_probab = BuildProbability(limits, mo, dm, "limits");
    const fxprobab_coefs lim_pcoefs = fxlib::ApproxMarginProbability(lim_probab);
    const auto lim_durats = fxlib::MarginDurationDistribution(limits, g_distr_size, mo, dm);
    const fxdurat_coefs lim_dcoefs = fxlib::ApproxDurationDistribution(lim_durats);
    if (lim_durats.size() != lim_probab.size()) {
      throw logic_error("Size of limits probability is not equal duration distribution one!");
    }
    const auto los_probab = BuildProbability(losses, mo, dm, "losses");
    const fxprobab_coefs los_pcoefs = fxlib::ApproxMarginProbability(los_probab);
    const auto los_durats = fxlib::MarginDurationDistribution(losses, g_distr_size, mo, dm);
    const fxdurat_coefs los_dcoefs = fxlib::ApproxDurationDistribution(los_durats);
    if (los_durats.size() != los_probab.size()) {
      throw logic_error("Size of losses probability is not equal duration distribution one!");
    }
    if (lim_probab.size() != los_probab.size()) {
      throw logic_error("Size of limits probability is not equal losses one!");
    }
    const double lim_max_m = fxlib::MaxMargin(lim_pcoefs, lim_dcoefs, min_adjust);
    const double lim_max_probab = fxlib::margin_probab(lim_pcoefs, lim_max_m);
    const double lim_max_durat = fxlib::margin_duration(lim_dcoefs, lim_max_m);
    const double lim_max_yield = fxlib::margin_yield(lim_pcoefs, lim_dcoefs, min_adjust, lim_max_m);
    cout << "done" << endl;

    boost::filesystem::path disp_file = g_outpath;
    disp_file.append(g_srcbin.filename().stem().string() + "-quick-" + positon + "-" + vm["timeout"].as<string>() + ".gpl");
    cout << "Writing " << disp_file << "..." << endl;
    ofstream fout(disp_file.string());
    if (!fout) {
      throw ios_base::failure("Could not open '" + g_outpath.string() + "'");
    }
    fout << "# Profit limits and stop-losses for " << positon << " positon with " << vm["timeout"].as<string>() << " timeout." << endl;
    for (const auto& s: out_strs) {
      fout << "# " << s << endl;
    }
    fout << "N=" << N << endl;
    fout << fixed << setprecision(6) << "adjustemnt_coef=" << min_adjust << endl;
    fout << defaultfloat << setprecision(6) << "prof_plam2=" << lim_pcoefs.lambda2 * g_pip * g_pip << "  # " << 1.0 / (sqrt(abs(lim_pcoefs.lambda2)) * g_pip) << endl;
    fout << defaultfloat << setprecision(6) << "prof_plam1=" << lim_pcoefs.lambda1 * g_pip << "  # " << 1.0 / (lim_pcoefs.lambda1 * g_pip) << endl;
    fout << "Pprof(t)=exp(-(prof_plam2*t**2 + prof_plam1*t))" << endl;
    fout << defaultfloat << setprecision(6) << "loss_plam2=" << los_pcoefs.lambda2 * g_pip * g_pip << "  # " << 1.0 / (sqrt(abs(los_pcoefs.lambda2)) * g_pip) << endl;
    fout << defaultfloat << setprecision(6) << "loss_plam1=" << los_pcoefs.lambda1 * g_pip << "  # " << 1.0 / (los_pcoefs.lambda1 * g_pip) << endl;
    fout << "Ploss(t)=exp(-(loss_plam2*t**2 + loss_plam1*t))" << endl;
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
    fout << defaultfloat << setprecision(6) << "prof_dT=" << lim_dcoefs.T << endl;
    fout << defaultfloat << setprecision(6) << "prof_dlam=" << lim_dcoefs.lambda * g_pip << endl;
    fout << "Dprof(t)=prof_dT*(1-exp(-(prof_dlam*t)))" << endl;
    fout << defaultfloat << setprecision(6) << "prof_m_max=" << lim_max_m / g_pip << endl;
    fout << defaultfloat << setprecision(6) << "prof_P_max=" << lim_max_probab << endl;
    fout << defaultfloat << setprecision(6) << "prof_W_max=" << min_adjust / lim_max_probab << endl;
    fout << defaultfloat << setprecision(6) << "prof_D_max=" << lim_max_durat << endl;
    fout << defaultfloat << setprecision(6) << "prof_T_max=" << min_adjust / lim_max_probab + lim_max_durat << endl;
    fout << defaultfloat << setprecision(6) << "prof_max=" << lim_max_yield / g_pip << endl;
    fout << defaultfloat << setprecision(6) << "loss_dT=" << los_dcoefs.T << endl;
    fout << defaultfloat << setprecision(6) << "loss_dlam=" << los_dcoefs.lambda * g_pip << endl;
    fout << "Dloss(t)=loss_dT*(1-exp(-(loss_dlam*t)))" << endl;
    fout << "$Probab << EOD" << endl;
    for (size_t i = 0; i <= g_distr_size; i++) {
      if (lim_probab[i + 1].bound != los_probab[i + 1].bound) {
        throw logic_error("Something has gone wrong!");
      }
      if (lim_probab[i + 1].bound != lim_durats[i + 1].bound) {
        throw logic_error("Something has gone wrong!");
      }
      if (lim_probab[i + 1].bound != los_durats[i + 1].bound) {
        throw logic_error("Something has gone wrong!");
      }
      fout << setw(3) << setfill('0') << i << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << lim_probab[i + 1].bound / g_pip << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(4) << lim_probab[i + 1].prob << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(4) << los_probab[i + 1].prob << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << lim_durats[i + 1].durat << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << lim_durats[i + 1].error << " ";
      fout << setw(6) << setfill(' ') << lim_durats[i + 1].count << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << los_durats[i + 1].durat << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << los_durats[i + 1].error << " ";
      fout << setw(6) << setfill(' ') << los_durats[i + 1].count << endl;
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
    if (seq.periodicity != minutes(1)) {
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