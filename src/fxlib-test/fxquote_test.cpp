#include "fxlib/helpers/fxquote_serializable.h"
#include "fxlib/fxquote.h"

#include <gtest/gtest.h>

#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#pragma warning(push)
#pragma warning(disable:4702)  // warning C4702: unreachable code
#include <boost/iostreams/stream.hpp>
#pragma warning(pop)

#include <vector>

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
  const detail::fxsequence_header_bin test_header = {1, 1234, {{0x20, 0x15, 0x01, 0x01, 0xDD, 0xFF, 0x00, 0x00}, {0x20, 0x15, 0x02, 0x01, 0xDD, 0x00, 0x00, 0x00}}};
  const detail::fxcandle_bin test_candle = { {0x20, 0x15, 0x01, 0x15, 0xDD, 0x15, 0x15, 0xFF}, 1123450, 1543210, 1555550, 1111110, 123 };
};

TEST_F(fxquote_test_fixture, fxsequence_header_serialize) {
  using namespace boost::iostreams;
  {
    detail::fxsequence_header_bin header{};
    EXPECT_NE(0, std::memcmp(&header, &test_header, sizeof(detail::fxsequence_header_bin)));
    stream<array_sink> out(reinterpret_cast<char*>(&header), sizeof(detail::fxsequence_header_bin));
    out << test_header;
    ASSERT_FALSE(out.fail());
    EXPECT_EQ(0, std::memcmp(&header, &test_header, sizeof(detail::fxsequence_header_bin)));
  }
  {
    detail::fxsequence_header_bin header{};
    EXPECT_NE(0, std::memcmp(&header, &test_header, sizeof(detail::fxsequence_header_bin)));
    stream<array_source> in(reinterpret_cast<const char*>(&test_header), sizeof(detail::fxsequence_header_bin));
    in >> header;
    ASSERT_FALSE(in.fail());
    EXPECT_EQ(0, std::memcmp(&header, &test_header, sizeof(detail::fxsequence_header_bin)));
  }
}

TEST_F(fxquote_test_fixture, fxcandle_serialize) {
  using namespace boost::iostreams;
  {
    detail::fxcandle_bin candle{};
    EXPECT_NE(0, std::memcmp(&candle, &test_candle, sizeof(detail::fxcandle_bin)));
    stream<array_sink> out(reinterpret_cast<char*>(&candle), sizeof(detail::fxcandle_bin));
    out << test_candle;
    ASSERT_FALSE(out.fail());
    EXPECT_EQ(0, std::memcmp(&candle, &test_candle, sizeof(detail::fxcandle_bin)));
  }
  {
    detail::fxcandle_bin candle{};
    EXPECT_NE(0, std::memcmp(&candle, &test_candle, sizeof(detail::fxcandle_bin)));
    stream<array_source> in(reinterpret_cast<const char*>(&test_candle), sizeof(detail::fxcandle_bin));
    in >> candle;
    ASSERT_FALSE(in.fail());
    EXPECT_EQ(0, std::memcmp(&candle, &test_candle, sizeof(detail::fxcandle_bin)));
  }
}

TEST_F(fxquote_test_fixture, fxsequence_serialize) {
  using namespace boost::iostreams;
  using buf_type = std::vector<char>;
  using dev_type = back_insert_device<buf_type>;
  buf_type buf;
  {
    dev_type sink{buf};
    stream<dev_type> out(sink);
    ASSERT_NO_THROW(WriteSequence(out, corr_sequence));
    ASSERT_FALSE(out.fail());
    ASSERT_EQ(sizeof(detail::fxsequence_header_bin) + corr_sequence.candles.size() * sizeof(detail::fxcandle_bin), buf.size());
  }
  {
    fxsequence seq = {fxperiodicity::tick, date_period(date(not_a_date_time), date(not_a_date_time)), {}};
    stream<array_source> in(&*buf.begin(), buf.size());
    ASSERT_NO_THROW(seq = ReadSequence(in));
    ASSERT_FALSE(in.fail());
    EXPECT_EQ(corr_sequence.periodicity, seq.periodicity);
    EXPECT_EQ(corr_sequence.period, seq.period);
    ASSERT_EQ(corr_sequence.candles.size(), seq.candles.size());
    for (size_t i = 0; i < corr_sequence.candles.size(); i++) {
      EXPECT_EQ(corr_sequence.candles[i].time, seq.candles[i].time);
      EXPECT_DOUBLE_EQ(corr_sequence.candles[i].open, seq.candles[i].open);
      EXPECT_DOUBLE_EQ(corr_sequence.candles[i].close, seq.candles[i].close);
      EXPECT_DOUBLE_EQ(corr_sequence.candles[i].high, seq.candles[i].high);
      EXPECT_DOUBLE_EQ(corr_sequence.candles[i].low, seq.candles[i].low);
      EXPECT_EQ(corr_sequence.candles[i].volume, seq.candles[i].volume);
    }
  }
}

}  // namespace fxlib