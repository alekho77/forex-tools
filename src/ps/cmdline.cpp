#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

boost::filesystem::path g_srcbin;
boost::filesystem::path g_outtxt;
boost::filesystem::path g_config;
double g_pip = 0.0001;
std::string g_algname;
bool g_quick_mode = false;
bool g_full_mode = false;
bool g_search_mode = false;
double g_threshold = 0;
double g_take_profit = 0;
double g_stop_loss = 0;
std::tuple<int, int> g_take_profit_range = { 0, 0 };
std::tuple<int, int> g_stop_loss_range = { 0, 0 };
std::tuple<double, double> g_threshold_range = { 0, 0 };
double g_momentum = 0.3;

std::tuple<int, int> irange_from_string(const std::string& str) {
  const boost::regex fmask("^(\\d+)-(\\d+)$");
  boost::smatch what;
  if (boost::regex_match(str, what, fmask)) {
    return std::make_tuple(std::stoi(what[1]), std::stoi(what[2]));
  }
  return{ 0 , 0 };
}

std::tuple<double, double> drange_from_string(const std::string& str) {
  const boost::regex fmask("^(\\d+\\.\\d*)-(\\d+\\.\\d*)$");
  boost::smatch what;
  if (boost::regex_match(str, what, fmask)) {
    return std::make_tuple(std::stod(what[1]), std::stod(what[2]));
  }
  return{ 0 , 0 };
}

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm) {
  using namespace std;
  bool help = false;
  options_description basic_desc("Basic options", 200);
  basic_desc.add_options()
    ("help,h", bool_switch(&help), "Show help.")
    ("quick,q", bool_switch(&g_quick_mode), "Quick play.")
    ("full,f", bool_switch(&g_full_mode), "Complex play.")
    ("search,r", bool_switch(&g_search_mode), "Search the best parameters for play.");
  options_description generic_desc("Generic options", 200);
  generic_desc.add_options()
    ("name,n", value<string>(&g_algname)->required()->value_name("name"), "Name of algorithm for play.")
    ("config,c", value<string>()->required()->value_name("config")->notifier(
      [](const string& cfgname) { g_config = boost::filesystem::canonical(cfgname); }), "Path to algorithm configuration file.")
    ("source,s", value<string>()->required()->value_name("bin")->notifier(
      [](const string& srcname) { g_srcbin = boost::filesystem::canonical(srcname); }), "Path to source binary quotes.");
  options_description quick_desc("Quick play options", 200);
  quick_desc.add_options()
    ("profit,p", value<double>(&g_take_profit)->required()->value_name("pip"), "Limit order for taking profit in pips.")
    ("loss,l", value<double>(&g_stop_loss)->required()->value_name("pip"), "Stop-loss order to limit losses in pips.")
    ("threshold,t", value<double>(&g_threshold)->required()->value_name("[0..1]"), "Threshold for making forecast.")
    ("out,o", value<string>()->value_name("[filename]")->implicit_value("")->notifier(
      [](const string& outname) { g_outtxt = boost::filesystem::canonical(outname); }), "Text file to write log.");
  options_description full_desc("Complex play options", 200);
  full_desc.add_options()
    ("profit,p", value<string>()->required()->value_name("a-b")->notifier(
      [](const string& str) { g_take_profit_range = irange_from_string(str); }), "Limit order Range for taking profit in pips.")
    ("loss,l", value<string>()->required()->value_name("a-b")->notifier(
      [](const string& str) { g_stop_loss_range = irange_from_string(str); }), "Stop-loss order Range to limit losses in pips.")
    ("threshold,t", value<double>(&g_threshold)->required()->value_name("[0..1]"), "Threshold for making forecast.")
    ("out,o", value<string>()->required()->value_name("filename")->implicit_value("")->notifier(
      [](const string& outname) { g_outtxt = boost::filesystem::canonical(outname); }), "File to write result (gnu-plot format).");
  options_description search_desc("Search play params options", 200);
  search_desc.add_options()
    ("profit,p", value<string>()->required()->value_name("a-b")->notifier(
      [](const string& str) { g_take_profit_range = irange_from_string(str); }), "Limit order Range for taking profit in pips.")
    ("loss,l", value<string>()->required()->value_name("a-b")->notifier(
      [](const string& str) { g_stop_loss_range = irange_from_string(str); }), "Stop-loss order Range to limit losses in pips.")
    ("threshold,t", value<string>()->required()->value_name("x.x-y.y")->notifier (
      [](const string& str) { g_threshold_range = drange_from_string(str); }), "Threshold for making forecast [0..1].")
    ("momentum,m", value<double>(&g_momentum)->default_value(0.3), "Momentum of gradient searching.");
    options_description additional_desc("Additional options", 200);
  additional_desc.add_options()
    ("pip,z", value<double>(&g_pip)->value_name("size"), "Pip size, usually 0.0001 or 0.01.");
  const auto list_desc = {basic_desc, generic_desc, quick_desc, full_desc, search_desc, additional_desc};
  try {
    store(command_line_parser(argc, argv).options(basic_desc).allow_unregistered().run(), vm);
    notify(vm);
    if (help) {
      cout << list_desc;
      return false;
    } else if (g_quick_mode) {
      options_description desc;
      store(parse_command_line(argc, argv, desc.add(basic_desc).add(generic_desc).add(quick_desc).add(additional_desc)), vm);
      notify(vm);
    } else if (g_full_mode) {
      options_description desc;
      store(parse_command_line(argc, argv, desc.add(basic_desc).add(generic_desc).add(full_desc).add(additional_desc)), vm);
      notify(vm);
    } else if (g_search_mode) {
      options_description desc;
      store(parse_command_line(argc, argv, desc.add(basic_desc).add(generic_desc).add(search_desc).add(additional_desc)), vm);
      notify(vm);
    } else {
      throw invalid_argument("The mode should be defined");
    }
  } catch (const exception& e) {
    cout << "[ERROR] Command line: " << e.what() << endl;
    cout << list_desc;
    return false;
  }
  return true;
}
