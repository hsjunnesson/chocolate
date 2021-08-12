#pragma once

enum class LoggingSeverity {
    Debug,
    Info,
    Error,
    Fatal,
};

void internal_log(LoggingSeverity severity, const char *format, ...);

#define log_debug(format, ...) internal_log(LoggingSeverity::Debug, format, __VA_ARGS__)
#define log_info(format, ...) internal_log(LoggingSeverity::Info, format, __VA_ARGS__)
#define log_error(format, ...) internal_log(LoggingSeverity::Error, format, __VA_ARGS__)
#define log_fatal(format, ...) internal_log(LoggingSeverity::Fatal, format, __VA_ARGS__)
