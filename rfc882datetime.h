#ifndef RFC882DATETIME_H
#define RFC882DATETIME_H

/* 
Parse RFC882 Date and Time Specification
https://tools.ietf.org/html/rfc822#section-5.1
There is one exception: Whereas the spec calls for a two-digit year, 
this library will accept years with four digits, as preferred by RSS feeds.
See https://validator.w3.org/feed/docs/rss2.html
*/

/*
 5.  DATE AND TIME SPECIFICATION

     5.1.  SYNTAX

     date-time   =  [ day "," ] date time        ; dd mm yy
                                                 ;  hh:mm:ss zzz

     day         =  "Mon"  / "Tue" /  "Wed"  / "Thu"
                 /  "Fri"  / "Sat" /  "Sun"

     date        =  1*2DIGIT month 2DIGIT        ; day month year
                                                 ;  e.g. 20 Jun 82

     month       =  "Jan"  /  "Feb" /  "Mar"  /  "Apr"
                 /  "May"  /  "Jun" /  "Jul"  /  "Aug"
                 /  "Sep"  /  "Oct" /  "Nov"  /  "Dec"

     time        =  hour zone                    ; ANSI and Military

     hour        =  2DIGIT ":" 2DIGIT [":" 2DIGIT]
                                                 ; 00:00:00 - 23:59:59

     zone        =  "UT"  / "GMT"                ; Universal Time
                                                 ; North American : UT
                 /  "EST" / "EDT"                ;  Eastern:  - 5/ - 4
                 /  "CST" / "CDT"                ;  Central:  - 6/ - 5
                 /  "MST" / "MDT"                ;  Mountain: - 7/ - 6
                 /  "PST" / "PDT"                ;  Pacific:  - 8/ - 7
                 /  1ALPHA                       ; Military: Z = UT;
                                                 ;  A:-1; (J not used)
                                                 ;  M:-12; N:+1; Y:+12
                 / ( ("+" / "-") 4DIGIT )        ; Local differential
                                                 ;  hours+min. (HHMM)

     5.2.  SEMANTICS

          If included, day-of-week must be the day implied by the date
     specification.

          Time zone may be indicated in several ways.  "UT" is Univer-
     sal  Time  (formerly called "Greenwich Mean Time"); "GMT" is per-
     mitted as a reference to Universal Time.  The  military  standard
     uses  a  single  character for each zone.  "Z" is Universal Time.
     "A" indicates one hour earlier, and "M" indicates 12  hours  ear-
     lier;  "N"  is  one  hour  later, and "Y" is 12 hours later.  The
     letter "J" is not used.  The other remaining two forms are  taken
     from ANSI standard X3.51-1975.  One allows explicit indication of
     the amount of offset from UT; the other uses  common  3-character
     strings for indicating time zones in North America.
 */

#include <chrono>
#include <optional>
#include <string>

namespace rfc882
{
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
            int year   = 1970;     // year (2 digit years are added to 2000)

            int hour   = 0;        // hour [0 - 23]
            int minute = 0;        // minute [0 - 59]
            int second = 0;        // second [0 - 59]

            std::chrono::minutes timeZoneDifferential{};  // Examples: EST = -5 * 60, +1230 = 12 * 60 + 30
        } dateTime;
    };

    // Take an RFC882 Date and Time and try to parse it into an RFC882DateTime structure.
    std::optional<RFC882DateTime> parseDateAndTimeSpec(std::string stamp);

    // Comparison operators. In C++20, these can be replaced by overloading <=>.
    inline bool operator<(const RFC882DateTime& x, const RFC882DateTime& y)
    {
        return x.time < y.time;
    }

    inline bool operator<=(const RFC882DateTime& x, const RFC882DateTime& y)
    {
        return x.time <= y.time;
    }

    inline bool operator>(const RFC882DateTime& x, const RFC882DateTime& y)
    {
        return x.time > y.time;
    }

    inline bool operator>=(const RFC882DateTime& x, const RFC882DateTime& y)
    {
        return x.time >= y.time;
    }

    inline bool operator==(const RFC882DateTime& x, const RFC882DateTime& y)
    {
        return x.time == y.time;
    }
}

#endif
