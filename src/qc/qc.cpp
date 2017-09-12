// qc.cpp : Defines the entry point for the console application.
//

#include "fxlib/fxcurrencies.h"
#include "fxlib/fxtime.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <string>
#include <iostream>

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

  cout << "Compile all [" << pair_name << "]-files in " << src_path << " to binary file " << out_path << endl;

  const boost::regex fmask("^" + pair_name + "_([0-9]{6})_([0-9]{6})\\.txt$");
  for (auto entry : boost::make_iterator_range(boost::filesystem::directory_iterator(src_path), {})) {
    boost::smatch what;
    if (boost::filesystem::is_regular_file(entry)) {
      const string filename = string_narrow({entry.path().filename().c_str()});
      if (boost::regex_match(filename, what, fmask)) {
        const boost::gregorian::date_period period(boost::gregorian::from_undelimited_string("20" + what[1].str()),
                                                   boost::gregorian::from_undelimited_string("20" + what[2].str()) + boost::gregorian::date_duration(1));
        cout << "Compiling " << filename << " " << period;

        cout << endl;
      }
    }
  }

  
  return 0;
}
