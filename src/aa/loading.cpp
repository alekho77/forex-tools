#include "fxlib/fxlib.h"

#include <boost/filesystem.hpp>

fxlib::fxsequence LoadingQuotes(const boost::filesystem::path& srcbin) {
    using namespace std;
    cout << "Reading " << srcbin << "..." << endl;
    ifstream fbin(srcbin.string(), ifstream::binary);
    if (!fbin) {
        throw ios_base::failure("Could not open source file'" + srcbin.string() + "'");
    }
    const fxlib::fxsequence seq = fxlib::ReadSequence(fbin);
    if (!fbin) {
        throw ios_base::failure("Could not read source file'" + srcbin.string() + "'");
    }
    if (seq.periodicity != boost::posix_time::minutes(1)) {
        throw logic_error("Wrong sequence periodicity");
    }
    if (seq.period.is_null()) {
        throw logic_error("Wrong sequence period");
    }
    if (seq.candles.empty()) {
        throw logic_error("No data was found in sequence");
    }
    return seq;
}
