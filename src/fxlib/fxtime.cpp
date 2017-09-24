#include "fxtime.h"

namespace fxlib {

using boost::posix_time::time_duration;
using boost::posix_time::time_period;
using boost::posix_time::hours;
using boost::posix_time::minutes;
using boost::posix_time::ptime;

// The Forex Market trading local hours ("wall clock").
//static const time_duration fxSessionOpen {hours(8)};
//static const time_duration fxSessionClose{hours(16)};

//struct fx_time_zone {
//  time_duration winter;
//  time_duration summer;
//};

/** Fraction of Time Zone Database

"ID","STD ABBR","STD NAME","DST ABBR","DST NAME","GMT offset","DST adjustment","DST Start Date rule","Start time","DST End date rule","End time"
"Australia/Sydney","EST","EST","EST","EST","+10:00:00","+01:00:00","1;0;10","+02:00:00","1;0;4","+03:00:00"
"Asia/Tokyo","JST","JST","","","+09:00:00","+00:00:00","","","","+00:00:00"
"Europe/London","GMT","GMT","BST","BST","+00:00:00","+01:00:00","-1;0;3","+01:00:00","-1;0;10","+02:00:00"
"America/New_York","EST","Eastern Standard Time","EDT","Eastern Daylight Time","-05:00:00","+01:00:00","2;0;3","+02:00:00","1;0;11","+02:00:00"
"Europe/Moscow","MSK","MSK","MSD","MSD","+03:00:00","+01:00:00","-1;0;3","+02:00:00","-1;0;10","+03:00:00"
*/

//static const fx_time_zone fxMarketZones[] = {
//  {hours(9), hours(10)},    // Sydney
//  {hours(9), hours(9)},     // Tokyo
//  {hours(0), hours(1)},     // London
//  {hours(-5), hours(-4)},   // New York
//};

//bool IsForexWeekday(const boost::gregorian::date& date) noexcept {
//  using namespace boost::gregorian;
//  return date.day_of_week() >= Monday && date.day_of_week() <= Friday;
//}

bool IsGMTDFST(const boost::gregorian::date& date) {
  using namespace boost::gregorian;
  day_iterator ditr_dst_start{{date.year(), Mar, 31}};
  while (ditr_dst_start->day_of_week() != Sunday) {
    --ditr_dst_start;
  }
  day_iterator ditr_dst_end{{date.year(), Oct, 31}};
  while (ditr_dst_end->day_of_week() != Sunday) {
    --ditr_dst_end;
  }
  return date >= *ditr_dst_start && date < *ditr_dst_end;
}

time_period ForexOpenHours(const boost::gregorian::date& date) noexcept {
  using namespace boost::gregorian;
  switch (date.day_of_week()) {
    case Sunday:
      if (IsGMTDFST(date)) {
        return time_period(ptime(date, date.month() == Oct ? time_duration(22,1,0) : time_duration(23,1,0)), ptime(date, time_duration(24,1,0)));
      }
      return time_period(ptime(date, time_duration(21,1,0)), ptime(date, time_duration(24,1,0)));
    case Monday:
    case Tuesday:
    case Wednesday:
    case Thursday:
    case Friday:
      return time_period(ptime(date, minutes(1)), hours(24));
    case Saturday:
      return time_period(ptime(date, minutes(1)), IsGMTDFST(date) ? time_duration(2,15,0) : time_duration(3,15,0));  // Sometimes we have small time lag for last quotations on Sat
    default:
      break;
  }
  return time_period(ptime(date), hours(0));
}

}  // namespace fxlib
