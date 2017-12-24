#include "fxlib/fxlib.h"

#include "fxlib/helpers/progress.h"

#include "math/mathlib/derivative.h"

#include <boost/filesystem.hpp>

extern boost::filesystem::path g_srcbin;
extern std::string g_algname;
extern std::tuple<int, int> g_take_profit_range;
extern std::tuple<int, int> g_stop_loss_range;
extern std::tuple<double, double> g_threshold_range;
extern double g_pip;

using boost::posix_time::time_duration;
using boost::posix_time::ptime;

fxlib::fxsequence LoadingQuotes(const boost::filesystem::path& srcbin);
bool IsWorseForOpen(fxlib::fxposition position, const fxlib::fxcandle& curr, const fxlib::fxcandle& worst);
double Play(fxlib::IForecaster* forecaster, const fxlib::fxsequence& seq, fxlib::fxposition position,
            const boost::posix_time::time_duration& timeout, const boost::posix_time::time_duration& window,
            double profit, double loss, double threshold,
            size_t* N, size_t* Np, double* sum_profit, size_t* Nl, double* sum_loss, double* sum_timeout);

void Search(const boost::property_tree::ptree& prop) {
  using namespace std;
  auto forecaster = fxlib::CreateForecaster(g_algname, prop);
  if (!forecaster) {
    throw invalid_argument("Could not create algorithm '" + g_algname + "'");
  }
  const fxlib::fxsequence seq = LoadingQuotes(g_srcbin);
  const fxlib::ForecastInfo info = forecaster->Info();
  cout << "Searching best play params for algorithm " << g_algname << " in threshold range " << get<0>(g_threshold_range) << "-" << get<1>(g_threshold_range) << endl;
  cout << "Position '" << (info.position == fxlib::fxposition::fxlong ? "long" : "short");
  cout << "' take-profit range " << get<0>(g_take_profit_range) << "-" << get<1>(g_take_profit_range);
  cout << " and stop-loss range " << get<0>(g_stop_loss_range) << "-" << get<1>(g_stop_loss_range) << endl;
  cout << "Window " << info.window << " with timeout " << info.timeout << endl;

  auto fun = [&](double p, double l, double t) {
    cout << ".";
    return Play(&*forecaster, seq, info.position, info.timeout, info.window, p, l, t, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  };
  mathlib::derivative<double(double, double, double)> deriv(fun, 1e-6);
  auto fgrad = [&deriv](double p, double l, double t) {
    return forward_as_tuple(deriv.diff<0>(p, l, t), deriv.diff<1>(p, l, t), deriv.diff<2>(p, l, t));
  };

  cout << "----------------------------------" << endl;
  double profit = (get<0>(g_take_profit_range) + get<1>(g_take_profit_range)) * g_pip / 2;
  double loss = (get<0>(g_stop_loss_range) + get<1>(g_stop_loss_range)) * g_pip / 2;
  double threshold = (get<0>(g_threshold_range) + get<1>(g_threshold_range)) / 2;
  double rate = 0.5;
  //double momentum = 0.3;
  
  double total = fun(profit, loss, threshold);
  auto grad = fgrad(profit, loss, threshold);

  cout << "grad: [" << get<0>(grad) << "," << get<1>(grad) << "," << get<2>(grad) << "]; total " << total << " ";

  auto dp = make_tuple(rate * get<0>(grad), rate * get<1>(grad), rate * get<2>(grad));
  auto pt = make_tuple(profit + get<0>(dp), loss + get<1>(dp), threshold + get<2>(dp));
  double totalt = fun(get<0>(pt), get<1>(pt), get<2>(pt));
  double eps = abs(total - totalt);
  auto out = [&]() {
    cout << fixed << setprecision(4) << "[" << (profit / g_pip) << "," << (loss / g_pip) << "," << threshold << "]->[";
    cout << (get<0>(pt) / g_pip) << "," << (get<1>(pt) / g_pip) << "," << get<2>(pt) << "]:" << (eps / g_pip) << endl; };
  out();
  profit = get<0>(pt);
  loss = get<1>(pt);
  threshold = get<2>(pt);
  total = totalt;
  //while (eps > g_pip / 10) {
  //  grad = fgrad(profit, loss, threshold);
  //  auto dpt = make_tuple(rate * get<0>(grad) + momentum * get<0>(dp), rate * get<1>(grad) + momentum * get<1>(dp), rate * get<2>(grad) + momentum * get<2>(dp));
  //  pt = make_tuple(profit + get<0>(dpt), loss + get<1>(dpt), threshold + get<2>(dpt));
  //  totalt = fun(get<0>(pt), get<1>(pt), get<2>(pt));
  //  double epst = abs(total - totalt);
  //  if (abs(epst) > abs(eps)) break;
  //  eps = epst;
  //  dp = dpt;
  //  out();
  //  profit = get<0>(pt);
  //  loss = get<1>(pt);
  //  threshold = get<2>(pt);
  //  total = totalt;
  //}
  cout << "----------------------------------" << endl;
  size_t N;
  size_t Np;
  size_t Nl;
  double sum_profit;
  double sum_loss;
  double sum_timeout;
  Play(&*forecaster, seq, info.position, info.timeout, info.window, profit, loss, threshold, &N, &Np, &sum_profit, &Nl, &sum_loss, &sum_timeout);
  cout << "It has been opened " << N << " positions" << endl;
  cout << "Profit has been taken " << Np << " times (" << fixed << setprecision(2) << (double(Np) / double(N) * 100.0) << "%) sum " << sum_profit / g_pip << " pips" << endl;
  cout << "Stop-loss has been happened " << Nl << " times (" << fixed << setprecision(2) << (double(Nl) / double(N) * 100.0) << "%) sum " << sum_loss / g_pip << " pips" << endl;
  cout << "Timeout has been happened " << (N - Np - Nl) << " times (" << fixed << setprecision(2) << (double(N - Np - Nl) / double(N) * 100.0) << "%) sum " << sum_timeout / g_pip << " pips" << endl;
  cout << "Total " << (sum_profit + sum_loss + sum_timeout) / g_pip << " pips" << endl;
}
