// qc.cpp : Defines the entry point for the console application.
//

#include <string>
#include <iostream>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

int main(int argc, char* argv[])
{
  using namespace std;
  cout << "Forex Quotation Compiler" << endl;
  if (argc < 3 || argc > 4) {
    cout << ">qc {src-dir} {pair-name} [out-file]" << endl;
    cout << "    {out-file} is {pair-name}.bin by default." << endl;
    return 1;
  }
  boost::filesystem::path src_dir = argv[1];
  string pair_name = argv[2];
  boost::algorithm::to_upper(pair_name);
  boost::filesystem::path out_file = pair_name + ".bin";
  if (argc == 4) {
    out_file = argv[3];
  }
  cout << "Compile all [" << pair_name << "]-files in " << src_dir << " to binary file " << out_file << endl;
  cout << "Finding raw quotation files in " << boost::filesystem::canonical(src_dir) << "..." << endl;

  return 0;
}
