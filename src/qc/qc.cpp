// qc.cpp : Defines the entry point for the console application.
//

#include "fxlib/fxlib.h"

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

template <typename From, typename To>
To str_convert(const From& str) {
  To result;
  boost::filesystem::path_traits::convert(str.c_str(), result);
  return result;
}

std::wstring string_widen(const std::string& str) {
  return str_convert<std::string, std::wstring>(str);
}

std::string string_narrow(const std::wstring& str) {
  return str_convert<std::wstring, std::string>(str);
}

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
  if (boost::filesystem::exists(out_path)) {
    // TODO: add hash checking to be sure that really nothing to do.
    cout << "[NOTE] The binary file " << out_path << " already exists. Nothing to do." << endl;
    return boost::system::errc::success;
  }

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
    cout << '\r' << "On date " << *ditr;
    const boost::posix_time::time_period open_period = fxlib::ForexOpenHours(*ditr);
    min_count += open_period.length().total_seconds() / 60;
    cout << " found total " << min_count << " minutes";
  }
  seq.candles.reserve(min_count);
  cout << endl;

  try {
    ofstream fout(string_narrow(out_path.c_str()), ofstream::binary);
    if (!fout.good()) {
      throw "Could not open " + string_narrow(out_path.c_str());
    }
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

      const boost::gregorian::date_period& fperiod = get<1>(src);
      boost::gregorian::day_iterator ditr{fperiod.begin()};
      const boost::posix_time::time_period open_period = fxlib::ForexOpenHours(*ditr);
      boost::posix_time::time_iterator titr{open_period.begin(), boost::posix_time::minutes(1)};

      cout << "Reading " << fullfilename << " " << fperiod << "..." << endl;
      while (!fin.eof()) {
        line_count++;
        string line;
        getline(fin, line);
        if (fin.bad()) {
          ostringstream ostr;
          ostr << fullfilename << " : line " << line_count << ". Read error!";
          throw ostr.str();
        }
        boost::smatch what;
        if (!boost::regex_match(line, what, fxlib::FinamExportFormat)) {  // It assumes that an empty line is not supported in the source files.
          ostringstream ostr;
          ostr << fullfilename << " : line " << line_count << ". The line is not matched Finam export format.";
          throw ostr.str();
        }


      }  // while !fin.eof()


//       for ( ditr < fperiod.end(); ++ditr) {
//         int gap_count = 0;
//         for (; titr < open_period.end(); ++titr) {
//           if (fin.eof()) {
//             gap_count++;
//           } else {
//             // ...
//           }  // if fin.eof
//              //seq.candles.push_back({*titr, 0, 0, 0, 0});
//         }  // for time_iterator
//       }  // for day_iterator

    }  // for src_list
  } catch (const string& e) {
    cout << "[ERROR] " << e << endl;
    return boost::system::errc::io_error;
  }

  return boost::system::errc::success;
}
