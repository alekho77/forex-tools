#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"

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

using boost::posix_time::time_duration;
using boost::posix_time::minutes;

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

// tuple<double,double> => mean, variance
using simple_distribution = std::vector<std::tuple<double, int, int>>;
simple_distribution BuildDistribution(std::vector<double>& limits, std::vector<double>& losses, const std::tuple<double,double>& limit, const std::tuple<double,double>& loss) {
  using namespace std;
  sort(limits.begin(), limits.end());
  sort(losses.begin(), losses.end());
  const double vo = (min)(get<0>(limit) - get<1>(limit), get<0>(loss) - get<1>(loss));
  const double dv = 3 * (max)(get<1>(limit), get<1>(loss)) / 50;
  simple_distribution distrib(g_distr_size + 1, make_tuple(0.0,0,0));
  auto limit_iter = limits.cbegin();
  auto loss_iter = losses.cbegin();
  using citer = std::vector<double>::const_iterator;
  auto count = [](citer& iter, const citer& end, const double bound, int& counter) {
    while ((iter < end) && (*iter <= bound)) {
      ++counter;
      ++iter;
    }
  };
  const double bottom_bound = vo - dv;
  int limits_data_before = 0;
  count(limit_iter, limits.cend(), bottom_bound, limits_data_before);
  if (limit_iter > limits.cbegin()) {
    cout << "[NOTE] There are " << limits_data_before << " extra data in limits before " << fixed << setprecision(3) << (bottom_bound / g_pip) << " value" << endl;
  }
  int losses_data_before = 0;
  count(loss_iter, losses.cend(), bottom_bound, losses_data_before);
  if (loss_iter > losses.cbegin()) {
    cout << "[NOTE] There are " << losses_data_before << " extra data in losses before " << fixed << setprecision(3) << (bottom_bound / g_pip) << " value" << endl;
  }
  for (size_t i = 0; i < distrib.size(); i++) {
    get<0>(distrib[i]) = vo + i * dv;
    count(limit_iter, limits.cend(), get<0>(distrib[i]), get<1>(distrib[i]));
    count(loss_iter, losses.end(), get<0>(distrib[i]), get<2>(distrib[i]));
  }
  if (limit_iter < limits.cend()) {
    cout << "[NOTE] There are " << (limits.cend() - limit_iter) << " extra data in limits beyond " << fixed << setprecision(3) << (distrib.size() * dv / g_pip) << " value" << endl;
  }
  if (loss_iter < losses.cend()) {
    cout << "[NOTE] There are " << (losses.cend() - loss_iter) << " extra data in losses beyond " << fixed << setprecision(3) << (distrib.size() * dv / g_pip) << " value" << endl;
  }
  return distrib;
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
  var_limit = sqrt(var_limit / static_cast<double>(N - 1));
  var_loss  = sqrt(var_loss / static_cast<double>(N - 1));
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
    const auto distrib = BuildDistribution(max_limits, max_losses, make_tuple(mean_limit, var_limit), make_tuple(mean_loss, var_loss));
    cout << "done" << endl;
    boost::filesystem::path disp_file = g_outpath;
    disp_file.append(g_srcbin.filename().stem().string() + "-disp-" + positon + "-" + str_tm + ".gpl");
    cout << "Writing " << disp_file << "..." << endl;
    ofstream fout(disp_file.string());
    if (!fout) {
      throw ios_base::failure("Could not open '" + g_outpath.string() + "'");
    }
    fout << "# Distribution of maximum profit limits and stop-losses for " << positon << " positon with " << str_tm << " timeout." << endl;
    for (const auto& s: out_strs) {
      fout << "# " << s << endl;
    }
    fout << "N=" << N << endl;
    fout << "$Distrib << EOD" << endl;
    for (size_t i = 0; i < distrib.size(); i++) {
      fout << setw(3) << setfill('0') << i << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << get<0>(distrib[i]) / g_pip << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << get<1>(distrib[i]) << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << get<2>(distrib[i]) << endl;
    }
    fout << "EOD" << endl;
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