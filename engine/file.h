#pragma once

namespace foundation {
template <typename T> struct Array;
}

namespace engine {
namespace file {

bool exist(const char *filename);

bool read(foundation::Array<char> &buffer, const char *filename);

} // namespace file
} // namespace engine
