// qc.cpp : Defines the entry point for the console application.
//

#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

#include "fxlib/fxcurrencies.h"

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
  
  return 0;
}
