#pragma once
#include <ostream>
#include <Windows.h>

//https://learn.microsoft.com/en-us/windows/win32/api/oleauto/nf-oleauto-dosdatetimetovarianttime
namespace MS_DOS
{
    class Time
    {
    public:
        const unsigned char bytes[2];
        explicit Time(const unsigned char bytes[2]) : bytes{ bytes[0], bytes[1] } {}
        explicit Time(const SYSTEMTIME& t);
        Time(unsigned short hour, unsigned short minute, unsigned short second);
        unsigned short get_hour() const;
        unsigned short get_minute() const;
        unsigned short get_second() const;
        Time operator= (const Time& time) const;
        friend std::ostream& operator<<(std::ostream& os, const Time& time);
    };
}
