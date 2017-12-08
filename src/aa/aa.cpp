#include "fxlib/fxlib.h"
#include "fxlib/helpers/program_options.h"

#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/optional.hpp>

#include <algorithm>

using boost::posix_time::time_duration;
using boost::posix_time::minutes;
using boost::posix_time::seconds;

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_config;
extern double g_pip;
extern std::string g_algname;

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm);

bool CheckPos(const boost::posix_time::ptime pos, const fxlib::markers& marks, const time_duration window) {
  auto icandidate = std::lower_bound(marks.cbegin(), marks.cend(), pos);
  if (icandidate != marks.cend() && *icandidate - pos < window) {
    return true;
  }
  return false;
}

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
    const fxlib::ForecastInfo info = forecaster->Info();
    fxlib::fprofit_t profit = info.position == fxlib::fxposition::fxlong ? fxlib::fxprofit_long : fxlib::fxprofit_short;
    const time_duration timeout = minutes(info.timeout);
    const time_duration window = minutes(info.window);
    cout << "Markup of rate sequence... " << endl;
    double time_adjust;
    double probab;
    double durat;
    auto marks = fxlib::GeniunePositions(seq, timeout, profit, info.margin * g_pip, time_adjust, probab, durat);
    const time_duration wait_operation = seconds(static_cast<long>(time_adjust * 60.0 / probab));
    const time_duration wait_margin = seconds(static_cast<long>(60.0 * durat));
    cout << "Geniune positions: " << marks.size() << endl;
    cout << "Testing algorithm " << g_algname << "..." << endl;
    cout << "Actual wait of operation " << wait_operation << " with margin wait " << wait_margin << endl;
    cout << "Window " << window << " with timeout " << timeout << endl;
    size_t N = 0;
    size_t Ngp = 0;
    const size_t distsize = 100;
    vector<double> actual_wait_operation(distsize);
    vector<boost::optional<boost::posix_time::ptime>> last_positive_cast(distsize);
    vector<size_t> Np(distsize + 1);
    vector<size_t> Nfa(distsize + 1);  // False acceptance
    vector<size_t> Nfr(distsize + 1);  // False rejection
    size_t curr_idx = 0;
    int progress = 1;
    size_t progress_idx = (progress * seq.candles.size()) / 10;
    for (auto piter = seq.candles.begin(); piter < seq.candles.end() && piter->time <= (seq.candles.back().time - timeout - window); ++piter, ++curr_idx, ++N) {
      if (curr_idx == progress_idx) {
        cout << piter->time << " processed " << (progress * 10) << "%" << endl;
        progress_idx = (++progress * seq.candles.size()) / 10;
      }
      const double est = forecaster->Feed(*piter);
      const bool genuine = CheckPos(piter->time, marks, window);
      if (genuine) {
        Ngp++;
      }
      for (size_t i = 0; i <= distsize; i++) {
        const bool pcast = est >= (double(i) / double(distsize));
        if (pcast) {
          if (last_positive_cast[i].is_initialized()) {
            const time_duration dt = piter->time - *last_positive_cast[i];
            actual_wait_operation[i] += dt.total_seconds() / 60.0;
          }
          last_positive_cast[i] = piter->time;
          Np[i]++;
        }
        if (genuine) {
          if (!pcast) {
            Nfr[i]++;
          }
        } else {
          if (pcast) {
            Nfa[i]++;
          }
        }
      }
    }
    cout << "Done" << endl;
    cout << "Number of casts: " << N << endl;
    cout << "----------------------------------" << endl;
    string out_file = g_algname + "-" + g_srcbin.filename().stem().string() + ".gpl";
    cout << "Writing " << out_file << "... ";
    ofstream fout(out_file);
    if (!fout) {
      throw ios_base::failure("Could not open '" + out_file + "'");
    }
    fout << "N=" << N << endl;
    fout << "Nga=" << Ngp << endl;
    fout << "Ngr=" << N - Ngp << endl;
    fout << "# (1)t   (2)Na   (3)Nr   (4)Ea   (5)Er   (6)FAR   (7)FRR     (8)T" << endl;
    fout << "$Distrib << EOD" << endl;
    for (size_t i = 0; i <= distsize; i++) {
      actual_wait_operation[i] /= Np[i] > 1 ? (Np[i] - 1) : 1;
      const double FAR = (N - Ngp) > 0 ? (double)(Nfa[i]) / (double)(N - Ngp) : 0;
      const double FRR = Ngp > 0 ? (double)(Nfr[i]) / (double)(Ngp) : 0;
      fout << setw(6) << setfill(' ') << fixed << setprecision(4) << double(i) / double(distsize) << " ";
      fout << setw(7) << setfill(' ') << Np[i] << " ";
      fout << setw(7) << setfill(' ') << N - Np[i] << " ";
      fout << setw(7) << setfill(' ') << Nfa[i] << " ";
      fout << setw(7) << setfill(' ') << Nfr[i] << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(6) << FAR << " ";
      fout << setw(8) << setfill(' ') << fixed << setprecision(6) << FRR << " ";
      fout << setw(8) << setfill(' ') << seconds(static_cast<long>(60.0 * actual_wait_operation[i])) << endl;
    }
    fout << "EOD" << endl;
    cout << "done" << endl;
  } catch (const system_error& e) {
    cout << "[ERROR] " << e.what() << endl;
    return e.code().value();
  } catch (const exception& e) {
    cout << "[ERROR] " << e.what() << endl;
    return boost::system::errc::operation_canceled;
  }

  return boost::system::errc::success;
}
