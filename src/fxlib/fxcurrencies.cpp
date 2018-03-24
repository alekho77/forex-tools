#include "fxcurrencies.h"

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <iterator>

namespace fxlib {

// Define major currencies
#define EUR "EUR"
#define USD "USD"
#define GBP "GBP"
#define CHF "CHF"
#define JPY "JPY"
#define CAD "CAD"
#define AUD "AUD"
#define NZD "NZD"

// Define major pairs
#define PAIR(A, B) (A B)
#define EURUSD PAIR(EUR, USD)
#define GBPUSD PAIR(GBP, USD)
#define USDCHF PAIR(USD, CHF)
#define USDJPY PAIR(USD, JPY)
#define USDCAD PAIR(USD, CAD)
#define AUDUSD PAIR(AUD, USD)
#define NZDUSD PAIR(NZD, USD)

// Define cross-pairs
#define EURJPY PAIR(EUR, JPY)
#define EURGBP PAIR(EUR, GBP)
#define EURCHF PAIR(EUR, CHF)
#define GBPJPY PAIR(GBP, JPY)
#define GBPCHF PAIR(GBP, CHF)

const char* fxcurrencies[] = {USD, EUR, GBP, CHF, JPY, CAD, AUD, NZD};
const char* fxpairs[] = {EURUSD, GBPUSD, USDCHF, USDJPY, USDCAD, AUDUSD,
                         NZDUSD, EURJPY, EURGBP, EURCHF, GBPJPY, GBPCHF};

bool IsCurrency(const std::string& curr) {
    return std::any_of(std::cbegin(fxcurrencies), std::cend(fxcurrencies),
                       [&curr](const auto& c) { return boost::algorithm::iequals(c, curr); });
}

bool IsPair(const std::string& pair) {
    if (pair.size() == 6) {
        const std::string curr1 = pair.substr(0, 3);
        const std::string curr2 = pair.substr(3, 3);
        return IsCurrency(curr1) && IsCurrency(curr2);
    }
    return false;
}

}  // namespace fxlib
