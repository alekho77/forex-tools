#include "fxlib/fxlib.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>

extern double g_pip;
extern std::string g_algname;
extern boost::filesystem::path g_srcbin;
extern size_t g_distr_size;

using boost::posix_time::time_duration;
using boost::posix_time::minutes;
using boost::posix_time::seconds;

bool CheckPos(const boost::posix_time::ptime pos, const fxlib::markers& marks, const time_duration window) {
  auto icandidate = std::lower_bound(marks.cbegin(), marks.cend(), pos);
  if (icandidate != marks.cend() && *icandidate - pos < window) {
    return true;
  }
  return false;
}

void Analyze(const boost::property_tree::ptree& prop, const fxlib::fxsequence seq) {
  using namespace std;
  auto forecaster = fxlib::CreateForecaster(g_algname, &prop);
  if (!forecaster) {
    throw invalid_argument("Could not create algorithm '" + g_algname + "'");
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
  vector<double> actual_wait_operation(g_distr_size);
  vector<boost::optional<boost::posix_time::ptime>> last_positive_cast(g_distr_size);
  vector<size_t> Np(g_distr_size + 1);
  vector<size_t> Nfa(g_distr_size + 1);  // False acceptance
  vector<size_t> Nfr(g_distr_size + 1);  // False rejection
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
    for (size_t i = 0; i <= g_distr_size; i++) {
      const bool pcast = est >= (double(i) / double(g_distr_size));
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
  for (size_t i = 0; i <= g_distr_size; i++) {
    actual_wait_operation[i] /= Np[i] > 1 ? (Np[i] - 1) : 1;
    const double FAR = (N - Ngp) > 0 ? (double)(Nfa[i]) / (double)(N - Ngp) : 0;
    const double FRR = Ngp > 0 ? (double)(Nfr[i]) / (double)(Ngp) : 0;
    fout << setw(6) << setfill(' ') << fixed << setprecision(4) << double(i) / double(g_distr_size) << " ";
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
}
