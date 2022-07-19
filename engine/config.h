#pragma once

struct ini_t;

namespace engine {
namespace config {

/// Reads a property from an optional section into properties.
const char *read_property(ini_t *ini, const char *section, const char *property);

} // namespace config
} // namespace engine
