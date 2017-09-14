#include "fxlib/fxtime.h"

#include <gtest/gtest.h>

#include <sstream>

class fxtime_test_fixture : public ::testing::Test {
protected:
  const fxlib::fxtime correct_time = {{0x20, 0x17, 0x09, 0x14, 0xDD, 0x17, 0x54, 0x31}};
  const std::string correct_time_str = "20170914T175431";

  const fxlib::fxtime invalid_time = {{0x00, 0x00, 0x55, 0x66, 0xDD, 0x77, 0x88, 0x99}};
  const std::string invalid_time_str = "00005566T778899";

  const fxlib::fxtime bad_separator_time = {{0x20, 0x17, 0x09, 0x14, 0xAA, 0x17, 0x54, 0x31}};
  const std::string bad_separator_time_str = std::string("20170914") + "\xAA" + "175431";

  const fxlib::fxtime bad_highdig_time   = {{0x20, 0x17, 0xA0, 0x14, 0xDD, 0x17, 0x54, 0x31}};
  const std::string bad_highdig_time_str = std::string("2017") + char('0' + 10) + "014T175431";

  const fxlib::fxtime bad_lowdig_time    = {{0x20, 0x17, 0x0A, 0x14, 0xDD, 0x17, 0x54, 0x31}};
  const std::string bad_lowdig_time_str = std::string("20170") + char('0' + 10) + "14T175431";
};

TEST_F(fxtime_test_fixture, write_fxtime_to_stream) {
  {
    std::stringstream ss;
    ss << correct_time;
    EXPECT_TRUE(ss.good());
    std::string str;
    ss >> str;
    EXPECT_EQ(correct_time_str, str);
  }
  {
    std::stringstream ss;
    ss << invalid_time;
    EXPECT_TRUE(ss.good());
    std::string str;
    ss >> str;
    EXPECT_EQ(invalid_time_str, str);
  }
  {
    std::stringstream ss;
    ss << bad_separator_time;
    EXPECT_TRUE(ss.fail());
  }
  {
    std::stringstream ss;
    ss << bad_highdig_time;
    EXPECT_TRUE(ss.fail());
  }
  {
    std::stringstream ss;
    ss << bad_lowdig_time;
    EXPECT_TRUE(ss.fail());
  }
}

TEST_F(fxtime_test_fixture, read_fxtime_from_stream) {
  {
    std::stringstream ss;
    ss << correct_time_str;
    fxlib::fxtime time;
    ss >> time;
    EXPECT_FALSE(ss.fail());
    EXPECT_EQ(correct_time.data, time.data);
  }
  {
    std::stringstream ss;
    ss << invalid_time_str;
    fxlib::fxtime time;
    ss >> time;
    EXPECT_FALSE(ss.fail());
    EXPECT_EQ(invalid_time.data, time.data);
  }
  {
    std::stringstream ss;
    ss << bad_separator_time_str;
    fxlib::fxtime time;
    ss >> time;
    EXPECT_TRUE(ss.fail());
  }
  {
    std::stringstream ss;
    ss << bad_highdig_time_str;
    fxlib::fxtime time;
    ss >> time;
    EXPECT_TRUE(ss.fail());
  }
  {
    std::stringstream ss;
    ss << bad_lowdig_time_str;
    fxlib::fxtime time;
    ss >> time;
    EXPECT_TRUE(ss.fail());
  }
}
