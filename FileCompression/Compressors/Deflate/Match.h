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
        int val_;
        int length_;
        int distance_;
        int length_code_;
        int dist_code_;
    public:

        Match(const Match& match)
        {
            val_ = match.val_;
            length_ = match.length_;
            distance_ = match.distance_;
            length_code_ = match.length_code_;
            dist_code_ = match.dist_code_;
        }

        explicit Match(Byte val) : val_(val),
                                   length_(0),
                                   distance_(0),
                                   length_code_(0),
                                   dist_code_(0)
        {}

        /** \brief The only use of this constructor is to allow the creation of the symbols buffer in Memory */
        Match() : val_(0),
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
