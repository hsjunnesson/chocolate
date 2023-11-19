#include "engine/file.h"
#include "engine/log.h"

#include <array.h>
#include <memory.h>
#include <string_stream.h>
#include <temp_allocator.h>
#include <filesystem>

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
namespace fs = std::filesystem;

bool exist(const char *filename) {
#if defined(_WIN32)
    DWORD dwAttrib = GetFileAttributes(filename);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    struct stat buf;
    return (stat(filename, &buf) == 0);
#else
    log_fatal("Unsupported platform");
    return false;
#endif
}

bool write(foundation::Array<char> &buffer, const char *filename) {
    using namespace string_stream;
    
    fs::path dir_path = fs::path(filename).parent_path();
    if (!fs::exists(dir_path)) {
        fs::create_directories(dir_path);
    }

#if defined(_WIN32)
    HANDLE file = CreateFile(TEXT(filename), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (INVALID_HANDLE_VALUE == file) {
        log_error("Could not open file %s for writing: invalid file handle", filename);
        return false;
    }
    
    SetFilePointer(file, 0, NULL, FILE_END);
    
    DWORD bytes_written = 0;
    BOOL err_flag = WriteFile(file, c_str(buffer), array::size(buffer), &bytes_written, NULL);
    
    if (FALSE == err_flag) {
        log_error("Error writing to file %s", filename);
        CloseHandle(file);
        return false;
    }
    
    if (bytes_written != array::size(buffer)) {
        log_error("Error writing to file %s, could not write entire buffer.", filename);
        CloseHandle(file);
        return false;
    }
    
    CloseHandle(file);
    return true;
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    // TODO: implement
    log_fatal("Unsupported platform");
    return false;
#else
    log_fatal("Unsupported platform");
    return false;
#endif
}

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
        fclose(file);
        return false;
    }

    fclose(file);

    return true;
#else
    log_fatal("Unsupported platform");
    return false;
#endif
}

} // namespace file
} // namespace engine
