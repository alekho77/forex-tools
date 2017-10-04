#include "fxlib/fxlib.h"

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <iostream>

using boost::program_options::options_description;
using boost::program_options::value;
using boost::program_options::variables_map;
using boost::program_options::parse_command_line;
using boost::program_options::store;
using boost::program_options::notify;
using boost::program_options::error;
using boost::program_options::command_line_parser;

void PrintCommandLineOptions(const std::vector<options_description>& opts) {
  using namespace std;
  options_description desc;
  for (const auto& opt : opts) {
    desc.add(opt);
  }
  cout << endl << "Command line options:" << endl << desc << endl;
}

bool TryParseCommandLine(int /*argc*/, char* /*argv*/[], variables_map& /*vm*/) {
  using namespace std;

  return true;
}

int main(int argc, char* argv[]) {
  using namespace std;
  cout << "Forex Analyzer for distribution of limits and stop-losses" << endl;

  variables_map vm;
  if (!TryParseCommandLine(argc, argv, vm)) {
    return boost::system::errc::invalid_argument;
  }

  return boost::system::errc::success;
}