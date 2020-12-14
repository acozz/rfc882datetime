# rfc882datetime
This is a C++17 library for parsing an "RFC882 Date &amp; Time Specification" string into a std::chrono::time_point.

The RFC882 Date and Time Specification is located here: https://tools.ietf.org/html/rfc822#section-5.1

While the specification calls for a two digit year, this library also allows for a four digit year. A four digit year is preferred in some applications such as RSS feeds (See https://validator.w3.org/feed/docs/rss2.html). All two digit years are assumed to be in the 21st century.

## Usage
rfc882datetime.h uses the rfc882 namespace.

It defines the following function:
```
std::optional<RFC882DateTime> parseDateAndTimeSpec(std::string stamp);
```
Use this function to parse a compatible time stamp string into an RFC882DateTime object. Comparison operators are defined for RFC882DateTime objects.
```
#include "rfc882datetime.h"

...

if(auto time = rfc882::parseDateAndTime("23 Nov 20 09:34:03 -0500")) // returns a std::optional<rfc882::RFC882DateTime>
{
  // Use *time...
}
```
## RFC882DateTime structure definition
```
struct RFC882DateTime
{
    std::string stamp;                              // The RFC882 formatted time stamp, unaltered.
    std::chrono::system_clock::time_point time{};   // The point in time that this time stamp represents, in UTC.

    struct Tokens
    {
        std::string dayOfWeek;                      // optional: Mon, Tue, Wed, Thu, Fri, Sat, Sun
        std::string day;                            // 2 digits
        std::string month;                          // Jan, Feb, Mar, etc...
        std::string year;                           // 2 or 4 digits

        std::string hour;                           // 2 digits
        std::string minute;                         // 2 digits
        std::string second;                         // optional: 2 digits

        std::string timeZone;                       // Limited time zones like EST, EDT, etc. or differential such as -0500.
    } tokens;

    // Note: these values are not adjusted by the time zone differential
    struct DateTime
    {
        int day    = 1;        // day of month [1 - 31] (depending on the month)
        int month  = 1;        // month of year [1 - 12]
        int year   = 1970;     // year (2 digit years are assumed in 21st century)

        int hour   = 0;        // hour [0 - 23]
        int minute = 0;        // minute [0 - 59]
        int second = 0;        // second [0 - 59]

        std::chrono::minutes timeZoneDifferential{};  // Examples: EST = -5 * 60, +1230 = 12 * 60 + 30
    } dateTime;
};
```


## Sample program
```
#include <iostream>
#include <string>
#include "rfc882datetime.h"

int main()
{
  std::string rfc882TimeStamp1 = "23 Nov 20 09:34:03 -0500"; // this is an RFC882 Date and Time
  std::string rfc882TimeStamp2 = "Tue, 7 Oct 2014 10:10:05 PST"; // this is an RFC882 Date and Time with 4-digit year (not to spec, but allowed by this library)

  auto time1 = rfc882::parseDateAndTimeSpec(rfc882TimeStamp1); // returns std::optional<rfc882::RFC882DateTime>
  auto time2 = rfc882::parseDateAndTimeSpec(rfc882TimeStamp2);
  if(time1 && time2)
  {
    std::cout << *time1.stamp << " comes ";
    std::cout << (*time1 <= *time2) ? " before " : " after ";
    std::cout << *time2.stamp << '\n';
  }
  
  return 0;
}
```
Output:
```
23 Nov 20 09:34:03 -0500 comes after Tue, 7 Oct 2014 10:10:05 PST
```
