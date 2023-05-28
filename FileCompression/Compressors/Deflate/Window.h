//
// Created by matmu on 28/05/2023.
//

#ifndef FILECOMPRESSION_WINDOW_H
#define FILECOMPRESSION_WINDOW_H

namespace Deflate {

    class Window {
        char data_[0x8000]{};
        int offset_ = 0;
        int size_ = 0;

    public:
        Window() = default;
        ~Window() = default;
        void add(const char& c) {
            data_[offset_++] = c;
            if (offset_ == 0x8000)
                offset_ = 0;
            if (size_ < 0x8000)
                size_++;

        }

        char get(const int offset) const {
            return data_[(offset_ - offset) % 0x8000];
        }

        int size() const { return size_; }
    };

} // Deflate

#endif //FILECOMPRESSION_WINDOW_H
