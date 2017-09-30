// qc.cpp : Defines the entry point for the console application.
//

#include "fxlib/fxlib.h"
#include "fxlib/finam/finam.h"
#include "fxlib/helpers/string_conversion.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>

#include <vector>
#include <string>
#include <tuple>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <sstream>
#include <stdexcept>

using fxlib::conversion::string_narrow;
using fxlib::conversion::string_widen;

using boost::posix_time::ptime;
using boost::posix_time::minutes;
using boost::posix_time::hours;
using boost::posix_time::time_period;
using boost::posix_time::time_duration;
using boost::posix_time::to_simple_string;

using boost::gregorian::date_period;
using boost::gregorian::to_simple_string;
using boost::gregorian::from_undelimited_string;

using boost::program_options::options_description;
using boost::program_options::value;
using boost::program_options::variables_map;
using boost::program_options::parse_command_line;
using boost::program_options::store;
//using boost::program_options::parsed_options;
using boost::program_options::notify;
using boost::program_options::error;
using boost::program_options::command_line_parser;
//using boost::program_options::bool_switch;

static const minutes tpAllowableGap{60};

time_duration CalcLongGap(const ptime& last_time, const boost::gregorian::date& date) {
  const boost::gregorian::date last_date = (last_time - minutes(1)).date();
  time_duration delta = (last_date.day_of_week() == boost::gregorian::Saturday) ? minutes(0) : (ptime(last_date, hours(24)) - last_time);
  boost::gregorian::day_iterator ditr{last_date};
  for (++ditr; ditr < date; ++ditr) {
    delta += fxlib::ForexOpenHours(*ditr).length();
  }
  return delta;
}

