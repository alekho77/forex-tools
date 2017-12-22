#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>

extern boost::filesystem::path g_config;
extern bool g_quick_mode;
extern double g_pip;

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm);
void Quick(const boost::property_tree::ptree& prop);

int main(int argc, char* argv[]) {
  using namespace std;
  cout << "Forex Play Simulator." << endl;

  variables_map vm;
  if (!TryParseCommandLine(argc, argv, vm)) {
    return boost::system::errc::invalid_argument;
  }

  try {
    if (!vm.count("pip")) {
      throw invalid_argument("Unknown pip size");
    }
    boost::property_tree::ptree prop;
    boost::property_tree::read_json(g_config.string(), prop);
    prop.put("pip", g_pip);
    if (g_quick_mode) {
      Quick(prop);
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
