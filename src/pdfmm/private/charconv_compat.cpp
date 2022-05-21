#include "charconv_compat.h"

#ifdef WANT_TO_CHARS

#include <locale.h>

#if _WIN32

// In Windows snprintf_l is available as _snprintf_l
// https://docs.microsoft.com/it-it/cpp/c-runtime-library/reference/snprintf-s-snprintf-s-l-snwprintf-s-snwprintf-s-l?view=msvc-170

#define snprintf_l _snprintf_l
#define locale_t _locale_t
#define newlocale(category_mask, locale, base) _create_locale(category_mask, locale)
#define freelocale _free_locale

#endif // _WIN32

struct Locale
{
    Locale()
    {
        Handle = newlocale(LC_ALL, "C", NULL);
    }
    ~Locale()
    {
        freelocale(Handle);
    }

    locale_t Handle;
};

Locale s_locale;

namespace std
{
    to_chars_result to_chars(char* first, char* last, double value,
        std::chars_format fmt, unsigned char precision)
    {
        (void)fmt;
        int rc = snprintf_l(first, last - first + 1, "%.*f", s_locale.Handle, precision, value);
        return { first + (rc < 0 ? 0 : rc), rc < 0 ? errc::value_too_large : errc{ } };
    }

    to_chars_result to_chars(char* first, char* last, float value,
        std::chars_format fmt, unsigned char precision)
    {
        (void)fmt;
        int rc = snprintf_l(first, last - first + 1, "%.*f", s_locale.Handle, precision, value);
        return { first + (rc < 0 ? 0 : rc), rc < 0 ? errc::value_too_large : errc{ } };
    }
}

#endif // WANT_TO_CHARS