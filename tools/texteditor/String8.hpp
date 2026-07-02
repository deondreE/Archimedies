typedef signed long long s64;
typedef unsigned short u16;

#include <format>
#include <print>

struct String8 {
  const char *str; // High performance usually implies immutable slices
  s64 size;

  constexpr String8() : str(nullptr), size(0) {}
  constexpr String8(const char *data, s64 len) : str(data), size(len) {}

  template <unsigned long long N>
  constexpr String8(const char (&literal)[N])
      : str(literal), size((s64)N - 1) {}

  template <s64 N>
  String8(const char (&literal)[N]) : str(literal), size((s64)N - 1) {}

  static s64 calculate_length(const char *s) {
    if (!s)
      return 0;
    s64 res = 0;
    while (s[res])
      res++;
    return res;
  }
  String8(const char *c_str) : str(c_str), size(calculate_length(c_str)) {}

  constexpr String8 substring(s64 start, s64 len) const {
    s64 new_start = (start < 0) ? 0 : (start > size ? size : start);
    s64 new_len = (new_start + len > size) ? (size - new_start) : len;
    return String8(str + new_start, new_len);
  }

  constexpr String8 skip(s64 n) const { return substring(n, size - n); }
  constexpr String8 prefix(s64 n) const { return substring(0, n); }
  constexpr String8 suffix(s64 n) const { return substring(size - n, n); }

  bool operator==(const String8 other) const {
    if (size != other.size)
      return false;
    if (str == other.str)
      return true; // Pointer identity check
    for (s64 i = 0; i < size; ++i) {
      if (str[i] != other.str[i])
        return false;
    }
    return true;
  }

#ifdef INTEROP_STD_VIEW
  operator std::string_view() const { return {str, (unsigned long long)size}; }
#endif

};

template <> struct std::formatter<String8> : std::formatter<std::string_view> {
  auto format(String8 s, format_context &ctx) const {
    return std::formatter<std::string_view>::format(
        std::string_view(s.str, (size_t)s.size), ctx);
  }
};
struct String16 {
  const u16 *str;
  s64 size;

  String16() : str(nullptr), size(0) {}
  String16(const u16 *data, s64 len) : str(data), size(len) {}
};

struct UnicodeResult {
  s64 processed_size;
  s64 written_size;
  bool error;
};

UnicodeResult utf8_to_utf16(const String8 &input, String16 &output);
