#pragma once

#include <ostream>
#include <Windows.h>

//https://learn.microsoft.com/en-us/windows/win32/api/oleauto/nf-oleauto-dosdatetimetovarianttime
typedef unsigned char Byte;
namespace MS_DOS
{
    class Time
    {
    public:
        const Byte bytes[2];

        explicit Time(const Byte bytes[2]) : bytes{bytes[0], bytes[1]}
        {}

        explicit Time(const SYSTEMTIME& t);

        Time(unsigned short hour, unsigned short minute, unsigned short second);

        [[nodiscard]] unsigned short get_hour() const;

        [[nodiscard]] unsigned short get_minute() const;

        [[nodiscard]] unsigned short get_second() const;

        Time operator=(const Time& time) const;

        friend std::ostream& operator<<(std::ostream& os, const Time& time);
    };
}
