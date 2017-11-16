#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

using boost::posix_time::time_duration;
using boost::posix_time::minutes;
using boost::posix_time::seconds;

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_config;
extern double g_pip;
extern std::string g_algname;

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm);

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
    boost::property_tree::ptree prop;
    boost::property_tree::read_json(g_config.string(), prop);
    auto forecaster = fxlib::CreateForecaster(g_algname, &prop);
    if (!forecaster) {
      throw invalid_argument("Could not create algorithm '" + g_algname + "'");
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
    cout << "Testing algorithm " << g_algname << "..." << endl;
    const fxlib::ForecastInfo info = forecaster->Info();
    const time_duration wait_operation = seconds(static_cast<long>(60.0 / info.probab));
    const time_duration wait_margin = seconds(static_cast<long>(60.0 * info.durat));
    const time_duration timeout = minutes(info.timeout);
    const time_duration window = minutes(info.window);
    cout << "Wait of operation " << wait_operation << " with margin wait " << wait_margin << endl;
    cout << "Window " << window << " with timeout " << timeout << endl;
    boost::optional<boost::posix_time::ptime> last_positive_cast;
    double actual_wait_operation = 0;
    size_t Np = 0;
    size_t curr_idx = 0;
    int progress = 1;
    size_t progress_idx = (progress * seq.candles.size()) / 10;
    for (auto piter = seq.candles.begin(); piter < seq.candles.end() && piter->time <= (seq.candles.back().time - timeout - window); ++piter, ++curr_idx) {
      if (curr_idx == progress_idx) {
        cout << piter->time << " processed " << (progress * 10) << "%" << endl;
        progress_idx = (++progress * seq.candles.size()) / 10;
      }
      const fxlib::fxforecast cast = forecaster->Feed(*piter);
      if (cast == fxlib::fxforecast::positive) {
        if (last_positive_cast.is_initialized()) {
          const time_duration dt = piter->time - *last_positive_cast;
          actual_wait_operation += dt.total_seconds() / 60.0;
        }
        last_positive_cast = piter->time;
        Np++;
      }
    }
    cout << "Done" << endl;
    cout << "----------------------------------" << endl;
    actual_wait_operation /= (Np - 1);
    cout << "Actual wait of operation " << seconds(static_cast<long>(60.0 * actual_wait_operation)) << endl;
  } catch (const system_error& e) {
    cout << "[ERROR] " << e.what() << endl;
    return e.code().value();
  } catch (const exception& e) {
    cout << "[ERROR] " << e.what() << endl;
    return boost::system::errc::operation_canceled;
  }

  return boost::system::errc::success;
}
