
#include "fxlib/fxlib.h"

#include <gtest/gtest.h>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <sstream>

class fxtime_test_fixture : public ::testing::Test {
protected:
  const fxlib::fxtime correct_fxtime = {{0x20, 0x17, 0x09, 0x14, 0xDD, 0x17, 0x54, 0x31}};
  const std::string correct_time_str = "20170914T175431";
  const boost::posix_time::ptime correct_ptime = { {2017, boost::date_time::Sep, 14}, {17, 54, 31} };
  const uint64_t correct_time_int = 0x20170914DD175431ui64;

  const fxlib::fxtime invalid_fxtime = {{0x00, 0x00, 0x55, 0x66, 0xDD, 0x77, 0x88, 0x99}};
  const std::string invalid_time_str = "00005566T778899";
  const boost::posix_time::ptime invalid_ptime = { boost::date_time::not_a_date_time };

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
    ss << correct_fxtime;
    EXPECT_TRUE(ss.good());
    std::string str;
    ss >> str;
    EXPECT_EQ(correct_time_str, str);
  }
  {
    std::stringstream ss;
    ss << invalid_fxtime;
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
    EXPECT_EQ(correct_fxtime.data, time.data);
  }
  {
    std::stringstream ss;
    ss << invalid_time_str;
    fxlib::fxtime time;
    ss >> time;
    EXPECT_FALSE(ss.fail());
    EXPECT_EQ(invalid_fxtime.data, time.data);
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

TEST_F(fxtime_test_fixture, fxtime_cast_to_ptime) {
  {
    boost::posix_time::ptime time;
    EXPECT_NO_THROW(time = boost::posix_time::from_iso_string(fxlib::to_iso_string(correct_fxtime)));
    EXPECT_EQ(correct_ptime, time);
  }
  {
    std::string str;
    EXPECT_NO_THROW(str = fxlib::to_iso_string(invalid_fxtime));
    EXPECT_THROW(boost::posix_time::from_iso_string(str), boost::exception);
  }
  {
    EXPECT_THROW(fxlib::to_iso_string(bad_separator_time), std::bad_cast);
    EXPECT_THROW(fxlib::to_iso_string(bad_highdig_time), std::bad_cast);
    EXPECT_THROW(fxlib::to_iso_string(bad_lowdig_time), std::bad_cast);
  }
}

TEST_F(fxtime_test_fixture, fxtime_try_cast_to_ptime) {
  {
    boost::posix_time::ptime time;
    std::string str;
    EXPECT_TRUE(fxlib::try_to_iso_string(correct_fxtime, str));
    EXPECT_NO_THROW(time = boost::posix_time::from_iso_string(str));
    EXPECT_EQ(correct_ptime, time);
  }
  {
    std::string str;
    EXPECT_TRUE(fxlib::try_to_iso_string(invalid_fxtime, str));
    EXPECT_THROW(boost::posix_time::from_iso_string(str), boost::exception);
  }
  {
    std::string str;
    EXPECT_FALSE(fxlib::try_to_iso_string(bad_separator_time, str));
    EXPECT_FALSE(fxlib::try_to_iso_string(bad_highdig_time, str));
    EXPECT_FALSE(fxlib::try_to_iso_string(bad_lowdig_time, str));
  }
}

TEST_F(fxtime_test_fixture, ptime_cast_to_fxtime) {
  {
    fxlib::fxtime time;
    EXPECT_NO_THROW(time = fxlib::from_iso_string(boost::posix_time::to_iso_string(correct_ptime)));
    EXPECT_EQ(correct_fxtime.data, time.data);
  }
  {
    std::string str;
    EXPECT_NO_THROW(str = boost::posix_time::to_iso_string(invalid_ptime));
    EXPECT_THROW(fxlib::from_iso_string(str), std::bad_cast);
  }
}

TEST_F(fxtime_test_fixture, ptime_try_cast_to_fxtime) {
  {
    fxlib::fxtime time;
    std::string str;
    EXPECT_NO_THROW(str = boost::posix_time::to_iso_string(correct_ptime));
    EXPECT_TRUE(fxlib::try_from_iso_string(str, time));
    EXPECT_EQ(correct_fxtime.data, time.data);
  }
  {
    fxlib::fxtime time;
    std::string str;
    EXPECT_NO_THROW(str = boost::posix_time::to_iso_string(invalid_ptime));
    EXPECT_FALSE(fxlib::try_from_iso_string(str, time));
  }
}

TEST_F(fxtime_test_fixture, cast_to_int) {
  EXPECT_EQ(correct_time_int, correct_fxtime.to_int());
}
