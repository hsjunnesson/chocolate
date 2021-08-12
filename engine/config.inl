#pragma once

#include <fstream>
#include <string>

#include <google/protobuf/message.h>
#include <google/protobuf/util/json_util.h>

#include "log.h"

namespace config {

inline void read(const char *filename, google::protobuf::Message *message) {
    std::ifstream ifs(filename);

    if (!ifs.is_open()) {
        log_fatal("Could not find config file %s", filename);
    }

    std::string content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));

    google::protobuf::util::Status status = google::protobuf::util::JsonStringToMessage(content, message);

    if (!status.ok()) {
        log_fatal("Could not read config file %s: %s", filename, status.ToString().c_str());
    }
}

} // namespace config
