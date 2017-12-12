#include "fxlib/fxlib.h"

#include <boost/filesystem.hpp>

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_outbin;
extern std::string g_algname;

fxlib::fxsequence LoadingQuotes(const boost::filesystem::path& srcbin);

void Markup(const boost::property_tree::ptree& prop) {
  using namespace std;
  auto trainer = fxlib::CreateTrainer(g_algname, prop);
  if (!trainer) {
    throw invalid_argument("Could not create algorithm trainer '" + g_algname + "'");
  }
  if (boost::filesystem::is_directory(g_outbin)) {
    g_outbin.append(g_srcbin.stem().string() + "-" + g_algname + "-training.bin");
  }
  ofstream fbin(g_outbin.string(), ifstream::binary);
  if (!fbin) {
    throw ios_base::failure("Could not open '" + g_outbin.string() + "'");
  }
  const fxlib::fxsequence seq = LoadingQuotes(g_srcbin);
  trainer->onTitling.connect([](const std::string& str) { std::cout << str << std::endl; });
  trainer->PrepareTraningSet(seq, fbin);
}
