#include "fxlib/fxlib.h"

#include <boost/filesystem.hpp>

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_outbin;
extern boost::filesystem::path g_config;
extern std::string g_algname;

fxlib::fxsequence LoadingQuotes(const boost::filesystem::path& srcbin);

void Markup(const boost::property_tree::ptree& prop) {
  using namespace std;
  if (boost::filesystem::is_directory(g_outbin)) {
    g_outbin.append(g_srcbin.stem().string() + "-" + g_config.stem().string() + "-trainset.bin");
  }
  ofstream fbin(g_outbin.string(), ifstream::binary);
  if (!fbin) {
    throw ios_base::failure("Could not open '" + g_outbin.string() + "'");
  }
  boost::filesystem::path logtxt = g_outbin;
  logtxt.replace_extension("log");
  ofstream flog(logtxt.string());
  if (!flog) {
    throw ios_base::failure("Could not open '" + logtxt.string() + "'");
  }
  auto trainer = fxlib::CreateTrainer(g_algname, prop, cout, flog);
  if (!trainer) {
    throw invalid_argument("Could not create algorithm trainer '" + g_algname + "'");
  }
  const fxlib::fxsequence seq = LoadingQuotes(g_srcbin);
  trainer->PrepareTrainingSet(seq, fbin);
}
