#include "engine/log.h"
#include "engine/file.h"

#include <backward.hpp>
#include <iomanip>
#include <memory.h>
#include <sstream>
#include <stdarg.h>
#include <stdio.h>
#include <string_stream.h>
#include <temp_allocator.h>
#include <time.h>
#include <chrono>
#include <ctime>
#include <filesystem>

#if defined(WIN32)
#include <windows.h>
#endif

using namespace foundation;
using namespace foundation::string_stream;
using namespace backward;
namespace fs = std::filesystem;

// TODO: Remake this - queue strings to a buffer, write those to log from a thread.

std::string get_current_date() {
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to tm struct for formatting in UTC
    std::tm now_tm = *std::gmtime(&now_c);

    // Format time to year-month-day (ISO 8601 date format)
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y-%m-%d");

    return ss.str();
}

std::string get_current_timestamp() {
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to tm struct for formatting in UTC
    std::tm now_tm = *std::gmtime(&now_c);

    // Format time to ISO 8601 in UTC (Zulu time)
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%FT%TZ");

    // Optional: Append fractional seconds
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

    return ss.str();
}

void console_log(Buffer &buffer) {
    const char *log_line = c_str(buffer);
    
#if defined(_WIN32)
    if (GetConsoleWindow() != NULL) {
        fprintf(stderr, "%s", log_line);
    }
    
    #if defined(_DEBUG)
    OutputDebugString(log_line);
    #endif
#else
    fprintf(stderr, "%s", log_line);
#endif
}

void file_log(Buffer &buffer) {
    TempAllocator256 ta;
    Buffer log_filename(ta);

#if defined(_WIN32)
    char path[FILENAME_MAX];
    GetModuleFileNameA(NULL, path, FILENAME_MAX);

    fs::path dir_path = fs::path(path).parent_path();

    log_filename << dir_path.string().c_str() << "/";
#endif

    log_filename << "logs/grunka-" << get_current_date().c_str() << ".log";
    engine::file::write(buffer, c_str(log_filename));
}

void internal_log(LoggingSeverity severity, const char *format, ...) {
    TempAllocator1024 ta;
    Buffer log_line(ta);
    
    const char *severity_prefix = nullptr;

    switch (severity) {
    case LoggingSeverity::Debug:
        severity_prefix = " [DEBUG] ";
        break;
    case LoggingSeverity::Info:
        severity_prefix = " [INFO] ";
        break;
    case LoggingSeverity::Error:
        severity_prefix = " [ERROR] ";
        break;
    case LoggingSeverity::Fatal:
        severity_prefix = " [FATAL] ";
        break;
    default:
        return;
    }

    log_line << get_current_timestamp().c_str() << severity_prefix;

#if defined(_WIN32)
    va_list args;
    va_start(args, format);
    log_line = string_stream::vprintf(log_line, format, args);
    va_end(args);
#else
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 1024, format, args);
    log_line << buffer;
    va_end(args);
#endif

    log_line << "\n";
    
    console_log(log_line);
    file_log(log_line);
    
    if (severity == LoggingSeverity::Fatal || severity == LoggingSeverity::Error) {
        // https://github.com/bombela/backward-cpp/issues/206
        TraceResolver _workaround;
        (void)_workaround;

        StackTrace st;
        st.load_here();
        Printer p;
        p.address = true;
        
        std::ostringstream stream;
        p.print(st, stream);

        Buffer buffer(ta);
        buffer << stream.str().c_str();
        
        console_log(buffer);
        file_log(buffer);
    }

    if (severity == LoggingSeverity::Fatal) {
        exit(EXIT_FAILURE);
    }
}
