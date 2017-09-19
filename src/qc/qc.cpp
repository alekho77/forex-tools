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
    cout << ">qc {src-dir} {pair-name} [out-file]" << endl;
    cout << "    {out-file} is {pair-name}.bin by default." << endl;
    return boost::system::errc::invalid_argument;
  }

  boost::system::error_code ec;
  boost::filesystem::path src_path = boost::filesystem::canonical({argv[1]}, ec);
  if (ec) {
    cout << "Source directory \"" << argv[1] << "\" has not found" << endl;
    return ec.value();
  }
  
  string pair_name = argv[2];
  if (!fxlib::IsPair(pair_name)) {
    cout << "The pair name " << argv[2] << " is not valid";
    return boost::system::errc::invalid_argument;
  }
  boost::algorithm::to_upper(pair_name);

  boost::filesystem::path out_file = pair_name + ".bin";
  if (argc == 4) {
    out_file = argv[3];
  }
  boost::filesystem::path out_path = boost::filesystem::canonical(out_file.parent_path(), ec);
  if (ec) {
    cout << "Destination directory \"" << out_file << "\" has not found" << endl;
    return ec.value();
  }
  out_path.append(out_file.filename().c_str());
  if (boost::filesystem::exists(out_path)) {
    // TODO: add hash checking to be sure that really nothing to do.
    cout << "The binary file " << out_path << " already exists. Nothing to do." << endl;
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
    cout << "No one source file has been found!" << endl;
    return boost::system::errc::no_such_file_or_directory;
  }

  sort(begin(src_list), end(src_list), [](const auto& a, const auto& b) { return get<1>(a) < get<1>(b); });
  
  boost::gregorian::date_period total_period = get<1>(src_list[0]);
  for (size_t i = 1; i < src_list.size(); i++) {
    if (total_period.is_adjacent(get<1>(src_list[i]))) {
      total_period = total_period.span(get<1>(src_list[i]));
    } else {
      cout << "There is a gap between " << get<0>(src_list[i - 1]) << " and " << get<0>(src_list[i]) << endl;
      return boost::system::errc::argument_out_of_domain;
    }
  }
  cout << "Found " << src_list.size() << " source files with total period " << total_period << endl;

  fxlib::fxsequence seq = {fxlib::fxperiodicity::minutely, total_period, {}};
  seq.candles.reserve(total_period.length().days() * 24 * 60);
  for (boost::gregorian::day_iterator ditr = {total_period.begin()}; ditr < total_period.end(); ++ditr) {
    cout << '\r' << "Open period on " << *ditr;
    const boost::posix_time::time_period open_period = fxlib::ForexOpenHours(*ditr);
    cout << " has " << (open_period.length().total_seconds() / 60) << " minutes";
    for (boost::posix_time::time_iterator titr = {open_period.begin(), boost::posix_time::minutes(1)}; titr < open_period.end(); ++titr) {
      seq.candles.push_back({*titr, 0, 0, 0, 0});
    }
  }
  cout << endl << "Expected total minute candles " << seq.candles.size() << endl;

  return boost::system::errc::success;
}
