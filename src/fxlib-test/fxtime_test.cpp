
#include "fxlib/helpers/fxtime_conversion.h"
#include "fxlib/helpers/fxtime_serializable.h"

#include <gtest/gtest.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#include <sstream>
#include <iostream>

namespace fxlib {
using std::string;
using std::stringstream;
using conversion::from_iso_string;
using conversion::to_iso_string;
using conversion::try_from_iso_string;
using conversion::try_to_iso_string;

class fxtime_test_fixture : public ::testing::Test {
protected:
  const fxtime correct_fxtime = {{0x20, 0x17, 0x09, 0x14, 0xDD, 0x17, 0x54, 0x31}};
  const string correct_time_str = "20170914T175431";
  const boost::posix_time::ptime correct_ptime = { {2017, boost::date_time::Sep, 14}, {17, 54, 31} };
  const uint64_t correct_time_int = 0x20170914DD175431ui64;

  const fxtime invalid_fxtime = {{0x00, 0x00, 0x55, 0x66, 0xDD, 0x77, 0x88, 0x99}};
  const string invalid_time_str = "00005566T778899";
  const boost::posix_time::ptime invalid_ptime = { boost::date_time::not_a_date_time };

  const fxtime bad_separator_time = {{0x20, 0x17, 0x09, 0x14, 0xAA, 0x17, 0x54, 0x31}};
  const string bad_separator_time_str = std::string("20170914") + "\xAA" + "175431";

  const fxtime bad_highdig_time   = {{0x20, 0x17, 0xA0, 0x14, 0xDD, 0x17, 0x54, 0x31}};
  const string bad_highdig_time_str = std::string("2017") + char('0' + 10) + "014T175431";

  const fxtime bad_lowdig_time    = {{0x20, 0x17, 0x0A, 0x14, 0xDD, 0x17, 0x54, 0x31}};
  const string bad_lowdig_time_str = std::string("20170") + char('0' + 10) + "14T175431";
};

TEST_F(fxtime_test_fixture, write_fxtime_to_stream) {
  {
    stringstream ss;
    ss << correct_fxtime;
    EXPECT_TRUE(ss.good());
    string str;
    ss >> str;
    EXPECT_EQ(correct_time_str, str);
  }
  {
    stringstream ss;
    ss << invalid_fxtime;
    EXPECT_TRUE(ss.good());
    string str;
    ss >> str;
    EXPECT_EQ(invalid_time_str, str);
  }
  {
    stringstream ss;
    ss << bad_separator_time;
    EXPECT_TRUE(ss.fail());
  }
  {
    stringstream ss;
    ss << bad_highdig_time;
    EXPECT_TRUE(ss.fail());
  }
  {
    stringstream ss;
    ss << bad_lowdig_time;
    EXPECT_TRUE(ss.fail());
  }
}

TEST_F(fxtime_test_fixture, read_fxtime_from_stream) {
  {
    stringstream ss;
    ss << correct_time_str;
    fxtime time;
    ss >> time;
    EXPECT_FALSE(ss.fail());
    EXPECT_EQ(correct_fxtime.data, time.data);
  }
  {
    stringstream ss;
    ss << invalid_time_str;
    fxtime time;
    ss >> time;
    EXPECT_FALSE(ss.fail());
    EXPECT_EQ(invalid_fxtime.data, time.data);
  }
  {
    stringstream ss;
    ss << bad_separator_time_str;
    fxtime time;
    ss >> time;
    EXPECT_TRUE(ss.fail());
  }
  {
    stringstream ss;
    ss << bad_highdig_time_str;
    fxtime time;
    ss >> time;
    EXPECT_TRUE(ss.fail());
  }
  {
    stringstream ss;
    ss << bad_lowdig_time_str;
    fxtime time;
    ss >> time;
    EXPECT_TRUE(ss.fail());
  }
}

TEST_F(fxtime_test_fixture, fxtime_cast_to_ptime) {
  {
    boost::posix_time::ptime time;
    EXPECT_NO_THROW(time = boost::posix_time::from_iso_string(to_iso_string(correct_fxtime)));
    EXPECT_EQ(correct_ptime, time);
  }
  {
    string str;
    EXPECT_NO_THROW(str = to_iso_string(invalid_fxtime));
    EXPECT_THROW(boost::posix_time::from_iso_string(str), boost::exception);
  }
  {
    EXPECT_THROW(to_iso_string(bad_separator_time), std::bad_cast);
    EXPECT_THROW(to_iso_string(bad_highdig_time), std::bad_cast);
    EXPECT_THROW(to_iso_string(bad_lowdig_time), std::bad_cast);
  }
}

TEST_F(fxtime_test_fixture, fxtime_try_cast_to_ptime) {
  {
    boost::posix_time::ptime time;
    string str;
    EXPECT_TRUE(try_to_iso_string(correct_fxtime, str));
    EXPECT_NO_THROW(time = boost::posix_time::from_iso_string(str));
    EXPECT_EQ(correct_ptime, time);
  }
  {
    string str;
    EXPECT_TRUE(try_to_iso_string(invalid_fxtime, str));
    EXPECT_THROW(boost::posix_time::from_iso_string(str), boost::exception);
  }
  {
    string str;
    EXPECT_FALSE(try_to_iso_string(bad_separator_time, str));
    EXPECT_FALSE(try_to_iso_string(bad_highdig_time, str));
    EXPECT_FALSE(try_to_iso_string(bad_lowdig_time, str));
  }
}

TEST_F(fxtime_test_fixture, ptime_cast_to_fxtime) {
  {
    fxtime time;
    EXPECT_NO_THROW(time = from_iso_string(boost::posix_time::to_iso_string(correct_ptime)));
    EXPECT_EQ(correct_fxtime.data, time.data);
  }
  {
    string str;
    EXPECT_NO_THROW(str = boost::posix_time::to_iso_string(invalid_ptime));
    EXPECT_THROW(from_iso_string(str), std::bad_cast);
  }
}

TEST_F(fxtime_test_fixture, ptime_try_cast_to_fxtime) {
  {
    fxtime time;
    string str;
    EXPECT_NO_THROW(str = boost::posix_time::to_iso_string(correct_ptime));
    EXPECT_TRUE(try_from_iso_string(str, time));
    EXPECT_EQ(correct_fxtime.data, time.data);
  }
  {
    fxtime time;
    string str;
    EXPECT_NO_THROW(str = boost::posix_time::to_iso_string(invalid_ptime));
    EXPECT_FALSE(try_from_iso_string(str, time));
  }
}

TEST_F(fxtime_test_fixture, fxtime_serialization) {
  using namespace boost::iostreams;
  {
    fxtime time{};
    EXPECT_NE(0, std::memcmp(time.data.data(), correct_fxtime.data.data(), time.data.size()));
    stream_buffer<array_sink> buf(reinterpret_cast<char*>(time.data.data()), time.data.size());
    std::ostream out(&buf);
    out << correct_fxtime.data;
    ASSERT_FALSE(out.fail());
    EXPECT_EQ(0, std::memcmp(time.data.data(), correct_fxtime.data.data(), time.data.size()));
  }
  {
    fxtime time{};
    EXPECT_NE(0, std::memcmp(time.data.data(), correct_fxtime.data.data(), time.data.size()));
    stream_buffer<array_source> buf(reinterpret_cast<const char*>(correct_fxtime.data.data()), correct_fxtime.data.size());
    std::istream in(&buf);
    in >> time.data;
    ASSERT_FALSE(in.fail());
    EXPECT_EQ(0, std::memcmp(time.data.data(), correct_fxtime.data.data(), time.data.size()));
  }
}

}  // namespace fxlib
