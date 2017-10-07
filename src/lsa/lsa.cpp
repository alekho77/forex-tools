#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/math/distributions/students_t.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>

using boost::posix_time::time_duration;
using boost::posix_time::minutes;

namespace {
boost::filesystem::path g_srcbin;
double g_pip = 0.0001;
double g_alpha = 0.1;
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
    ("out,o", value<string>()->value_name("[path]")->implicit_value(""), "Optionally distributions and probabilities can be written into output files.");
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
  double(*profit)(const fxlib::fxcandle&, const fxlib::fxcandle&);
  if (positon == "long") {
    profit = [](const fxlib::fxcandle& cc, const fxlib::fxcandle& co) { return cc.low - co.high; };
  } else if (positon == "short") {
    profit = [](const fxlib::fxcandle& cc, const fxlib::fxcandle& co) { return co.low - cc.high; };
  } else {
    throw invalid_argument("Wrong position '" + positon + "'");
  }
  cout << "Analyzing near " << seq.candles.size() << " " << positon << " positions with " << timeout << " timeout..." << endl;
  vector<double> max_limits;
  vector<double> max_losses;
  max_limits.reserve(seq.candles.size());
  max_losses.reserve(seq.candles.size());
  size_t curr_idx = 0;
  int progress = 1;
  size_t progress_idx = (progress * seq.candles.size()) / 10;
  double mean_limit = 0;
  double mean_loss  = 0;
  for (auto piter = seq.candles.begin(); piter < seq.candles.end() && piter->time <= (seq.candles.back().time - timeout); ++piter, ++curr_idx) {
    if (curr_idx == progress_idx) {
      cout << piter->time << " processed " << (progress * 10) << "%" << endl;
      progress_idx = (++progress * seq.candles.size()) / 10;
    }
    max_limits.push_back(profit(*piter, *piter));
    max_losses.push_back(-profit(*piter, *piter));
    for (auto citer = piter + 1; citer < seq.candles.end() && citer->time <= (piter->time + timeout); ++citer) {
      max_limits.back() = (max)(max_limits.back(), profit(*citer, *piter));
      max_losses.back() = (max)(max_losses.back(), -profit(*citer, *piter));
    }
    mean_limit += max_limits.back();
    mean_loss  += max_losses.back();
  }  // for seq.candles
  if (max_limits.size() < 2 || max_losses.size() < 2 || max_limits.size() != max_losses.size()) {
    throw logic_error("No result");
  }
  const size_t N = max_limits.size();
  mean_limit /= N;
  mean_loss  /= N;
  double var_limit = 0;
  double var_loss  = 0;
  for (size_t i = 0; i < max_limits.size(); i++) {
    const double dl = max_limits[i] - mean_limit;
    const double ds = max_losses[i] - mean_loss;
    var_limit += dl * dl;
    var_loss  += ds * ds;
  }
  mean_limit /= g_pip;
  mean_loss  /= g_pip;
  var_limit = sqrt(var_limit / static_cast<double>(N - 1)) / g_pip;
  var_loss  = sqrt(var_loss / static_cast<double>(N - 1)) / g_pip;
  boost::math::students_t dist(static_cast<double>(N - 1));
  const double T = boost::math::quantile(boost::math::complement(dist, g_alpha / 2));
  const double w_limit = T * var_limit / sqrt(static_cast<double>(N));
  const double w_loss  = T * var_loss / sqrt(static_cast<double>(N));
  cout << "Done" << endl;
  cout << "----------------------------------" << endl;
  cout << "Sample size (N) = " << N << endl << endl;
  cout << "Limit mean      = " << fixed << setprecision(1) << mean_limit << " [+/-" << setprecision(3) << w_limit << " " << setprecision(10) << defaultfloat << (1 - g_alpha) << "]" << endl;
  cout << "Limit variance  = " << fixed << setprecision(1) << var_limit << endl << endl;
  cout << "Loss mean       = " << fixed << setprecision(1) << mean_loss << " [+/-" << setprecision(3) << w_loss << " " << setprecision(10) << defaultfloat << (1 - g_alpha) << "]" << endl;
  cout << "Loss variance   = " << fixed << setprecision(1) << var_loss << endl;
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