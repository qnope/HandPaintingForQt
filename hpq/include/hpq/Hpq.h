#pragma once

// All of hpq in one include.
#include <hpq/Export.h>
#include <hpq/HandPaintDevice.h>
#include <hpq/HandPaintEffect.h>
#include <hpq/HandPaintStyle.h>
#include <hpq/PaintCache.h>
#include <hpq/RoughGeometry.h>
#include <hpq/Theme.h>
#include <hpq/Themes.h>
#include <hpq/Theming.h>
#include <hpq/Version.h>

namespace hpq {

// The version of the hpq library loaded at run time, as "major.minor.patch". HPQ_VERSION_STRING
// (hpq/Version.h) is the version that the code was compiled with.
HPQ_EXPORT const char* version();

}  // namespace hpq
