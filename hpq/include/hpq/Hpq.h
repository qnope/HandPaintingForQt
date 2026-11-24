#pragma once

#include <hpq/Export.h>

namespace hpq {

// Placeholder: the name of the library.
HPQ_EXPORT const char* library_name();

// The version of the hpq library loaded at run time, as "major.minor.patch". HPQ_VERSION_STRING
// (hpq/Version.h) is the version that the code was compiled with.
HPQ_EXPORT const char* version();

}  // namespace hpq
