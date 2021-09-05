#include "engine/file.h"
#include "engine/log.h"

#include <array.h>
#include <memory.h>
#include <string_stream.h>
#include <temp_allocator.h>

// clang-format off
#if defined(_WIN32)
#include <windows.h>
#include <fileapi.h>
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <sys/stat.h>
#endif
// clang-format on

namespace engine {
namespace file {
using namespace foundation;
using namespace foundation::array;

// https://wiki.sei.cmu.edu/confluence/display/c/FIO19-C.+Do+not+use+fseek%28%29+and+ftell%28%29+to+compute+the+size+of+a+regular+file
bool read(string_stream::Buffer &buffer, const char *filename) {
#if defined(_WIN32)
    LARGE_INTEGER file_size;

    HANDLE file = CreateFile(TEXT(filename), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE == file) {
        log_error("Could not read file %s: invalid file handle", filename);
        return false;
    }

    if (!GetFileSizeEx(file, &file_size)) {
        log_error("Could not read file %s: could not get size", filename);
        return false;
    }

    resize(buffer, (uint32_t)file_size.QuadPart);

    if (!ReadFile(file, begin(buffer), (uint32_t)file_size.QuadPart, NULL, NULL)) {
        log_error("Could not read file %s", filename);
        return false;
    }

    if (!CloseHandle(file)) {
        log_error("Could not close file after read %s", filename);
        return false;
    }

    return true;
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    struct stat info;
    if (stat(filename, &info) != 0) {
        log_error("Could not read file %s", filename);
        return false;
    }

    resize(buffer, info.st_size);

    FILE *file = fopen(filename, "rb");
    if (!file) {
        log_error("Could not read file %s", filename);
        return false;
    }

    fread(begin(buffer), info.st_size, 1, file);

    if (ferror(file) != 0) {
        log_error("Could not read file %s", filename);
        return false;
    }

    fclose(file);

    return true;
#else
    log_error("Unsupported platform");
    return false;
#endif
}

} // namespace file
} // namespace engine
