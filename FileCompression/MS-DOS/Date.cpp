#include "Date.h"

namespace MS_DOS
{
    Date::Date(const SYSTEMTIME& t) : Date(t.wYear, t.wMonth, t.wDay)
    {
    }

    Date::Date(unsigned short year, unsigned short month, unsigned short day) : bytes_
        {static_cast<unsigned char>((day & 0b00011111) | ((month & 0b00001111) << 5)),
         static_cast<unsigned char>(((year - 1980) << 1) | ((month & 0b00001000) >> 3))}
    { }

    unsigned short Date::get_year() const
    {
        return (bytes_[1] >> 1) + 1980;
    }

    unsigned short Date::get_month() const
    {
        return ((bytes_[1] & 0b00000001) << 3) + (bytes_[0] >> 5);
    }

    unsigned short Date::get_day() const
    {
        return bytes_[0] & 0b00011111;
    }

    Date Date::operator=(const Date& date) const
    {
        return Date{ date.bytes_ };
    }

    std::ostream& operator<<(std::ostream& os, const Date& date)
    {
        os << date.get_year() << '-' << date.get_month() << '-' << date.get_day();
        
        return os;
    }
}

