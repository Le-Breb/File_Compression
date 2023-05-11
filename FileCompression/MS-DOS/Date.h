#pragma once
#include <ostream>

namespace MS_DOS
{
    class Date
    {
    public:
        const unsigned char bytes_[2];
        explicit Date(const unsigned char bytes[2]) : bytes_{ bytes[0], bytes[1] } {}
        Date(unsigned short year, unsigned short month, unsigned short day);
        unsigned short get_year() const;
        unsigned short get_month() const;
        unsigned short get_day() const;
        Date operator= (const Date& date) const;
        friend std::ostream& operator<<(std::ostream& os, const Date& date);
    };
}
