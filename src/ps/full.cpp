#include "fxlib/fxlib.h"

#include <boost/filesystem.hpp>

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_outtxt;
extern boost::filesystem::path g_config;
extern std::string g_algname;
extern std::tuple<int, int> g_take_profit_range;
extern std::tuple<int, int> g_stop_loss_range;
extern double g_pip;
extern double g_threshold;

using boost::posix_time::time_duration;
using boost::posix_time::ptime;

fxlib::fxsequence LoadingQuotes(const boost::filesystem::path& srcbin);
bool IsWorseForOpen(fxlib::fxposition position, const fxlib::fxcandle& curr, const fxlib::fxcandle& worst);

void Full(const boost::property_tree::ptree& prop) {
  using namespace std;
  ofstream fout;
  if (boost::filesystem::is_directory(g_outtxt)) {
    g_outtxt.append(g_srcbin.stem().string() + "-" + g_config.stem().string() + "-play.gpl");
  }
  fout.open(g_outtxt.string());
  if (!fout) {
    throw ios_base::failure("Could not open '" + g_outtxt.string() + "'");
  }
  auto forecaster = fxlib::CreateForecaster(g_algname, prop);
  if (!forecaster) {
    throw invalid_argument("Could not create algorithm '" + g_algname + "'");
  }
  const fxlib::fxsequence seq = LoadingQuotes(g_srcbin);
  const fxlib::ForecastInfo info = forecaster->Info();
  cout << "Playing algorithm " << g_algname << " with threshold " << g_threshold << "..." << endl;
  cout << "Position '" << (info.position == fxlib::fxposition::fxlong ? "long" : "short");
  cout << "' with take-profit " << get<0>(g_take_profit_range) << "-" << get<1>(g_take_profit_range);
  cout << " and stop-loss " << get<0>(g_stop_loss_range) << "-" << get<1>(g_stop_loss_range) << endl;
  cout << "Window " << info.window << " with timeout " << info.timeout << endl;
  
  const size_t profit_range_size = get<1>(g_take_profit_range) - get<0>(g_take_profit_range) + 1;
  const size_t loss_range_size = get<1>(g_stop_loss_range) - get<0>(g_stop_loss_range) + 1;
  const size_t size = profit_range_size * loss_range_size;
  vector<size_t> N(size, 0);
  vector<size_t> Np(size, 0);
  vector<size_t> Nl(size, 0);
  vector<double> sum_profit(size, 0.0);
  vector<double> sum_loss(size, 0.0);
  vector<double> sum_timeout(size, 0.0);
  
  for (int profit = get<0>(g_take_profit_range); profit <= get<1>(g_take_profit_range); profit++) {
    for (int loss = get<0>(g_stop_loss_range); loss <= get<1>(g_stop_loss_range); loss++) {
      cout << "Processed profit " << profit << ", loss " << loss;
      const size_t idx = (profit - get<0>(g_take_profit_range)) * loss_range_size + (loss - get<0>(g_stop_loss_range));
      size_t curr_idx = 0;
      int progress = 1;
      size_t progress_idx = (progress * seq.candles.size()) / 10;
      for (auto piter = seq.candles.cbegin(); piter < seq.candles.cend() && piter->time <= (seq.candles.back().time - info.timeout - info.window); ++piter, ++curr_idx) {
        if (curr_idx == progress_idx) {
          cout << ".";
          progress_idx = (++progress * seq.candles.size()) / 10;
        }
        const double est = forecaster->Feed(*piter);
        if (est >= g_threshold) {
          N[idx]++;
          // Finding the worst case to open position in window
          auto iopen = piter;
          for (auto iter = piter + 1; iter->time < (piter->time + info.window); ++iter) {
            if (IsWorseForOpen(info.position, *iter, *iopen)) {
              iopen = iter;
            }
          }
          // Shift progress to open position
          while (piter < iopen) {
            ++piter;
            ++curr_idx;
            if (curr_idx == progress_idx) {
              cout << ".";
              progress_idx = (++progress * seq.candles.size()) / 10;
            }
          }
          // Open position and await result
          const double open_rate = (info.position == fxlib::fxposition::fxlong) ? iopen->high : iopen->low;
          bool trigged = false;
          for (; piter->time <= (iopen->time + info.timeout); ++piter, ++curr_idx) {
            if (curr_idx == progress_idx) {
              cout << ".";
              progress_idx = (++progress * seq.candles.size()) / 10;
            }
            const double margin = (info.position == fxlib::fxposition::fxlong) ? piter->low - open_rate : open_rate - piter->high;
            if (margin >= profit * g_pip) {
              Np[idx]++;
              sum_profit[idx] += margin;
              trigged = true;
              break;
            } else if (margin <= -loss * g_pip) {
              Nl[idx]++;
              sum_loss[idx] += margin;
              trigged = true;
              break;
            }
          }
          if (!trigged) {
            const double margin = (info.position == fxlib::fxposition::fxlong) ? piter->low - open_rate : open_rate - piter->high;
            sum_timeout[idx] += margin;
          }
        }
      }
      cout << endl;
    }
  }
  cout << "Done" << endl;
  cout << "----------------------------------" << endl;
  //cout << "It has been opened " << N << " positions" << endl;
  //cout << "Profit has been taken " << Np << " times (" << fixed << setprecision(2) << (double(Np) / double(N) * 100.0) << "%) sum " << sum_profit / g_pip << " pips" << endl;
  //cout << "Stop-loss has been happened " << Nl << " times (" << fixed << setprecision(2) << (double(Nl) / double(N) * 100.0) << "%) sum " << sum_loss / g_pip << " pips" << endl;
  //cout << "Timeout has been happened " << (N - Np - Nl) << " times (" << fixed << setprecision(2) << (double(N - Np - Nl) / double(N) * 100.0) << "%) sum " << sum_timeout / g_pip << " pips" << endl;
  //cout << "Total " << (sum_profit + sum_loss + sum_timeout) / g_pip << " pips" << endl;
}
