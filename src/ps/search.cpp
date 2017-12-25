#include "fxlib/fxlib.h"

#include "fxlib/helpers/progress.h"

#include <boost/filesystem.hpp>

extern boost::filesystem::path g_srcbin;
extern std::string g_algname;
extern std::tuple<int, int> g_take_profit_range;
extern std::tuple<int, int> g_stop_loss_range;
extern std::tuple<double, double> g_threshold_range;
extern double g_pip;
extern double g_momentum;

using boost::posix_time::time_duration;
using boost::posix_time::ptime;

fxlib::fxsequence LoadingQuotes(const boost::filesystem::path& srcbin);
bool IsWorseForOpen(fxlib::fxposition position, const fxlib::fxcandle& curr, const fxlib::fxcandle& worst);
double Play(fxlib::IForecaster* forecaster, const fxlib::fxsequence& seq, fxlib::fxposition position,
            const boost::posix_time::time_duration& timeout, const boost::posix_time::time_duration& window,
            double profit, double loss, double threshold,
            size_t* N, size_t* Np, double* sum_profit, size_t* Nl, double* sum_loss, double* sum_timeout);

namespace {
template <typename F>
std::tuple<double, double, double> Grad(F& fun, double profit, double loss, double threshold) {
  const double rph = 1 * g_pip;
  const double rlh = 5 * g_pip;
  const double th = 0.005;
  // Derivative by profit
  double fp1 = fun(profit - rph, loss, threshold);
  double fp2 = fun(profit + rph, loss, threshold);
  // Derivative by loss
  double fl1 = fun(profit, loss - rlh, threshold);
  double fl2 = fun(profit, loss + rlh, threshold);
  // Derivative by threshold
  double ft1 = fun(profit, loss, threshold - 1 * th);
  double ft2 = fun(profit, loss, threshold + 1 * th);
  return std::make_tuple((fp2 - fp1) / (2 * rph), (fl2 - fl1) / (2 * rlh), (ft2 - ft1) / (2 * th));
}

std::tuple<double, double, double> operator * (const std::tuple<double, double, double>& a, const std::tuple<double, double, double>& b) {
  using namespace std;
  return forward_as_tuple(get<0>(a) * get<0>(b), get<1>(a) * get<1>(b), get<2>(a) * get<2>(b));
}

std::tuple<double, double, double> operator + (const std::tuple<double, double, double>& a, const std::tuple<double, double, double>& b) {
  using namespace std;
  return forward_as_tuple(get<0>(a) + get<0>(b), get<1>(a) + get<1>(b), get<2>(a) + get<2>(b));
}

std::tuple<double, double, double> operator * (const double& a, const std::tuple<double, double, double>& b) {
  using namespace std;
  return forward_as_tuple(a * get<0>(b), a * get<1>(b), a * get<2>(b));
}
std::ostream& operator << (std::ostream& out, const std::tuple<double, double, double>& t) {
  using namespace std;
  out << get<0>(t) << "," << get<1>(t) << "," << get<2>(t);
  return out;
}

const std::tuple<double, double, double> g_rate(0.005, 0.01, 0.000001);

double SubSearch(fxlib::IForecaster* forecaster, const fxlib::fxsequence& seq, const fxlib::ForecastInfo& info, double& profit, double& loss, double& threshold) {
  using namespace std;
  auto fun = [&](double p, double l, double t) {
    cout << ".";
    return Play(forecaster, seq, info.position, info.timeout, info.window, p, l, t, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  };
  double total = fun(profit, loss, threshold);
  cout << fixed << setprecision(4) << " point [" << (profit / g_pip) << "," << (loss / g_pip) << "," << threshold << "]: " << (total / g_pip) << endl;
  auto grad = Grad(fun, profit, loss, threshold);
  auto dp = g_rate * grad;
  auto pt = make_tuple(profit, loss, threshold) + dp;
  double totalt = fun(get<0>(pt), get<1>(pt), get<2>(pt));
  double eps = abs(total - totalt);
  auto out = [&]() {
    cout << fixed << setprecision(4) << "(" << grad << ")->[" << (profit / g_pip) << "," << (loss / g_pip) << "," << threshold << "]: " << (total / g_pip) << " => " << (eps / g_pip) << endl;
  };
  profit = get<0>(pt);
  loss = get<1>(pt);
  threshold = get<2>(pt);
  total = totalt;
  out();
  while (eps > g_pip / 10) {
    grad = Grad(fun, profit, loss, threshold);
    const auto dpt = g_rate * grad + g_momentum * dp;
    pt = make_tuple(profit, loss, threshold) + dpt;
    totalt = fun(get<0>(pt), get<1>(pt), get<2>(pt));
    const double epst = abs(total - totalt);
    //if (abs(epst) > abs(eps)) break;
    eps = epst;
    dp = dpt;
    profit = get<0>(pt);
    loss = get<1>(pt);
    threshold = get<2>(pt);
    total = totalt;
    out();
  }
  return total;
}
}  // namespace

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
  cout << "Searching rate: [" << g_rate << "] and momentum " << g_momentum << endl;
  cout << "----------------------------------" << endl;
  double profit1 = (get<0>(g_take_profit_range) + get<1>(g_take_profit_range)) * g_pip / 2;
  double loss1 = (get<0>(g_stop_loss_range) + get<1>(g_stop_loss_range)) * g_pip / 2;
  double threshold1 = (get<0>(g_threshold_range) + get<1>(g_threshold_range)) / 2;
  /*double total1 =*/ SubSearch(&*forecaster, seq, info, profit1, loss1, threshold1);
  cout << "----------------------------------" << endl;
  size_t N;
  size_t Np;
  size_t Nl;
  double sum_profit;
  double sum_loss;
  double sum_timeout;
  Play(&*forecaster, seq, info.position, info.timeout, info.window, profit1, loss1, threshold1, &N, &Np, &sum_profit, &Nl, &sum_loss, &sum_timeout);
  cout << "It has been opened " << N << " positions" << endl;
  cout << "Profit has been taken " << Np << " times (" << fixed << setprecision(2) << (double(Np) / double(N) * 100.0) << "%) sum " << sum_profit / g_pip << " pips" << endl;
  cout << "Stop-loss has been happened " << Nl << " times (" << fixed << setprecision(2) << (double(Nl) / double(N) * 100.0) << "%) sum " << sum_loss / g_pip << " pips" << endl;
  cout << "Timeout has been happened " << (N - Np - Nl) << " times (" << fixed << setprecision(2) << (double(N - Np - Nl) / double(N) * 100.0) << "%) sum " << sum_timeout / g_pip << " pips" << endl;
  cout << "Total " << (sum_profit + sum_loss + sum_timeout) / g_pip << " pips" << endl;
}
