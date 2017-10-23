#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"
#include "math/mathlib/lsyseq.h"

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
using simple_distribution = std::vector<std::tuple<double /*rate*/, int /*p limit*/, int /*p loss*/>>;
simple_distribution BuildDistribution(const std::vector<double>& limits, const std::vector<double>& losses,
                                      const std::tuple<double,double>& limit, const std::tuple<double,double>& loss) {
  using namespace std;
  const double vo = 0;
  const double dv = 6 * (max)(get<1>(limit), get<1>(loss)) / g_distr_size;
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
    cout << "[ERROR] There are " << limits_data_before << " extra data in limits before " << fixed << setprecision(3) << (bottom_bound / g_pip) << " value" << endl;
    throw logic_error("Something has gone wrong!");
  }
  int losses_data_before = 0;
  count(loss_iter, losses.cend(), bottom_bound, losses_data_before);
  if (loss_iter > losses.cbegin()) {
    cout << "[ERROR] There are " << losses_data_before << " extra data in losses before " << fixed << setprecision(3) << (bottom_bound / g_pip) << " value" << endl;
    throw logic_error("Something has gone wrong!");
  }
  for (size_t i = 0; i < distrib.size(); i++) {
    get<0>(distrib[i]) = vo + i * dv;
    count(limit_iter, limits.cend(), get<0>(distrib[i]), get<1>(distrib[i]));
    count(loss_iter, losses.end(), get<0>(distrib[i]), get<2>(distrib[i]));
  }
  if (limit_iter < limits.cend()) {
    cout << "[NOTE] There are " << (limits.cend() - limit_iter) << " extra data in limits beyond " << fixed << setprecision(3) << (distrib.size() * dv / g_pip) << " value" << endl;
  }
  if ((limits_data_before 
       + accumulate(distrib.cbegin(), distrib.cend(), 0, [](auto a, const auto& b) { return a + get<1>(b); }) 
       + (limits.cend() - limit_iter)) != static_cast<int>(limits.size())) {
    throw logic_error("Sum of limits distribution is not equal the total limits!");
  }
  if (loss_iter < losses.cend()) {
    cout << "[NOTE] There are " << (losses.cend() - loss_iter) << " extra data in losses beyond " << fixed << setprecision(3) << (distrib.size() * dv / g_pip) << " value" << endl;
  }
  if ((losses_data_before 
       + accumulate(distrib.cbegin(), distrib.cend(), 0, [](auto a, const auto& b) { return a + get<2>(b); }) 
       + (losses.cend() - loss_iter)) != static_cast<int>(losses.size())) {
    throw logic_error("Sum of losses distribution is not equal the total losses!");
  }
  return distrib;
}

