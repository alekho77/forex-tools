// qc.cpp : Defines the entry point for the console application.
//

#include "fxlib/fxlib.h"
#include "fxlib/finam/finam.h"
#include "fxlib/helpers/string_conversion.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

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

using boost::posix_time::to_simple_string;
using boost::gregorian::to_simple_string;

static const boost::posix_time::minutes tpAllowableGap{15};

int main(int argc, char* argv[])
{
  using namespace std;
  cout << "Forex Quotation Compiler" << endl;
  if (argc < 3 || argc > 4) {
    cout << "[NOTE] Parameters are required" << endl;
    cout << ">qc {src-dir} {pair-name} [out-file]" << endl;
    cout << "    {out-file} is {pair-name}.bin by default." << endl;
    return boost::system::errc::invalid_argument;
  }

  boost::system::error_code ec;
  boost::filesystem::path src_path = boost::filesystem::canonical({argv[1]}, ec);
  if (ec) {
    cout << "[ERROR] Source directory \"" << argv[1] << "\" has not found" << endl;
    return ec.value();
  }
  
  string pair_name = argv[2];
  if (!fxlib::IsPair(pair_name)) {
    cout << "[ERROR] The pair name " << argv[2] << " is not valid";
    return boost::system::errc::invalid_argument;
  }
  boost::algorithm::to_upper(pair_name);

  boost::filesystem::path out_file = pair_name + ".bin";
  if (argc == 4) {
    out_file = argv[3];
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

  vector<tuple<boost::filesystem::path, boost::gregorian::date_period>> src_list;
  
  const boost::regex fmask("^" + pair_name + "_([0-9]{6})_([0-9]{6})\\.txt$");
  for (auto entry : boost::make_iterator_range(boost::filesystem::directory_iterator(src_path), {})) {
    boost::smatch what;
    if (boost::filesystem::is_regular_file(entry)) {
      const string filename = string_narrow({entry.path().filename().c_str()});
      if (boost::regex_match(filename, what, fmask)) {
        const boost::gregorian::date_period period(boost::gregorian::from_undelimited_string("20" + what[1].str()),
                                                   boost::gregorian::from_undelimited_string("20" + what[2].str()) + boost::gregorian::date_duration(1));
        src_list.push_back(make_tuple(entry.path(), period));
      }
    }
  }

  if (src_list.empty()) {
    cout << "[ERROR] No one source file has been found!" << endl;
    return boost::system::errc::no_such_file_or_directory;
  }

  sort(begin(src_list), end(src_list), [](const auto& a, const auto& b) { return get<1>(a) < get<1>(b); });
  
  boost::gregorian::date_period total_period = get<1>(src_list[0]);
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
    const boost::posix_time::time_period open_period = fxlib::ForexOpenHours(*ditr);
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

      const boost::gregorian::date_period& file_period = get<1>(src);
      boost::gregorian::day_iterator ditr{file_period.begin()};
      boost::posix_time::time_period open_period = fxlib::ForexOpenHours(*ditr);
      boost::posix_time::time_iterator titr{open_period.begin(), boost::posix_time::minutes(1)};

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
          if (ditr >= file_period.end()) {
            throw std::logic_error("Found extra data that is out of file period " + to_simple_string(file_period));
          }
          // It assumes that an empty line is not supported in the source files.
          const fxlib::fxcandle candle = fxlib::MakeFromFinam(line, pair_name);
          // Test parsed candle data
          if (candle.time != *titr) {
            if (candle.time < *titr) {
              throw std::logic_error("Wrong candle time " + to_simple_string(candle.time) + " that is less than expected time " + to_simple_string(*titr));
            }
            boost::posix_time::time_duration tdelta;
            if (candle.time < open_period.end()) {
              tdelta = candle.time - *titr;
            } else {
              tdelta = open_period.end() - *titr;
              for (++ditr; ditr < candle.time.date(); ++ditr) {
                tdelta += fxlib::ForexOpenHours(*ditr).length();
              }
              if (ditr >= file_period.end()) {
                throw std::logic_error("Candle date " + to_simple_string(candle.time.date()) + " is out of file period " + to_simple_string(file_period));
              }
              open_period = fxlib::ForexOpenHours(*ditr);
              if (candle.time < open_period.begin() || candle.time >= open_period.end()) {
                throw std::logic_error("Candle date-time " + to_simple_string(candle.time) + " is out of open period " + to_simple_string(open_period));
              }
              tdelta += candle.time - open_period.begin();
            }
            //tdelta += boost::posix_time::minutes(1);
            if (tdelta >= tpAllowableGap) {
              cout << "[WARN] Line: " << line_count << ". Gap " << tdelta << endl;
            }
            titr = {candle.time, boost::posix_time::minutes(1)};
          }  // if (candle.time != *titr)
        } catch (const std::exception& e) {
          ostringstream ostr;
          ostr << "Line: " << line_count << " - " << e.what();
          throw ostr.str();
        }
        // Corect expected date-time for new line
        ++titr;
        if (titr >= open_period.end()) {
          do {
            ++ditr;
            open_period = fxlib::ForexOpenHours(*ditr);
          } while (open_period.is_null());
          titr = {open_period.begin(), boost::posix_time::minutes(1)};
        }
      }  // while !fin.eof()

      // Check possible gap at the end of the file.
      boost::posix_time::time_duration tdelta = open_period.end() - *titr;
      for (; ditr < file_period.end(); ++ditr) {
        tdelta += fxlib::ForexOpenHours(*ditr).length();
      }
      if (tdelta >= tpAllowableGap) {
        cout << "[WARN] Line: " << line_count << ". Gap " << tdelta << endl;
      }

    }  // for src_list

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
