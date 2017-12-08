#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_config;
extern bool g_analyze_mode;
extern bool g_learn_mode;

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm);
void Analyze(const boost::property_tree::ptree& prop, const fxlib::fxsequence seq);
void Learning(const boost::property_tree::ptree& prop, const fxlib::fxsequence seq);

int main(int argc, char* argv[]) {
  using namespace std;
  cout << "Forex Analyzer for forecast algorithms." << endl;

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

    boost::property_tree::ptree prop;
    boost::property_tree::read_json(g_config.string(), prop);
    if (g_analyze_mode) {
      Analyze(prop, seq);
    } else if (g_learn_mode) {
      Learning(prop, seq);
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
