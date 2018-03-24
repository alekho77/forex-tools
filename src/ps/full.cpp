#include "fxlib/fxlib.h"

#include "fxlib/helpers/progress.h"

#include <boost/filesystem.hpp>

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_outtxt;
extern boost::filesystem::path g_config;
extern std::string g_algname;
extern std::tuple<int, int> g_take_profit_range;
extern std::tuple<int, int> g_stop_loss_range;
extern double g_pip;
extern double g_threshold;

using boost::posix_time::ptime;
using boost::posix_time::time_duration;

fxlib::fxsequence LoadingQuotes(const boost::filesystem::path& srcbin);
bool IsWorseForOpen(fxlib::fxposition position, const fxlib::fxcandle& curr, const fxlib::fxcandle& worst);
double Play(fxlib::IForecaster* forecaster, const fxlib::fxsequence& seq, fxlib::fxposition position,
            const boost::posix_time::time_duration& timeout, const boost::posix_time::time_duration& window,
            double profit, double loss, double threshold, size_t* N, size_t* Np, double* sum_profit, size_t* Nl,
            double* sum_loss, double* sum_timeout);

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
    const size_t range_size = profit_range_size * loss_range_size;
    vector<size_t> N(range_size, 0);
    vector<size_t> Np(range_size, 0);
    vector<size_t> Nl(range_size, 0);
    vector<double> sum_profit(range_size, 0.0);
    vector<double> sum_loss(range_size, 0.0);
    vector<double> sum_timeout(range_size, 0.0);

    cout << "----------------------------------" << endl;
    fxlib::helpers::progress progress(range_size, cout);
    for (int profit = get<0>(g_take_profit_range); profit <= get<1>(g_take_profit_range); profit++) {
        for (int loss = get<0>(g_stop_loss_range); loss <= get<1>(g_stop_loss_range); loss++) {
            const size_t idx =
                (profit - get<0>(g_take_profit_range)) * loss_range_size + (loss - get<0>(g_stop_loss_range));
            progress(idx);
            /*double total_sum =*/Play(&*forecaster, seq, info.position, info.timeout, info.window, profit * g_pip,
                                       loss * g_pip, g_threshold, &N[idx], &Np[idx], &sum_profit[idx], &Nl[idx],
                                       &sum_loss[idx], &sum_timeout[idx]);
        }
    }
    cout << "----------------------------------" << endl;
    // cout << "It has been opened " << N << " positions" << endl;
    // cout << "Profit has been taken " << Np << " times (" << fixed << setprecision(2) << (double(Np) / double(N) *
    // 100.0) << "%) sum " << sum_profit / g_pip << " pips" << endl;  cout << "Stop-loss has been happened " << Nl << "
    // times (" << fixed << setprecision(2) << (double(Nl) / double(N) * 100.0) << "%) sum " << sum_loss / g_pip << "
    // pips" << endl;  cout << "Timeout has been happened " << (N - Np - Nl) << " times (" << fixed << setprecision(2) <<
    // (double(N - Np - Nl) / double(N) * 100.0) << "%) sum " << sum_timeout / g_pip << " pips" << endl;  cout << "Total "
    // << (sum_profit + sum_loss + sum_timeout) / g_pip << " pips" << endl;
}
