#include "fxlib/fxlib.h"

#pragma warning(push)
#pragma warning(disable:4505)  // warning C4505: unreferenced local function has been removed
#include <boost/program_options.hpp>
#pragma warning(pop)
#include <boost/system/error_code.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <map>
#include <vector>

using boost::posix_time::time_duration;
using boost::posix_time::minutes;
//using boost::posix_time::hours;

// using boost::gregorian::days;
// using boost::gregorian::weeks;

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
  bool quick_mode = false;
  generic_desc.add_options()
    ("source,s",value<string>()->required()->value_name("pair-bin"),"Path to compiled (binary) quotes.")
    ("quick,q", bool_switch(&quick_mode), "Start a quick (simple) analyze.");
  options_description quick_desc("Quick analyze options", 200);
  quick_desc.add_options()
    ("position,p", value<string>()->required()->value_name("{long|short}"), "What position to be analyzed.")
    ("timeout,t", value<string>()->required()->value_name("n{m,h,d,w}"), "How far to look into future (minutes, hours, days, weeks).");
  try {
    store(command_line_parser(argc, argv).options(basic_desc).options(generic_desc).allow_unregistered().run(), vm);
    notify(vm);
    if (vm.count("help")) {
      PrintCommandLineOptions({basic_desc, generic_desc, quick_desc});
      return false;
    }
    if (quick_mode) {
      options_description desc;
      store(parse_command_line(argc, argv, desc.add(generic_desc).add(quick_desc)), vm);
      notify(vm);
    } else {
      throw error("No one mode of analyze has been found!");
    }
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

  try {
    const string binfile_name = vm["source"].as<string>();
    boost::filesystem::path bin_file(binfile_name);
    boost::system::error_code ec;
    bin_file = boost::filesystem::canonical(bin_file, ec);
    if (ec) {
      throw ios_base::failure("Source file '" + binfile_name + "' has not been found");
    }
    cout << "Reading " << bin_file << "..." << endl;
    ifstream fbin(bin_file.c_str(), ifstream::binary);
    if (!fbin) {
      throw ios_base::failure("Could not open source file'" + binfile_name + "'");
    }
    const fxlib::fxsequence seq = fxlib::ReadSequence(fbin);
    if (!fbin) {
      throw ios_base::failure("Could not read source file'" + binfile_name + "'");
    }
    fbin.close();
    if (seq.periodicity != fxlib::fxperiodicity::minutely) {
      throw logic_error("Wrong sequence periodicity");
    }
    if (seq.period.is_null()) {
      throw logic_error("Wrong sequence period");
    }
    if (seq.candles.empty()) {
      throw logic_error("No data was found in sequence");
    }
    if (vm.count("quick")) {
      const string str_tm = vm["timeout"].as<string>();
      boost::regex rx_timeout("(\\d+)([mhdw])");
      boost::smatch what_tm;
      if (!boost::regex_match(str_tm, what_tm, rx_timeout)) {
        throw invalid_argument("Wrong timeout '" + str_tm + "'");
      }
      const int tm_val = stoi(what_tm[1]);
      const map<string, int> fxperiods = {{"m", static_cast<int>(fxlib::fxperiodicity::minutely)},
                                          {"h", static_cast<int>(fxlib::fxperiodicity::hourly)},
                                          {"d", static_cast<int>(fxlib::fxperiodicity::daily)},
                                          {"w", static_cast<int>(fxlib::fxperiodicity::weekly)}};
      const time_duration timeout = minutes(fxperiods.at(what_tm[2]));
      const string positon = boost::algorithm::to_lower_copy(vm["position"].as<string>());
      if (positon == "long") {

      } else if (positon == "short") {

      } else {
        throw invalid_argument("Wrong position '" + positon + "'");
      }
      cout << "Analyzing near " << seq.candles.size() << " " << positon << " positions with " << timeout << " timeout..." << endl;
      vector<double> max_limits;
      vector<double> max_losses;
      max_limits.reserve(seq.candles.size());
      max_losses.reserve(seq.candles.size());
      size_t curr_idx = 0;
      map<size_t, string> progress_tags;
      for (size_t i = 1; i < 10; i++) {
        const string tag = "processed " + to_string(i * 10) + "%";
        progress_tags.insert({(i * seq.candles.size()) / 10, tag});
      }
      for (auto citer = seq.candles.begin(); citer < seq.candles.end() && citer->time <= (seq.candles.back().time - timeout); ++citer, ++curr_idx) {
        auto itag = progress_tags.find(curr_idx);
        if (itag != progress_tags.end()) {
          cout << itag->second << endl;
        }
      }
    }  // if quick
  } catch (const system_error& e) {
    cout << "[ERROR] " << e.what() << endl;
    return e.code().value();
  } catch (const exception& e) {
    cout << "[ERROR] " << e.what() << endl;
    return boost::system::errc::operation_canceled;
  }

  return boost::system::errc::success;
}