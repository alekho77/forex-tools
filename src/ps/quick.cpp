#include "fxlib/fxlib.h"

#include "fxlib/helpers/progress.h"

#include <boost/filesystem.hpp>

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_outtxt;
extern boost::filesystem::path g_config;
extern std::string g_algname;
extern double g_take_profit;
extern double g_stop_loss;
extern double g_pip;
extern double g_threshold;

using boost::posix_time::ptime;
using boost::posix_time::time_duration;

fxlib::fxsequence LoadingQuotes(const boost::filesystem::path& srcbin);
bool IsWorseForOpen(fxlib::fxposition position, const fxlib::fxcandle& curr, const fxlib::fxcandle& worst);

void Quick(const boost::property_tree::ptree& prop, bool out) {
    using namespace std;
    ofstream flog;
    if (out) {
        if (boost::filesystem::is_directory(g_outtxt)) {
            g_outtxt.append(g_srcbin.stem().string() + "-" + g_config.stem().string() + "-play.log");
        }
        flog.open(g_outtxt.string());
        if (!flog) {
            throw ios_base::failure("Could not open '" + g_outtxt.string() + "'");
        }
    }
    auto forecaster = fxlib::CreateForecaster(g_algname, prop);
    if (!forecaster) {
        throw invalid_argument("Could not create algorithm '" + g_algname + "'");
    }
    const fxlib::fxsequence seq = LoadingQuotes(g_srcbin);
    const fxlib::ForecastInfo info = forecaster->Info();
    cout << "Playing algorithm " << g_algname << " with threshold " << g_threshold << "..." << endl;
    cout << "Position '" << (info.position == fxlib::fxposition::fxlong ? "long" : "short") << "' with take-profit "
         << g_take_profit << " and stop-loss " << g_stop_loss << endl;
    cout << "Window " << info.window << " with timeout " << info.timeout << endl;
    size_t N = 0;
    size_t Np = 0;
    size_t Nl = 0;
    double sum_profit = 0;
    double sum_loss = 0;
    double sum_timeout = 0;
    fxlib::helpers::progress progress(seq.candles.size(), cout);
    for (auto piter = seq.candles.cbegin();
         piter < seq.candles.cend() && piter->time <= (seq.candles.back().time - info.timeout - info.window); ++piter) {
        progress(piter - seq.candles.cbegin());
        const double est = forecaster->Feed(*piter);
        if (est >= g_threshold) {
            N++;
            if (flog.is_open()) {
                flog << setfill(' ') << setw(6) << N << " " << piter->time << " ";
            }
            // Finding the worst case to open position in window
            auto iopen = piter;
            for (auto iter = piter + 1; iter->time < (piter->time + info.window); ++iter) {
                if (IsWorseForOpen(info.position, *iter, *iopen)) {
                    iopen = iter;
                }
            }
            if (flog.is_open()) {
                flog << iopen->time << fixed << setprecision(3) << setw(8) << iopen->high << setw(8) << iopen->low
                     << " ";
            }
            // Shift progress to open position
            while (piter < iopen) {
                ++piter;
            }
            // Open position and await result
            const double open_rate = (info.position == fxlib::fxposition::fxlong) ? iopen->high : iopen->low;
            bool trigged = false;
            for (; piter->time <= (iopen->time + info.timeout); ++piter) {
                const double margin =
                    (info.position == fxlib::fxposition::fxlong) ? piter->low - open_rate : open_rate - piter->high;
                if (margin >= g_take_profit * g_pip) {
                    Np++;
                    sum_profit += margin;
                    trigged = true;
                    if (flog.is_open()) {
                        flog << setw(8) << "profit" << fixed << setprecision(1) << setw(8) << margin / g_pip;
                    }
                    break;
                } else if (margin <= -g_stop_loss * g_pip) {
                    Nl++;
                    sum_loss += margin;
                    trigged = true;
                    if (flog.is_open()) {
                        flog << setw(8) << "loss" << fixed << setprecision(1) << setw(8) << margin / g_pip;
                    }
                    break;
                }
            }
            if (!trigged) {
                const double margin =
                    (info.position == fxlib::fxposition::fxlong) ? piter->low - open_rate : open_rate - piter->high;
                sum_timeout += margin;
                if (flog.is_open()) {
                    flog << setw(8) << "timeout" << fixed << setprecision(1) << setw(8) << margin / g_pip;
                }
            }
            if (flog.is_open()) {
                flog << endl;
            }
        }
    }
    cout << "Done" << endl;
    cout << "----------------------------------" << endl;
    cout << "It has been opened " << N << " positions" << endl;
    cout << "Profit has been taken " << Np << " times (" << fixed << setprecision(2) << (double(Np) / double(N) * 100.0)
         << "%) sum " << sum_profit / g_pip << " pips" << endl;
    cout << "Stop-loss has been happened " << Nl << " times (" << fixed << setprecision(2)
         << (double(Nl) / double(N) * 100.0) << "%) sum " << sum_loss / g_pip << " pips" << endl;
    cout << "Timeout has been happened " << (N - Np - Nl) << " times (" << fixed << setprecision(2)
         << (double(N - Np - Nl) / double(N) * 100.0) << "%) sum " << sum_timeout / g_pip << " pips" << endl;
    cout << "Total " << (sum_profit + sum_loss + sum_timeout) / g_pip << " pips" << endl;
}
