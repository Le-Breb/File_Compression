//
// Created by matmu on 28/05/2023.
//

#ifndef FILECOMPRESSION_MATCH_H
#define FILECOMPRESSION_MATCH_H

#include <ostream>

typedef unsigned char Byte;
namespace Deflate
{

    class Match
    {
    private:
        const int val_;
        const int length_;
        const int distance_;
        const int length_code_;
        const int dist_code_;
    public:
        explicit Match(Byte val) : val_(val),
                                   length_(0),
                                   distance_(0),
                                   length_code_(0),
                                   dist_code_(0)
        {}

        Match(int val, int length, int distance, int length_code, int dist_code) : val_(val),
                                                                                   length_(length),
                                                                                   distance_(distance),
                                                                                   length_code_(length_code),
                                                                                   dist_code_(dist_code)
        {}

        [[nodiscard]] int length() const
        { return length_; }

        [[nodiscard]] int distance() const
        { return distance_; }

        [[nodiscard]] int val() const
        { return val_; }

        [[nodiscard]] int length_code() const
        { return length_code_; }

        [[nodiscard]] int dist_code() const
        { return dist_code_; }

        friend std::ostream& operator<<(std::ostream& os, const Match& match)
        {
            os << "(length: " << match.length_ << ", distance: " << match.distance_ << ")";
            return os;
        }
    };

} // Deflate

#endif //FILECOMPRESSION_MATCH_H
