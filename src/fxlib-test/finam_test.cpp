#include "fxlib/finam/finam.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>
#include <utility>

using fxlib::detail::FinamExportFormat;

class finam_test_fixture : public ::testing::Test {
protected:
  const std::vector<std::string> ex_names = {{"TICKER"},{"PER"},{"DATE"},{"TIME"},{"OPEN"},{"HIGH"},{"LOW"},{"CLOSE"},{"VOL"}};
  const std::vector<std::pair<std::string, std::vector<std::string>>> valid_finam_lines = {
    {"USDJPY 1 170622 0631 111.0840000 111.1100000 111.0440000 111.1070000 711", {{"USDJPY"},{"1"},{"170622"},{"0631"},{"111.0840000"},{"111.1100000"},{"111.0440000"},{"111.1070000"},{"711"}}},
    {"EURJPY 1 010903 1544 107.7700000 107.8800000 107.7500000 107.8500000 0", {{"EURJPY"},{"1"},{"010903"},{"1544"},{"107.7700000"},{"107.8800000"},{"107.7500000"},{"107.8500000"},{"0"}}},
    {"ABCXYZ\t123   140915 \t \t \t 1403 1.233 1. 1.2 0.2280000 \r\n 00000111122223333", {{"ABCXYZ"},{"123"},{"140915"},{"1403"},{"1.233"},{"1."},{"1.2"},{"0.2280000"},{"00000111122223333"}}}
    };
  const std::vector<std::string> invalid_finam_lines = {
    {"usdjpy 1 170622 0631 111.0840000 111.1100000 111.0440000 111.1070000 711"},
    {" USDJPY 1 170622 0631 111.0840000 111.1100000 111.0440000 111.1070000 711"},
    {"USDJPY 1 170622 0631 111.0840000 111.1100000 111.0440000 111.1070000 711 "},
    {"USJPY 1 170622 0631 111.0840000 111.1100000 111.0440000 111.1070000 711"},
    {"USDJPY  170622 0631 111.0840000 111.1100000 111.0440000 111.1070000 711"},
    {"USDJPY x 170622 0631 111.0840000 111.1100000 111.0440000 111.1070000 711"},
    {"USDJPY 1 17022 0631 111.0840000 111.1100000 111.0440000 111.1070000 711"},
    {"USDJPY 1 170622 061 111.0840000 111.1100000 111.0440000 111.1070000 711"},
    {"USDJPY 1 170622 0631 1110840000 111.1100000 111.0440000 111.1070000 711"},
    {"USDJPY 1 170622 0631 111. 0840000 111.1100000 111.0440000 111.1070000 711"},
    {"USDJPY 1 170622 0631 111.0840000 .1100000 111.0440000 111.1070000 711"},
    {"USDJPY 1 170622 0631 111.0840000 111.110x000 111.0440000 111.1070000 711"},
    {"USDJPY 1 170622 0631 111.0840000 111.1100000 111.0440000 111.1070000"},
  };
};

TEST_F(finam_test_fixture, check_match) {
  for (const auto& line: valid_finam_lines) {
    EXPECT_TRUE(boost::regex_match(line.first, FinamExportFormat));
  }
}

TEST_F(finam_test_fixture, check_no_match) {
  for (const auto& line : invalid_finam_lines) {
    EXPECT_FALSE(boost::regex_match(line, FinamExportFormat));
  }
}

TEST_F(finam_test_fixture, check_parsing) {
  for (const auto& line : valid_finam_lines) {
    boost::smatch what;
    ASSERT_TRUE(boost::regex_match(line.first, what, FinamExportFormat));
    ASSERT_EQ(ex_names.size() + 1, what.size());
    for (int i = 0; i < ex_names.size(); i++) {
      EXPECT_EQ(line.second[i], what[i + 1]);
      EXPECT_EQ(line.second[i], what[ex_names[i]]);
    }
  }
}