int main(int argc, char* argv[])
{
  using namespace std;
  cout << "Forex Quotation Compiler" << endl;

  string src_dir;
  string pair_name;
  variables_map vm;
  try {
    options_description desc("Command line options");
    desc.add_options()
      ("help,h", "Show help");
    options_description compile_desc(200);
    compile_desc.add_options()
      ("source,s", value<string>(&src_dir)->required()->value_name("src-dir"), "Path to source directory.")
      ("pair,p", value<string>(&pair_name)->required()->value_name("pair-name"), "Quotation pair name.")
      ("out,o", value<string>()->value_name("out-file"), "Filename to binary output, 'pair-name.bin' by default.")
      ("rewrite,r", "Rewrite existing output file.")
      ("gap,g", value<int>()->value_name("min")->default_value(60), "Allowable gap in minutes, 0 - to suppress informing.")
      ("warn,w", value<string>()->default_value("true")->implicit_value("true"), "Show warnings.");
    store(command_line_parser(argc, argv).options(desc).allow_unregistered().run(), vm);
    notify(vm);
    desc.add(compile_desc);
    if (vm.empty() || vm.count("help")) {
      cout << desc << endl;
      return boost::system::errc::invalid_argument;
    }
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);
  } catch (const error& e) {
    cout << e.what() << endl;
    return boost::system::errc::invalid_argument;
  }

  boost::system::error_code ec;
  boost::filesystem::path src_path = boost::filesystem::canonical(src_dir, ec);
  if (ec) {
    cout << "[ERROR] Source directory \"" << src_dir << "\" has not found" << endl;
    return ec.value();
  }
  
  if (!fxlib::IsPair(pair_name)) {
    cout << "[ERROR] The pair name " << pair_name << " is not valid";
    return boost::system::errc::invalid_argument;
  }
  boost::algorithm::to_upper(pair_name);

  boost::filesystem::path out_file = pair_name + ".bin";
  if (vm.count("out")) {
    out_file = vm["out"].as<string>();
  }
  boost::filesystem::path out_path = boost::filesystem::canonical(out_file.parent_path(), ec);
  if (ec) {
    cout << "[ERROR] Destination directory \"" << out_file << "\" has not found" << endl;
    return ec.value();
  }
  out_path.append(out_file.filename().c_str());
  //if (boost::filesystem::exists(out_path)) {
  //  // TODO: add hash checking to be sure that really nothing to do.
  //  cout << "[NOTE] The binary file " << out_path << " already exists. Nothing to do." << endl;
  //  return boost::system::errc::success;
  //}

  cout << "Compile all [" << pair_name << "]-files in " << src_path << " to binary file " << out_path << endl;

  vector<tuple<boost::filesystem::path, date_period>> src_list;
  
  const boost::regex fmask("^" + pair_name + "_([0-9]{6})_([0-9]{6})\\.txt$");
  for (auto entry : boost::make_iterator_range(boost::filesystem::directory_iterator(src_path), {})) {
    boost::smatch what;
    if (boost::filesystem::is_regular_file(entry)) {
      const string filename = string_narrow({entry.path().filename().c_str()});
      if (boost::regex_match(filename, what, fmask)) {
        const date_period period(from_undelimited_string("20" + what[1].str()),
                                 from_undelimited_string("20" + what[2].str()) + boost::gregorian::date_duration(1));
        src_list.push_back(make_tuple(entry.path(), period));
      }
    }
  }

  if (src_list.empty()) {
    cout << "[ERROR] No one source file has been found!" << endl;
    return boost::system::errc::no_such_file_or_directory;
  }

  sort(begin(src_list), end(src_list), [](const auto& a, const auto& b) { return get<1>(a) < get<1>(b); });
  
  date_period total_period = get<1>(src_list[0]);
  for (size_t i = 1; i < src_list.size(); i++) {
    if (total_period.is_adjacent(get<1>(src_list[i]))) {
      total_period = total_period.span(get<1>(src_list[i]));
    } else {
      cout << "[ERROR] There is a gap between " << get<0>(src_list[i - 1]) << " and " << get<0>(src_list[i]) << endl;
      return boost::system::errc::argument_out_of_domain;
    }
  }
  cout << "Found " << src_list.size() << " source files with total period " << total_period << endl;

  fxlib::fxsequence seq = {fxlib::fxperiodicity::minutely, total_period, {}};
  size_t min_count = 0;
  for (boost::gregorian::day_iterator ditr = {total_period.begin()}; ditr < total_period.end(); ++ditr) {
    const time_period open_period = fxlib::ForexOpenHours(*ditr);
    min_count += open_period.length().total_seconds() / 60;
  }
  seq.candles.reserve(min_count);
  cout << "Expected total " << min_count << " minutely quotes" << endl;

  try {
    for (const auto& src : src_list) {
      // Open source file and check header
      const string fullfilename = string_narrow(get<0>(src).c_str());
      ifstream fin(fullfilename);
      int line_count = 0;
      if (fin.good()) {
        string header;
        if (!getline(fin, header).good()) {
          throw "Could not read the header from " + fullfilename;
        } else if (header != "<TICKER> <PER> <DATE> <TIME> <OPEN> <HIGH> <LOW> <CLOSE> <VOL>") {
          throw "The header is mismatched in " + fullfilename;
        }
        line_count++;  // The header has been read.
      } else {
        throw "Could not open " + fullfilename;
      }

      const date_period& file_period = get<1>(src);
      boost::optional<ptime> previous_time;

      cout << "Reading " << fullfilename << " " << file_period << "..." << endl;
      while (!fin.eof()) {
        line_count++;
        try {
          // Read line
          string line;
          getline(fin, line);
          if (fin.bad()) {
            throw std::logic_error("Read error!");
          }
          if (line.empty() && fin.eof()) {
            break;
          }
          // It assumes that an empty line is not supported in the source files.
          const fxlib::fxcandle candle = fxlib::MakeFromFinam(line, pair_name);
          // Test parsed candle data
          if (candle.volume == 0) {
            throw std::logic_error("Empty candle volume");
          }
          const boost::gregorian::date curr_date = (candle.time - minutes(1)).date();
          if (curr_date >= file_period.end()) {
            cout << "[WARN] Line:" << line_count << " - extra data that is out of file period " + to_simple_string(file_period) << ". All further data will be skipped!" << endl;
            break;
          }
          time_duration delta;
          if (previous_time.is_initialized()) {
            if (candle.time <= *previous_time) {
              throw std::logic_error("Wrong candle time " + to_simple_string(candle.time) + " that is less or equal previous time " + to_simple_string(*previous_time));
            }
            delta = candle.time - *previous_time;
            const boost::gregorian::date prev_date = (*previous_time - minutes(1)).date();
            if ((delta > minutes(1)) && (curr_date > prev_date)) {
              delta = CalcLongGap(*previous_time, curr_date);
              if (curr_date.day_of_week() != boost::gregorian::Sunday) {
                delta += candle.time - ptime(curr_date);
              }
            }  // if ((delta > minutes(1)) && (candle.time.date() > previous_time->date()))
          } else { // if (previous_time.is_initialized())
            delta = curr_date > file_period.begin() ? CalcLongGap(ptime(file_period.begin()) + minutes(1), curr_date) : minutes(0);
            if (curr_date.day_of_week() != boost::gregorian::Sunday) {
              delta += candle.time - ptime(curr_date);
            }
          }
          if (delta >= tpAllowableGap) {
            cout << "[INFO] Line: " << line_count << " - Gap " << delta << endl;
          }
          previous_time = candle.time;
          const time_period open_period = fxlib::ForexOpenHours(curr_date);
          if (!open_period.contains(candle.time)) {
            cout << "[WARN] Line: " << line_count << " - Candle date-time " << candle.time << " is out of open period " << open_period << endl;
          }

          seq.candles.push_back(candle);
        } catch (const std::exception& e) {
          ostringstream ostr;
          ostr << "Line: " << line_count << " - " << e.what();
          throw ostr.str();
        }
      }  // while !fin.eof()

      if (!previous_time.is_initialized()) {
        throw string("File is empty!");
      }
      time_duration delta = CalcLongGap(*previous_time, file_period.end());
      if (delta >= tpAllowableGap) {
        cout << "[INFO] Line: " << line_count << ". Gap " << delta << endl;
      }
    }  // for src_list

    cout << "It has been read " << seq.candles.size() << " quotes." << endl;
    cout << "Writing " << out_path << "..." << endl;

    //ofstream fout(string_narrow(out_path.c_str()), ofstream::binary);
    //if (!fout.good()) {
    //  throw "Could not open " + string_narrow(out_path.c_str());
    //}

  } catch (const string& e) {
    cout << "[ERROR] " << e << endl;
    return boost::system::errc::io_error;
  }

  return boost::system::errc::success;
}
