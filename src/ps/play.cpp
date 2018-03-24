#include "fxlib/fxlib.h"

bool IsWorseForOpen(fxlib::fxposition position, const fxlib::fxcandle& curr, const fxlib::fxcandle& worst) {
    if (position == fxlib::fxposition::fxlong) {
        return curr.high > worst.high;
    }
    return curr.low < worst.low;
}

double Play(fxlib::IForecaster* forecaster, const fxlib::fxsequence& seq, fxlib::fxposition position,
            const boost::posix_time::time_duration& timeout, const boost::posix_time::time_duration& window,
            double profit, double loss, double threshold, size_t* N, size_t* Np, double* sum_profit, size_t* Nl,
            double* sum_loss, double* sum_timeout) {
    double sprofit = 0;
    double sloss = 0;
    double stimeout = 0;
    if (N)
        *N = 0;
    if (Np)
        *Np = 0;
    if (Nl)
        *Nl = 0;
    for (auto piter = seq.candles.cbegin();
         piter < seq.candles.cend() && piter->time <= (seq.candles.back().time - timeout - window); ++piter) {
        const double est = forecaster->Feed(*piter);
        if (est >= threshold) {
            if (N)
                ++(*N);
            // Finding the worst case to open position in window
            auto iopen = piter;
            for (auto iter = piter + 1; iter->time < (piter->time + window); ++iter) {
                if (IsWorseForOpen(position, *iter, *iopen)) {
                    iopen = iter;
                }
            }
            // Shift progress to open position
            while (piter < iopen) {
                ++piter;
            }
            // Open position and await result
            const double open_rate = (position == fxlib::fxposition::fxlong) ? iopen->high : iopen->low;
            bool trigged = false;
            for (; piter->time <= (iopen->time + timeout); ++piter) {
                const double margin =
                    (position == fxlib::fxposition::fxlong) ? piter->low - open_rate : open_rate - piter->high;
                if (margin >= profit) {
                    if (Np)
                        ++(*Np);
                    sprofit += margin;
                    trigged = true;
                    break;
                } else if (margin <= -loss) {
                    if (Nl)
                        ++(*Nl);
                    sloss += margin;
                    trigged = true;
                    break;
                }
            }
            if (!trigged) {
                const double margin =
                    (position == fxlib::fxposition::fxlong) ? piter->low - open_rate : open_rate - piter->high;
                stimeout += margin;
            }
        }
    }
    if (sum_profit)
        *sum_profit = sprofit;
    if (sum_loss)
        *sum_loss = sloss;
    if (sum_timeout)
        *sum_timeout = stimeout;
    forecaster->Reset();
    return sprofit + sloss + stimeout;
}
