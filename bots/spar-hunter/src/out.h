// Cheap buffered stdout: iostream formatting costs ~1.3k fuel per int and ~4k per float (measured),
// so the probe formats numbers itself and writes with one fwrite per flush.
#pragma once
#include <cambc.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace out {
inline char buf[8192];
inline int n = 0;

inline void flush() {
    if (n) {
        std::fwrite(buf, 1, static_cast<std::size_t>(n), stdout);
        n = 0;
    }
}
inline void raw(const char* p, int len) {
    if (n + len > static_cast<int>(sizeof(buf))) flush();
    std::memcpy(buf + n, p, static_cast<std::size_t>(len));
    n += len;
}
inline void s(const char* str) { raw(str, static_cast<int>(std::strlen(str))); }
inline void s(const std::string& str) { raw(str.data(), static_cast<int>(str.size())); }
inline void i(long long v) {
    char t[24];
    int k = 0;
    const bool neg = v < 0;
    unsigned long long u = neg ? 0ull - static_cast<unsigned long long>(v) : static_cast<unsigned long long>(v);
    if (u <= 0xFFFFFFFFull) { // 32-bit division is native in wasm32; 64-bit goes through a libcall
        auto u32 = static_cast<std::uint32_t>(u);
        do { t[k++] = static_cast<char>('0' + u32 % 10); u32 /= 10; } while (u32);
    } else {
        do { t[k++] = static_cast<char>('0' + u % 10); u /= 10; } while (u);
    }
    if (neg) t[k++] = '-';
    char r[24];
    for (int j = 0; j < k; ++j) r[j] = t[k - 1 - j];
    raw(r, k);
}
// Two decimals, trailing zeros trimmed.
inline void f(double v) {
    if (v < 0) { s("-"); v = -v; }
    const auto scaled = static_cast<std::uint32_t>(v * 100.0 + 0.5);
    i(scaled / 100);
    const std::uint32_t frac = scaled % 100;
    if (frac == 0) return;
    s(".");
    if (frac % 10 == 0) { i(frac / 10); return; }
    if (frac < 10) s("0");
    i(frac);
}
inline void v(cambc::vec2 p) { s("("); i(p.x); s(","); i(p.y); s(")"); }
inline void nl() { s("\n"); }
// key=value helpers
inline void kv(const char* key, long long value) { s(" "); s(key); s("="); i(value); }
inline void kvf(const char* key, double value) { s(" "); s(key); s("="); f(value); }
inline void kvs(const char* key, const std::string& value) { s(" "); s(key); s("="); s(value); }
} // namespace out
