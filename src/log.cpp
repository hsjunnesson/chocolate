#include "engine/log.h"

#pragma warning(push, 0)
#include <iomanip>
#include <stdarg.h>
#include <sstream>
#include <stdio.h>
#include <time.h>

#include <backward.hpp>
#include <memory.h>
#include <string_stream.h>
#include <temp_allocator.h>

#if defined(WIN32)
#include <windows.h>
#endif
#pragma warning(pop)

using namespace foundation;
using namespace foundation::string_stream;
using namespace backward;

void internal_log(LoggingSeverity severity, const char *format, ...) {
    TempAllocator1024 ta;
    Buffer ss(ta);

    const char *severity_prefix = nullptr;

    switch (severity) {
    case LoggingSeverity::Debug:
        severity_prefix = "[DEBUG] ";
        break;
    case LoggingSeverity::Info:
        severity_prefix = "[INFO] ";
        break;
    case LoggingSeverity::Error:
        severity_prefix = "[ERROR] ";
        break;
    case LoggingSeverity::Fatal:
        severity_prefix = "[FATAL] ";
        break;
    default:
        return;
    }

    ss << severity_prefix;

#ifdef __APPLE__
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 1024, format, args);
    ss << buffer;
    va_end(args);
#else
    va_list args;
    va_start(args, format);
    ss = string_stream::vprintf(ss, format, args);
    va_end(args);
#endif

    ss << "\n";

    fprintf(stdout, "%s", c_str(ss));

#if defined(_DEBUG) && defined(WIN32)
    OutputDebugString(c_str(ss));
#endif

    if (severity == LoggingSeverity::Fatal || severity == LoggingSeverity::Error) {
        // https://github.com/bombela/backward-cpp/issues/206
        TraceResolver _workaround;
        (void)_workaround;

        StackTrace st;
        st.load_here();
        Printer p;
        p.color_mode = ColorMode::automatic;
        p.address = true;
        p.print(st, stdout);

#if defined(_DEBUG) && defined(WIN32)
        std::ostringstream stream;
        p.print(st, stream);
        stream << c_str(ss);
        OutputDebugString(stream.str().c_str());
#endif
    }

    if (severity == LoggingSeverity::Fatal) {
        exit(EXIT_FAILURE);
    }
}
