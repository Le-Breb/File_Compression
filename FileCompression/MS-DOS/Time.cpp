#include "Time.h"

namespace MS_DOS
{
    unsigned short Time::get_second() const
    {
        return (bytes[0] & 0b00011111) * 2;
    }

    Time Time::operator=(const Time& time) const
    {
        return Time{ time.bytes };
    }

    unsigned short Time::get_minute() const
    {
        return (bytes[0] >> 5) + ((bytes[1] & 0b00000111) << 3);
    }

    Time::Time(unsigned short hour, unsigned short minute, unsigned short second) : bytes
        {static_cast<unsigned char>((second / 2) | ((minute & 0b00000111) << 5)),
         static_cast<unsigned char>(((minute & 0b00111000) >> 3) | (hour << 3))}
    { }

    unsigned short Time::get_hour() const
    {
        return bytes[1] >> 3;
    }

    std::ostream& operator<<(std::ostream& os, const Time& time)
    {
        os << time.get_hour() << ':' << time.get_minute() << ':' << time.get_second();

        return os;
    }
}
