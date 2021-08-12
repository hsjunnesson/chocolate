#include "engine/log.h"

#include <iomanip>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include <backward.hpp>
#include <date/date.h>
#include <memory.h>
#include <string_stream.h>
#include <temp_allocator.h>

using namespace foundation;
using namespace foundation::string_stream;
using namespace date;
using namespace backward;

void internal_log(LoggingSeverity severity, const char *format, ...) {
    TempAllocator1024 ta;
    Buffer ss(ta);

    const char *severity_prefix = nullptr;
    FILE *stream = nullptr;

    switch (severity) {
    case LoggingSeverity::Debug:
        severity_prefix = "[DEBUG] ";
        stream = stdout;
        break;
    case LoggingSeverity::Info:
        severity_prefix = "[INFO] ";
        stream = stdout;
        break;
    case LoggingSeverity::Error:
        severity_prefix = "[ERROR] ";
        stream = stderr;
        break;
    case LoggingSeverity::Fatal:
        severity_prefix = "[FATAL] ";
        stream = stderr;
        break;
    default:
        return;
    }

    // {
    //     auto now = std::chrono::system_clock::now();
    //     auto now_c = std::chrono::system_clock::to_time_t(now);
    //     auto now_tm = *std::localtime(&now_c);

    //     char timebuf[100];
    //     std::strftime(timebuf, sizeof(timebuf), "%FT%T", &now_tm);
    //     ss << timebuf;
    // }

    ss << severity_prefix;

    va_list(args);
    va_start(args, format);
    ss = vprintf(ss, format, args);
    va_end(args);

    ss << "\n";

    fprintf(stream, "%s", c_str(ss));

    if (severity == LoggingSeverity::Fatal || severity == LoggingSeverity::Error) {
        // https://github.com/bombela/backward-cpp/issues/206
        TraceResolver _workaround;
        (void)_workaround;
        
        StackTrace st;
        st.load_here();
        Printer p;
        p.color_mode = ColorMode::always;
        p.address = true;
        p.print(st, stream);
    }

#if defined(_DEBUG)
    fflush(stream);
#endif

    if (severity == LoggingSeverity::Fatal) {
        exit(EXIT_FAILURE);
    }
}
