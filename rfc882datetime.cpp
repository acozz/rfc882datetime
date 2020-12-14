#include <array>
#include <ctime>
#include <regex>

#include "rfc882datetime.h"

namespace rfc882
{
    // Function prototypes
    std::chrono::system_clock::time_point generateUTCTime(const RFC882DateTime::DateTime& date) noexcept;
    [[nodiscard]] bool isValidDate(const RFC882DateTime::DateTime& date) noexcept;
    [[nodiscard]] bool isValidTime(const RFC882DateTime::DateTime& date) noexcept;
    std::chrono::minutes parseLocalDifferential(const std::string& localDifferential);
    int parseMonth(const std::string& month) noexcept;
    std::chrono::minutes parseTimeZone(const std::string& timezone);

    // Algorithm: http://howardhinnant.github.io/date_algorithms.html
    // This is basically what you would see baked into C++20's std::chrono calendar support.
    // So let's not reinvent the wheel. Howard Hinnant designed std::chrono...
    // =====================================================================================
    // Returns number of days since civil 1970-01-01.  Negative values indicate
    //    days prior to 1970-01-01.
    // Preconditions:  y-m-d represents a date in the civil (Gregorian) calendar
    //                 m is in [1, 12]
    //                 d is in [1, last_day_of_month(y, m)]
    //                 y is "approximately" in
    //                   [numeric_limits<Int>::min()/366, numeric_limits<Int>::max()/366]
    //                 Exact range of validity is:
    //                 [civil_from_days(numeric_limits<Int>::min()),
    //                  civil_from_days(numeric_limits<Int>::max()-719468)]
    template <class Int>
    constexpr Int days_from_civil(Int y, unsigned m, unsigned d) noexcept
    {
        static_assert(std::numeric_limits<unsigned>::digits >= 18,
            "This algorithm has not been ported to a 16 bit unsigned integer");
        static_assert(std::numeric_limits<Int>::digits >= 20,
            "This algorithm has not been ported to a 16 bit signed integer");
        y -= m <= 2;
        const Int era = (y >= 0 ? y : y - 399) / 400;
        const unsigned yoe = static_cast<unsigned>(y - era * 400);          // [0, 399]
        const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;// [0, 365]
        const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;         // [0, 146096]
        return era * 146097 + static_cast<Int>(doe) - 719468;
    }

    std::chrono::system_clock::time_point generateUTCTime(const RFC882DateTime::DateTime& date) noexcept
    {
        // In C++17, there's no good built-in way to handle calendars (coming in C++20).
        // Instead, we will use the C-library's Unix time_t and convert that to a std::chrono time_point
        
        // Get number of days from Unix epoch: January 1, 1970
        auto daysFromEpoch = days_from_civil(date.year, date.month, date.day);

        // Convert to a std::time_t value
        std::time_t localizedTime = (((
            (24 * static_cast<std::time_t>(daysFromEpoch) + date.hour) * 60) // convert days/hour to minutes
            + date.minute) * 60) // convert minutes to seconds
            + date.second; // add remaining seconds

        // Then convert that to a std::chrono time_point and then convert to UTC
        return std::chrono::system_clock::from_time_t(localizedTime) - date.timeZoneDifferential;
    }

    bool isValidDate(const RFC882DateTime::DateTime& date) noexcept
    {
        // Make sure we're within calendar bounds
        if(date.day < 1 || date.day > 31 || date.month < 1 || date.month > 12)
            return false;

        // To be a leap year, the year must be divisible by 4 and either of the following cases:
        // 1. Not evenly divisible by 100.
        // 2. Evenly divisible by 100 and 400.
        bool isLeapYear = (date.year % 4 == 0) && ((date.year % 100 != 0) || ((date.year % 100 == 0) && (date.year % 400 == 0)));
        int febDays = isLeapYear ? 29 : 28;

        // Days 1 - 28 (or 29 in a leap year) are always ok
        if(date.day < febDays)
            return true;

        // Some months don't have 31 days
        if(date.day == 31)
            return (date.month != 2 && date.month != 4 && date.month != 6 && date.month != 9 && date.month != 11);

        // There are 29 or 30 days which is only invalid in February
        return (date.month != 2);
    }

    bool isValidTime(const RFC882DateTime::DateTime& date) noexcept
    {
        return 
            (date.hour >= 0 && date.hour <= 23) &&
            (date.minute >= 0 && date.minute <= 59) && 
            (date.second >= 0 && date.second <= 59);
    }

    bool operator<(const RFC882DateTime& x, const RFC882DateTime& y)
    {
        return x.time < y.time;
    }

