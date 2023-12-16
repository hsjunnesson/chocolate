#pragma once

enum ch_logging_severity {
    CH_LOGGING_SEVERITY_DEBUG,
    CH_LOGGING_SEVERITY_INFO,
    CH_LOGGING_SEVERITY_ERROR,
    CH_LOGGING_SEVERITY_FATAL,
};

void ch_internal_log(enum ch_logging_severity severity, const char *format, ...);

#define ch_log_debug(format, ...) ch_internal_log(CH_LOGGING_SEVERITY_DEBUG, format, ##__VA_ARGS__)
#define ch_log_info(format, ...) ch_internal_log(CH_LOGGING_SEVERITY_INFO, format, ##__VA_ARGS__)
#define ch_log_error(format, ...) ch_internal_log(CH_LOGGING_SEVERITY_ERROR, format, ##__VA_ARGS__)
#define ch_log_fatal(format, ...) ch_internal_log(CH_LOGGING_SEVERITY_FATAL, format, ##__VA_ARGS__)
