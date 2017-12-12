#include "fxlib/fxlib.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <fstream>

extern boost::filesystem::path g_srcbin;
extern boost::filesystem::path g_outtxt;
extern boost::filesystem::path g_config;
extern std::string g_algname;

void Training(const boost::property_tree::ptree& prop, bool out) {
  using namespace std;
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
  auto trainer = fxlib::CreateTrainer(g_algname, prop, cout, out ? fout : cout);
  if (!trainer) {
    throw invalid_argument("Could not create algorithm trainer '" + g_algname + "'");
  }
  ifstream fin(g_srcbin.string(), ifstream::binary);
  if (!fin) {
    throw ios_base::failure("Could not open '" + g_srcbin.string() + "'");
  }
  trainer->LoadTrainingSet(fin);
  trainer->Train();
  boost::property_tree::ptree res = prop;
  res.erase("pip");
  trainer->SaveResult(res);
  boost::property_tree::write_json(g_config.string(), res);
}