    bool operator<=(const RFC882DateTime& x, const RFC882DateTime& y)
    {
        return x.time <= y.time;
    }

    bool operator>(const RFC882DateTime& x, const RFC882DateTime& y)
    {
        return x.time > y.time;
    }

    bool operator>=(const RFC882DateTime& x, const RFC882DateTime& y)
    {
        return x.time >= y.time;
    }

    bool operator==(const RFC882DateTime& x, const RFC882DateTime& y)
    {
        return x.time == y.time;
    }

    std::optional<RFC882DateTime> parseDateAndTimeSpec(std::string stamp)
    {
        const std::regex rfc882DateTime{
            /*
             Group1 = Optional day of week (with trailing comma)
             Group2 = Day of month (3 letters)
             Group3 = Month (3 letters)
             Group4 = Year (2 or 4 digits)
             Group5 = Hour (2 digits)
             Group6 = Minute (2 digits)
             Group7 = Optional seconds (2 digits with prepended :)
             Group8 = Time zone (one of Group9 or Group10 are required)
             Group9 = Optional named time zone
             Group10 = Optional local differential
            */
            R"(^(Mon,|Tue,|Wed,|Thu,|Fri,|Sat,|Sun,)?\s*(\d{1,2})\s+(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)\s+(\d{2,4})\s+(\d{2}):(\d{2})(:\d{2})?\s+((UT|GMT|EST|EDT|CST|CDT|MST|MDT|PST|PDT|Z|A|M|N|Y)|((\+|-)(\d{4})))$)"
        };

        if(std::smatch results; std::regex_match(stamp, results, rfc882DateTime))
        {
            // This timestamp is verified to be RFC882 compliant. Now, parse the data into an RFC882DateTime structure.
            RFC882DateTime date;
            date.stamp = std::move(stamp);

            // Gather the tokens and convert them to integers, as necessary.
            // The regex matching guarantees that std::stoi will not fail.
            date.tokens.dayOfWeek = results[1].matched ? std::string{ results[1].first, results[1].second - 1 } : "";

            date.dateTime.day = std::stoi(date.tokens.day = results[2].str());
            date.dateTime.month = parseMonth(date.tokens.month = results[3].str());
            date.dateTime.year = std::stoi(date.tokens.year = results[4].str());
            if(date.dateTime.year < 100)
                date.dateTime.year += 2000; // assume 21st century

            date.dateTime.hour = std::stoi(date.tokens.hour = results[5].str());
            date.dateTime.minute = std::stoi(date.tokens.minute = results[6].str());
            date.tokens.second = results[7].matched ? std::string{ results[7].first + 1, results[7].second } : "";
            date.dateTime.second = (date.tokens.second.size()) ? std::stoi(date.tokens.second) : 0;

            date.dateTime.timeZoneDifferential = parseTimeZone(date.tokens.timeZone = results[8].str());

            // Make sure that the date and time are not out of normal bounds.
            if(!isValidDate(date.dateTime) || !isValidTime(date.dateTime))
                return {};

            // Calculate the time point
            date.time = generateUTCTime(date.dateTime);

            return { date };
        }
        
        // The timestamp is not RFC882 compliant
        return {};
    }

    std::chrono::minutes parseLocalDifferential(const std::string& localDifferential)
    {
        // Precondition: this is a valid local differential of the form (+/-)HHMM
        int diff = std::stoi(localDifferential);
        int hours = diff / 100;
        int minutes = diff - (hours * 100);

        return { std::chrono::hours{hours} + std::chrono::minutes{minutes} };
    }

    int parseMonth(const std::string& month) noexcept
    {
        const std::array<char[4], 12> months{ "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

        for(unsigned int i = 0; i < months.size(); ++i)
            if(months[i] == month)
                return i + 1;
        
        // Not found
        return 0;
    }
    
    std::chrono::minutes parseTimeZone(const std::string& timezone)
    {
        using namespace std::chrono_literals;

        if(!timezone.empty() && (timezone.front() == '+' || timezone.front() == '-'))
            return parseLocalDifferential(timezone);

        if(timezone == "EST")
            return -5h;
        if(timezone == "EDT")
            return -4h;

        if(timezone == "CST")
            return -6h;
        if(timezone == "CDT")
            return -5h;

        if(timezone == "MST")
            return -7h;
        if(timezone == "MDT")
            return -6h;

        if(timezone == "PST")
            return -8h;
        if(timezone == "PDT")
            return -7h;

        if(timezone == "A")
            return -1h;
        if(timezone == "M")
            return -12h;
        if(timezone == "N")
            return 1h;
        if(timezone == "Y")
            return 12h;
        
        // UT/GMT/Z
        return 0h;
    }
}