#include "fxlib/fxlib.h"

#include <boost/filesystem.hpp>

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_outtxt;
extern std::string g_algname;
extern double g_take_profit;
extern double g_stop_loss;
extern double g_pip;
extern double g_threshold;

using boost::posix_time::time_duration;
using boost::posix_time::ptime;

fxlib::fxsequence LoadingQuotes(const boost::filesystem::path& srcbin);

enum class pos_result {
  profit,
  loss,
  timeout
};

void Quick(const boost::property_tree::ptree& prop, bool out) {
  using namespace std;
  ofstream fout;
  if (out) {
    if (boost::filesystem::is_directory(g_outtxt)) {
      g_outtxt.append(g_srcbin.stem().string() + "-play.log");
    }
    fout.open(g_outtxt.string());
    if (!fout) {
      throw ios_base::failure("Could not open '" + g_outtxt.string() + "'");
    }
  }
  auto forecaster = fxlib::CreateForecaster(g_algname, prop);
  if (!forecaster) {
    throw invalid_argument("Could not create algorithm '" + g_algname + "'");
  }
  const fxlib::fxsequence seq = LoadingQuotes(g_srcbin);
  const fxlib::ForecastInfo info = forecaster->Info();
  //fxlib::fprofit_t profit = info.position == fxlib::fxposition::fxlong ? fxlib::fxprofit_long : fxlib::fxprofit_short;
  cout << "Playing algorithm " << g_algname << " with threshold " << g_threshold << "..." << endl;
  cout << "Position '" << (info.position == fxlib::fxposition::fxlong ? "long" : "short") << "' with take-profit " << g_take_profit << " and stop-loss " << g_stop_loss << endl;
  cout << "Window " << info.window << " with timeout " << info.timeout << endl;
  size_t N = 0;
  size_t Np = 0;
  size_t Nl = 0;
  double sum_profit = 0;
  double sum_loss = 0;
  double sum_timeout = 0;
  size_t curr_idx = 0;
  int progress = 1;
  size_t progress_idx = (progress * seq.candles.size()) / 10;
  for (auto piter = seq.candles.cbegin(); piter < seq.candles.cend() && piter->time <= (seq.candles.back().time - info.timeout - info.window); ++piter, ++curr_idx) {
    if (curr_idx == progress_idx) {
      cout << piter->time << " processed " << (progress * 10) << "%" << endl;
      progress_idx = (++progress * seq.candles.size()) / 10;
    }
    const double est = forecaster->Feed(*piter);
    if (est >= g_threshold) {
      N++;
      // Finding the worst case to open position in window
      const ptime cast_time = piter->time;
      for (; piter->time < (cast_time + info.window); ++piter, ++curr_idx) {
        if (curr_idx == progress_idx) {
          cout << piter->time << " processed " << (progress * 10) << "%" << endl;
          progress_idx = (++progress * seq.candles.size()) / 10;
        }
      }
      // Open position and await result
      const ptime pos_start = piter->time;
      for (; piter->time <= (pos_start + info.timeout); ++piter, ++curr_idx) {
        if (curr_idx == progress_idx) {
          cout << piter->time << " processed " << (progress * 10) << "%" << endl;
          progress_idx = (++progress * seq.candles.size()) / 10;
        }
      }


      //auto pos = CheckPos(piter, curr_idx, info.window, info.timeout, profit);
      //switch (get<0>(pos)) {
      //  case pos_result::profit:
      //    Np++;
      //    sum_profit += get<1>(pos);
      //    break;
      //  case pos_result::loss:
      //    Nl++;
      //    sum_loss += get<1>(pos);
      //    break;
      //  default:
      //    sum_timeout += get<1>(pos);
      //    break;
      //}
    }
  }
  cout << "Done" << endl;
  cout << "----------------------------------" << endl;
  cout << "It has been opened " << N << " positions" << endl;
  cout << "Profit has been taken " << Np << " times (" << fixed << setprecision(2) << (double(Np) / double(N) * 100.0) << "%) total " << sum_profit << " pips" << endl;
  cout << "Stop-loss has been happened " << Nl << " times (" << fixed << setprecision(2) << (double(Nl) / double(N) * 100.0) << "%) total " << sum_loss << " pips" << endl;
  cout << "Timeout has been happened " << (N - Np - Nl) << " times (" << fixed << setprecision(2) << (double(N - Np - Nl) / double(N) * 100.0) << "%) total " << sum_timeout << " pips" << endl;
}
