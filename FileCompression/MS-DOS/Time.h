#pragma once
#include <ostream>

namespace MS_DOS
{
    class Time
    {
        const unsigned char bytes_[2];
    public:
        explicit Time(const unsigned char bytes[2]) : bytes_{ bytes[0], bytes[1] } {}
        Time(unsigned short hour, unsigned short minute, unsigned short second);
        unsigned short get_hour() const;
        unsigned short get_minute() const;
        unsigned short get_second() const;
        Time operator= (const Time& time) const;
        friend std::ostream& operator<<(std::ostream& os, const Time& time);
    };
}
