#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"
#include <boost/filesystem.hpp>

boost::filesystem::path g_srcbin;
boost::filesystem::path g_config;
double g_pip = 0.0001;
std::unique_ptr<fxlib::IForecaster> g_forecaster;

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm) {
  using namespace std;
  options_description basic_desc("Basic options", 200);
  basic_desc.add_options()
    ("help,h", "Show help");
  options_description generic_desc("Generic analyze options", 200);
  generic_desc.add_options()
    ("source,s", value<string>()->required()->value_name("pair-bin")->notifier(
      [](const string& srcname) { g_srcbin = boost::filesystem::canonical(srcname); }), "Path to compiled (binary) quotes.")
      ("algorithm,a", value<string>()->required()->value_name("name"), "Name of algorithm to analyze.")
    ("config,c", value<string>()->required()->value_name("config")->notifier(
      [](const string& cfgname) { g_config = boost::filesystem::canonical(cfgname); }), "Path to algorithm configuration file.");
  options_description additional_desc("Additional options", 200);
  additional_desc.add_options()
    ("pip,z", value<double>(&g_pip)->value_name("size"), "Pip size, usually 0.0001 or 0.01.");
  const auto list_desc = {basic_desc, generic_desc, additional_desc};
  try {
    store(command_line_parser(argc, argv).options(basic_desc).options(generic_desc).allow_unregistered().run(), vm);
    notify(vm);
    if (vm.count("help")) {
      cout << list_desc;
      return false;
    }
  } catch (const exception& e) {
    cout << "[ERROR] Command line: " << e.what() << endl;
    cout << list_desc;
    return false;
  }
  return true;
}
