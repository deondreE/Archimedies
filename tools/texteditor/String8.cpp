#include "String8.hpp"

UnicodeResult utf8_to_utf16(String8 src, u16* dst, s64 dst_max) {
    UnicodeResult res = {0, 0, false};
    s64 i = 0;
    s64 j = 0;

    while (i < src.size && j < dst_max) {
        unsigned char c = (unsigned char)src.str[i];
        unsigned int cp = 0;
        int extra = 0;

        // Manual UTF-8 Decoding
        if (c <= 0x7F) { cp = c; }
        else if ((c & 0xE0) == 0xC0) { cp = c & 0x1F; extra = 1; }
        else if ((c & 0xF0) == 0xE0) { cp = c & 0x0F; extra = 2; }
        else if ((c & 0xF8) == 0xF0) { cp = c & 0x07; extra = 3; }
        else { res.error = true; break; }

        if (i + extra >= src.size) break;
        for (int k = 0; k < extra; ++k) {
            cp = (cp << 6) | (src.str[++i] & 0x3F);
        }
        i++;

        // UTF-16 Encoding
        if (cp <= 0xFFFF) {
            dst[j++] = (u16)cp;
        } else {
            if (j + 1 >= dst_max) break;
            cp -= 0x10000;
            dst[j++] = (u16)((cp >> 10) + 0xD800);
            dst[j++] = (u16)((cp & 0x3FF) + 0xDC00);
        }
    }
    res.processed_size = i;
    res.written_size = j;
    return res;
}