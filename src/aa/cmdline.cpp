#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"
#include <boost/filesystem.hpp>

boost::filesystem::path g_srcbin;
boost::filesystem::path g_config;
double g_pip = 0.0001;
std::string g_algname;
bool g_analyze_mode = false;
bool g_learn_mode = false;
size_t g_distr_size = 100;

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm) {
  using namespace std;
  options_description basic_desc("Basic options", 200);
  basic_desc.add_options()
    ("help,h", "Show help");
  options_description generic_desc("Generic options", 200);
  generic_desc.add_options()
    ("analyze,a", bool_switch(&g_analyze_mode), "Analyze the algorithm.")
    ("learning,l", bool_switch(&g_learn_mode), "Learn the algorithm.")
    ("source,s", value<string>()->required()->value_name("pair-bin")->notifier(
      [](const string& srcname) { g_srcbin = boost::filesystem::canonical(srcname); }), "Path to compiled (binary) quotes.")
    ("name,n", value<string>(&g_algname)->required()->value_name("name"), "Name of algorithm to analyze or learn.")
    ("config,c", value<string>()->required()->value_name("config")->notifier(
      [](const string& cfgname) { g_config = boost::filesystem::canonical(cfgname); }), "Path to algorithm configuration file.");
  options_description additional_desc("Additional options", 200);
  additional_desc.add_options()
    ("pip,z", value<double>(&g_pip)->value_name("size"), "Pip size, usually 0.0001 or 0.01.")
    ("distsize,d", value<size_t>(&g_distr_size)->default_value(100)->value_name("size"), "Number of intervals to build a distribution.");
  const auto list_desc = {basic_desc, generic_desc, additional_desc};
  try {
    options_description desc;
    desc.add(basic_desc).add(generic_desc).add(additional_desc);
    store(command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    notify(vm);
    if (vm.count("help")) {
      cout << list_desc;
      return false;
    }
    if (!g_analyze_mode && !g_learn_mode) {
      throw invalid_argument("The mode should be defined (analyzing or learning)");
    }
  } catch (const exception& e) {
    cout << "[ERROR] Command line: " << e.what() << endl;
    cout << list_desc;
    return false;
  }
  return true;
}
