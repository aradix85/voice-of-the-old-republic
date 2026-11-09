// Single source of truth for the installed mod version. Bumped together
// with manifest.toml [patch].version on every release.
//
// Read by:
//   - core_dllmain.cpp's "loaded, version X" greeting

#pragma once

namespace acc {

constexpr const char* kModVersion = "0.9.1";

}  // namespace acc
