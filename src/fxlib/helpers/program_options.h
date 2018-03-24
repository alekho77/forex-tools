#pragma once

#include <vector>
#include <iostream>

#pragma warning(push)
#pragma warning(disable : 4505)  // warning C4505: unreferenced local function has been removed
#include <boost/program_options.hpp>
#pragma warning(pop)

using boost::program_options::bool_switch;
using boost::program_options::command_line_parser;
using boost::program_options::error;
using boost::program_options::notify;
using boost::program_options::options_description;
using boost::program_options::parse_command_line;
using boost::program_options::store;
using boost::program_options::value;
using boost::program_options::variables_map;

static inline std::ostream& operator<<(std::ostream& out, const std::vector<options_description>& opts) {
    using namespace std;
    options_description desc;
    for (const auto& opt : opts) {
        desc.add(opt);
    }
    out << endl << "Command line options:" << endl << desc << endl;
    return out;
}
