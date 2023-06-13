//
// Created by matmu on 28/05/2023.
//

#ifndef FILECOMPRESSION_WINDOW_H
#define FILECOMPRESSION_WINDOW_H

typedef unsigned char Byte;
namespace Deflate
{

    class Window
    {
        Byte data_[0x8000]{};
        int offset_ = 0;
        int size_ = 0;

    public:
        Window() = default;

        ~Window() = default;

        void add(const Byte& c)
        {
            data_[offset_++] = c;
            if (offset_ == 0x8000)
                offset_ = 0;
            if (size_ < 0x8000)
                size_++;

        }

        [[nodiscard]] Byte get(const int offset) const
        {
            const int m = offset_ - offset;
            return m < 0 ? data_[0x8000 + m] : data_[m % 0x8000];
        }

        [[nodiscard]] int size() const
        { return size_; }
    };

} // Deflate

#endif //FILECOMPRESSION_WINDOW_H
