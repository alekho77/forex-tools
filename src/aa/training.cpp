#include "fxlib/fxlib.h"

#include <boost/filesystem.hpp>

#include <fstream>

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_outtxt;
extern std::string g_algname;

void Training(const boost::property_tree::ptree& prop, bool out) {
  using namespace std;
  auto trainer = fxlib::CreateTrainer(g_algname, prop);
  if (!trainer) {
    throw invalid_argument("Could not create algorithm trainer '" + g_algname + "'");
  }
  ofstream fout;
  if (out) {
    if (boost::filesystem::is_directory(g_outtxt)) {
      g_outtxt.append(g_srcbin.stem().string() + "-" + g_algname + "-training.log");
    }
    fout.open(g_outtxt.string());
    if (!fout) {
      throw ios_base::failure("Could not open '" + g_outtxt.string() + "'");
    }
  }
  //const fxlib::fxsequence seq = LoadingQuotes(g_srcbin);
  //trainer->onPreparing.connect([](const std::string& str) { std::cout << str << std::endl; });
  //trainer->PrepareTraningSet(seq, fbin);
}
