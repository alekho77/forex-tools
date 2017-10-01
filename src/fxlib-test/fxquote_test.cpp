#include "fxlib/helpers/fxquote_serializable.h"
#include "fxlib/fxquote.h"

#include <gtest/gtest.h>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/stream_buffer.hpp>

#include <iostream>

namespace fxlib {
using namespace boost::gregorian;
using namespace boost::posix_time;

class fxquote_test_fixture : public ::testing::Test {
protected:
  const fxsequence corr_sequence = {fxperiodicity::minutely, date_period(date(2015, Jan, 1), days(31)), {
                      {ptime(date(2015, Jan, 15), time_duration(15,15,15)), 1.12345, 1.54321, 1.55555, 1.11111, 123},
                      {ptime(date(2015, Jan, 20), time_duration(10,11,22)), 1.54321, 1.12345, 1.55555, 1.11111, 321},
                      {ptime(date(2015, Jan, 25), time_duration(22,33,44)), 1.12345, 1.54321, 1.55555, 1.11111, 123},
                    }};
  const detail::fxsequence_header_bin corr_header = {1, {{0x20, 0x15, 0x01, 0x01, 0xDD, 0x00, 0x00, 0x00}, {0x20, 0x15, 0x02, 0x01, 0xDD, 0x00, 0x00, 0x00}}};
  const detail::fxcandle_bin corr_candle = { {0x20, 0x15, 0x01, 0x15, 0xDD, 0x15, 0x15, 0x15}, 1123450, 1543210, 1555550, 1111110, 123 };
};

TEST_F(fxquote_test_fixture, fxsequence_header_serialize) {
  using namespace boost::iostreams;
  {
    detail::fxsequence_header_bin header{};
    EXPECT_NE(0, std::memcmp(&header, &corr_header, sizeof(detail::fxsequence_header_bin)));
    stream_buffer<array_sink> buf(reinterpret_cast<char*>(&header), sizeof(detail::fxsequence_header_bin));
    std::ostream out(&buf);
    out << corr_header;
    ASSERT_FALSE(out.fail());
    EXPECT_EQ(0, std::memcmp(&header, &corr_header, sizeof(detail::fxsequence_header_bin)));
  }
  {
    detail::fxsequence_header_bin header{};
    EXPECT_NE(0, std::memcmp(&header, &corr_header, sizeof(detail::fxsequence_header_bin)));
    stream_buffer<array_source> buf(reinterpret_cast<const char*>(&corr_header), sizeof(detail::fxsequence_header_bin));
    std::istream in(&buf);
    in >> header;
    ASSERT_FALSE(in.fail());
    EXPECT_EQ(0, std::memcmp(&header, &corr_header, sizeof(detail::fxsequence_header_bin)));
  }
}

TEST_F(fxquote_test_fixture, fxcandle_serialize) {
  using namespace boost::iostreams;
  {
    detail::fxcandle_bin candle{};
    EXPECT_NE(0, std::memcmp(&candle, &corr_candle, sizeof(detail::fxcandle_bin)));
    stream_buffer<array_sink> buf(reinterpret_cast<char*>(&candle), sizeof(detail::fxcandle_bin));
    std::ostream out(&buf);
    out << corr_candle;
    ASSERT_FALSE(out.fail());
    EXPECT_EQ(0, std::memcmp(&candle, &corr_candle, sizeof(detail::fxcandle_bin)));
  }
  {
    detail::fxcandle_bin candle{};
    EXPECT_NE(0, std::memcmp(&candle, &corr_candle, sizeof(detail::fxcandle_bin)));
    stream_buffer<array_source> buf(reinterpret_cast<const char*>(&corr_candle), sizeof(detail::fxcandle_bin));
    std::istream in(&buf);
    in >> candle;
    ASSERT_FALSE(in.fail());
    EXPECT_EQ(0, std::memcmp(&candle, &corr_candle, sizeof(detail::fxcandle_bin)));
  }
}

}  // namespace fxlib