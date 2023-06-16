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
        const int position_;
        const int length_;
        const int distance_;
        const int length_code_;
        const int dist_code_;
    public:
        Match(int position, int length, int distance, int length_code, int dist_code) : position_(position),
                                                                                        length_(length),
                                                                                        distance_(distance),
                                                                                        length_code_(length_code),
                                                                                        dist_code_(dist_code)
        {}

        [[nodiscard]] int length() const
        { return length_; }

        [[nodiscard]] int distance() const
        { return distance_; }

        [[nodiscard]] int position() const
        { return position_; }

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
