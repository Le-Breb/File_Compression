//
// Created by matmu on 28/05/2023.
//

#ifndef FILECOMPRESSION_MATCH_H
#define FILECOMPRESSION_MATCH_H

#include <ostream>

namespace Deflate
{

    class Match
    {
    private:
        int position_;
        int length_;
        int distance_;
    public:
        Match(int position, int length, int distance) : position_(position), length_(length), distance_(distance)
        {}

        [[nodiscard]] int length() const
        { return length_; }

        [[nodiscard]] int distance() const
        { return distance_; }

        [[nodiscard]] int position() const
        { return position_; }

        friend std::ostream& operator<<(std::ostream& os, const Match& match)
        {
            os << "(length: " << match.length_ << ", distance: " << match.distance_ << ")";
            return os;
        }
    };

} // Deflate

#endif //FILECOMPRESSION_MATCH_H