using simple_probability = std::vector<std::tuple<double /*rate*/, double /*P profit*/, double /*P loss*/>>;
simple_probability BuildProbability(const std::vector<double>& limits, const std::vector<double>& losses,
                                    const std::tuple<double,double>& limit, const std::tuple<double,double>& loss,
                                    std::tuple<double,double>& lambda_prof, std::tuple<double, double>& lambda_loss) {
  using namespace std;
  const double vo = 0;
  const double dv = 6 * (max)(get<1>(limit), get<1>(loss)) / g_distr_size;
  simple_probability probab(g_distr_size + 1);
  int limits_count = static_cast<int>(limits.size());
  int losses_count = static_cast<int>(losses.size());
  auto limit_iter = limits.cbegin();
  auto loss_iter = losses.cbegin();
  using citer = std::vector<double>::const_iterator;
  auto count = [](citer& iter, const citer& end, const double bound, int& counter) {
    while ((iter <= end) && (*iter < bound)) {
      --counter;
      ++iter;
    }
  };
  const double bottom_bound = vo - dv;
  count(limit_iter, limits.cend(), bottom_bound, limits_count);
  count(loss_iter, losses.cend(), bottom_bound, losses_count);
  if (limit_iter > limits.cbegin()) {
    cout << "[ERROR] There are extra data in limits before " << fixed << setprecision(3) << (bottom_bound / g_pip) << endl;
    throw logic_error("Something has gone wrong!");
  }
  if (loss_iter > losses.cbegin()) {
    cout << "[NOTE] There are extra data in losses before " << fixed << setprecision(3) << (bottom_bound / g_pip) << endl;
    throw logic_error("Something has gone wrong!");
  }
  const size_t good_interval = g_distr_size / 3 + 1;
  mathlib::matrix<double> Ap{good_interval, 2};
  mathlib::matrix<double> Bp{good_interval};
  mathlib::matrix<double> Al{good_interval, 2};
  mathlib::matrix<double> Bl{good_interval};
  for (size_t i = 0; i < probab.size(); i++) {
    get<0>(probab[i]) = vo + i * dv;
    count(limit_iter, limits.cend(), get<0>(probab[i]), limits_count);
    get<1>(probab[i]) = double(limits_count) / double(limits.size());
    count(loss_iter, losses.end(), get<0>(probab[i]), losses_count);
    get<2>(probab[i]) = double(losses_count) / double(losses.size());
    if (i < good_interval) {
      const double t = get<0>(probab[i]);
      Al[i][0] = Ap[i][0] = t * t;
      Al[i][1] = Ap[i][1] = t;
      Bp[i][0] = - std::log(get<1>(probab[i]));
      Bl[i][0] = - std::log(get<2>(probab[i]));
    }
  }
  if (limit_iter < limits.cend()) {
    cout << "[NOTE] There are " << (limits.cend() - limit_iter) << " extra data in limits beyond " << fixed << setprecision(3) << (probab.size() * dv / g_pip) << " value" << endl;
  }
  if (limits_count != static_cast<int>(limits.cend() - limit_iter)) {
    throw logic_error("Sum of limits is not equal the total limits!");
  }
  if (loss_iter < losses.cend()) {
    cout << "[NOTE] There are " << (losses.cend() - loss_iter) << " extra data in losses beyond " << fixed << setprecision(3) << (probab.size() * dv / g_pip) << " value" << endl;
  }
  if (losses_count != static_cast<int>(losses.cend() - loss_iter)) {
    throw logic_error("Sum of losses is not equal the total losses!");
  }
  {
    const mathlib::matrix<double> ApT = mathlib::transpose(Ap);
    mathlib::linear_equations<double> syseq{ApT * Ap, ApT * Bp};
    const auto& Xp = syseq.normalize().solve();
    lambda_prof = {Xp[0][0], Xp[1][0]};
  }
  {
    const mathlib::matrix<double> AlT = mathlib::transpose(Al);
    mathlib::linear_equations<double> syseq{AlT * Al, AlT * Bl};
    const auto& Xl = syseq.normalize().solve();
    lambda_loss = {Xl[0][0], Xl[1][0]};
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
    if (max_limits.back() < 0 || max_losses.back() < 0) {
      throw logic_error("Something has gone wrong!");
    }
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
    sort(max_limits.begin(), max_limits.end());
    sort(max_losses.begin(), max_losses.end());
    const auto distrib = BuildDistribution(max_limits, max_losses, make_tuple(mean_limit, var_limit), make_tuple(mean_loss, var_loss));
    cout << "done" << endl;
    cout << "Preparing probabilities..." << endl;
    auto lambda_prof = make_tuple(0.0, 0.0);
    auto lambda_loss = make_tuple(0.0, 0.0);
    const auto probab = BuildProbability(max_limits, max_losses, make_tuple(mean_limit, var_limit), make_tuple(mean_loss, var_loss), lambda_prof, lambda_loss);
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
    fout << "lambda1_prof=" << get<0>(lambda_prof) * g_pip * g_pip << endl;
    fout << "lambda2_prof=" << get<1>(lambda_prof) * g_pip << endl;
    fout << "Pprof(t)=exp(-(lambda1_prof*t*t + lambda2_prof*t))" << endl;
    fout << "lambda1_loss=" << get<0>(lambda_loss) * g_pip * g_pip << endl;
    fout << "lambda2_loss=" << get<1>(lambda_loss) * g_pip << endl;
    fout << "Ploss(t)=exp(-(lambda1_loss*t*t + lambda2_loss*t))" << endl;
    fout << "# Distribution of maximum profit limits and stop-losses." << endl;
    fout << "$Distrib << EOD" << endl;
    for (size_t i = 0; i < distrib.size(); i++) {
      fout << setw(3) << setfill('0') << i << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << get<0>(distrib[i]) / g_pip << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << get<1>(distrib[i]) << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << get<2>(distrib[i]) << endl;
    }
    fout << "EOD" << endl;
    fout << "# Probability of maximum profit limits and stop-losses." << endl;
    fout << "$Probab << EOD" << endl;
    for (size_t i = 0; i < probab.size(); i++) {
      fout << setw(3) << setfill('0') << i << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(1) << get<0>(probab[i]) / g_pip << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(4) << get<1>(probab[i]) << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(4) << get<2>(probab[i]) << endl;
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