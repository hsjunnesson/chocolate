#pragma once

struct ini_t;

namespace engine {
namespace config {

/// True if config has a property.
bool has_property(ini_t *ini, const char *section, const char *property);

/// Reads a property from an optional section into properties.
const char *read_property(ini_t *ini, const char *section, const char *property);

} // namespace config
} // namespace engine
