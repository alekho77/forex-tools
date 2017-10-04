#include "fxlib/fxlib.h"

#pragma warning(push)
#pragma warning(disable:4505)  // warning C4505: unreferenced local function has been removed
#include <boost/program_options.hpp>
#pragma warning(pop)
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
using boost::program_options::bool_switch;

void PrintCommandLineOptions(const std::vector<options_description>& opts) {
  using namespace std;
  options_description desc;
  for (const auto& opt : opts) {
    desc.add(opt);
  }
  cout << endl << "Command line options:" << endl << desc << endl;
}

bool TryParseCommandLine(int argc, char* argv[], variables_map& vm) {
  using namespace std;
  options_description basic_desc("Basic options", 200);
  basic_desc.add_options()
    ("help,h", "Show help");
  options_description generic_desc("Generic analyze options", 200);
  generic_desc.add_options()
    ("source,s",value<string>()->required()->value_name("pair-bin"),"Path to compiled (binary) quotes.")
    ("quick,q", bool_switch()->default_value(true), "Start a quick (simple) analyze.");
  options_description quick_desc("Quick analyze options", 200);
  quick_desc.add_options()
    ("position,p", value<string>()->required()->value_name("{long|short}"), "What position to be analyzed.")
    ("timeout,t", value<string>()->required()->value_name("n{m,h,d,w}"), "How far to look into future (minutes, hours, days, weeks).");
  try {
    store(command_line_parser(argc, argv).options(basic_desc).allow_unregistered().run(), vm);
    notify(vm);
    if (vm.count("help")) {
      PrintCommandLineOptions({basic_desc, generic_desc, quick_desc});
      return false;
    }
    //options_description desc;
    //store(parse_command_line(argc, argv, desc.add(generic_desc)), vm);
    //notify(vm);
  } catch (const error& e) {
    cout << "[ERROR] Command line: " << e.what() << endl;
    PrintCommandLineOptions({basic_desc, generic_desc, quick_desc});
    return false;
  }
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