#pragma once
#include "app.hpp"

namespace spire {
struct TextBuffer {
    char* p;
    size_t cap, n;
    const Content* locale = nullptr;
    void ch(char v) {
        if (n + 1 < cap) {
            p[n++] = v;
            p[n] = 0;
        }
    }
    void text(const char* s) {
        if (locale)
            s = locale->ui_text(s);
        while (*s)
            ch(*s++);
    }
    void number(int v) {
        uint32_t u = v < 0 ? uint32_t(-(v + 1)) + 1 : uint32_t(v);
        if (v < 0)
            ch('-');
        char b[12];
        int n = 0;
        do {
            b[n++] = (char)('0' + u % 10);
            u /= 10;
        } while (u && n < 12);
        while (n)
            ch(b[--n]);
    }
};
} // namespace spire
