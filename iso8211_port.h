/******************************************************************************
 *
 * Minimal portability layer for the standalone ISO8211 sample project.
 *
 ****************************************************************************/

#ifndef ISO8211_PORT_H_INCLUDED
#define ISO8211_PORT_H_INCLUDED

#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <string>
#include <vector>

#ifdef _WIN32
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

using ISO8211File = std::FILE;
using ISO8211Offset = std::int64_t;

enum ISO8211ErrorClass
{
    ISO8211ErrorWarning = 2,
    ISO8211ErrorFailure = 3
};

enum ISO8211ErrorCode
{
    ISO8211AppDefined = 1,
    ISO8211OutOfMemory = 2,
    ISO8211FileIO = 3,
    ISO8211OpenFailed = 4,
    ISO8211NotSupported = 5,
    ISO8211AssertionFailed = 6,
    ISO8211DiscardedFormat = 1301
};

inline void ISO8211Error(ISO8211ErrorClass errorClass, int errorCode,
                         const char *format, ...)
{
    std::fprintf(stderr, "ISO8211 %s %d: ",
                 errorClass == ISO8211ErrorWarning ? "warning" : "error",
                 errorCode);

    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
}

inline void ISO8211Debug(const char *module, const char *format, ...)
{
#ifdef ISO8211_ENABLE_DEBUG
    std::fprintf(stderr, "%s: ", module);
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
#else
    (void)module;
    (void)format;
#endif
}

#define ISO8211Assert(expr) assert(expr)

inline double ISO8211Atof(const char *text)
{
    return std::strtod(text, nullptr);
}

inline ISO8211File *ISO8211FOpen(const char *filename, const char *mode)
{
#ifdef _WIN32
    ISO8211File *fp = nullptr;
    return fopen_s(&fp, filename, mode) == 0 ? fp : nullptr;
#else
    return std::fopen(filename, mode);
#endif
}

inline int ISO8211FClose(ISO8211File *fp)
{
    return std::fclose(fp);
}

inline size_t ISO8211FRead(void *buffer, size_t size, size_t count,
                           ISO8211File *fp)
{
    return std::fread(buffer, size, count, fp);
}

inline size_t ISO8211FWrite(const void *buffer, size_t size, size_t count,
                            ISO8211File *fp)
{
    return std::fwrite(buffer, size, count, fp);
}

inline int ISO8211FSeek(ISO8211File *fp, ISO8211Offset offset, int origin)
{
#ifdef _WIN32
    return _fseeki64(fp, offset, origin);
#else
    return fseeko(fp, static_cast<off_t>(offset), origin);
#endif
}

inline ISO8211Offset ISO8211FTell(ISO8211File *fp)
{
#ifdef _WIN32
    return _ftelli64(fp);
#else
    return static_cast<ISO8211Offset>(ftello(fp));
#endif
}

inline int ISO8211FEof(ISO8211File *fp)
{
    return std::feof(fp);
}

inline bool ISO8211IsRegularFile(const char *filename)
{
#ifdef _WIN32
    struct _stat64 st;
    if (_stat64(filename, &st) != 0)
        return false;
    return (st.st_mode & _S_IFDIR) == 0;
#else
    struct stat st;
    if (stat(filename, &st) != 0)
        return false;
    return S_ISREG(st.st_mode);
#endif
}

inline bool ISO8211IsLittleEndian()
{
    const std::uint16_t value = 1;
    return *reinterpret_cast<const std::uint8_t *>(&value) == 1;
}

inline void ISO8211SwapBytes(void *data, size_t size)
{
    auto *bytes = static_cast<std::uint8_t *>(data);
    for (size_t i = 0; i < size / 2; ++i)
    {
        const std::uint8_t tmp = bytes[i];
        bytes[i] = bytes[size - i - 1];
        bytes[size - i - 1] = tmp;
    }
}

inline void ISO8211ToBigEndian(void *data, size_t size)
{
    if (ISO8211IsLittleEndian())
        ISO8211SwapBytes(data, size);
}

inline void ISO8211ToLittleEndian(void *data, size_t size)
{
    if (!ISO8211IsLittleEndian())
        ISO8211SwapBytes(data, size);
}

inline bool ISO8211NeedsByteSwap(char formatChar)
{
    return (ISO8211IsLittleEndian() && formatChar == 'B') ||
           (!ISO8211IsLittleEndian() && formatChar == 'b');
}

inline std::vector<std::string> ISO8211Split(const char *text, char delimiter)
{
    std::vector<std::string> tokens;
    const char *start = text;
    for (const char *cur = text; *cur != '\0'; ++cur)
    {
        if (*cur == delimiter)
        {
            tokens.emplace_back(start, cur - start);
            start = cur + 1;
        }
    }
    tokens.emplace_back(start);
    return tokens;
}

inline bool ISO8211Equal(const char *lhs, const char *rhs)
{
    if (lhs == nullptr || rhs == nullptr)
        return lhs == rhs;

    while (*lhs != '\0' && *rhs != '\0')
    {
        const auto a = static_cast<unsigned char>(*lhs);
        const auto b = static_cast<unsigned char>(*rhs);
        if (std::tolower(a) != std::tolower(b))
            return false;
        ++lhs;
        ++rhs;
    }

    return *lhs == *rhs;
}

#endif
