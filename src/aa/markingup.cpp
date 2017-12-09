#include "fxlib/fxlib.h"

#include <boost/filesystem.hpp>

extern boost::filesystem::path g_srcbin;
extern std::string g_algname;

fxlib::fxsequence LoadingQuotes(const boost::filesystem::path& srcbin);

void Markup(const boost::property_tree::ptree& /*prop*/) {
  using namespace std;
  const fxlib::fxsequence seq = LoadingQuotes(g_srcbin);

}