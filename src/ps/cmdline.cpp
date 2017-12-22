#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"
#include <boost/filesystem.hpp>

boost::filesystem::path g_srcbin;
boost::filesystem::path g_outtxt;
boost::filesystem::path g_config;
double g_pip = 0.0001;
std::string g_algname;
bool g_quick_mode = false;
double g_threshold = 0;
double g_take_profit = 0;
double g_stop_loss = 0;

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm) {
  using namespace std;
  bool help = false;
  options_description basic_desc("Basic options", 200);
  basic_desc.add_options()
    ("help,h", bool_switch(&help), "Show help.")
    ("quick,q", bool_switch(&g_quick_mode), "Quick play.");
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
    ("threshold,t", value<double>(&g_threshold)->required()->value_name("[0..1]"), "Threshold for making forecast.");
  options_description additional_desc("Additional options", 200);
  additional_desc.add_options()
    ("pip,z", value<double>(&g_pip)->value_name("size"), "Pip size, usually 0.0001 or 0.01.")
    ("out,o", value<string>()->value_name("[filename]")->implicit_value("")->notifier(
      [](const string& outname) { g_outtxt = boost::filesystem::canonical(outname); }), "Text file to write log.");
  const auto list_desc = {basic_desc, generic_desc, quick_desc, additional_desc};
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
