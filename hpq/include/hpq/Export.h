#pragma once

#include <QtGlobal>

// HPQ_EXPORT marks what the hpq library shows to the code that uses it: its public classes
// and functions. Everything else stays inside the library.
// HPQ_STATIC is defined where hpq is a static library (Qt for WebAssembly): nothing is
// exported or imported there. HPQ_BUILDING_LIBRARY is defined only while hpq itself is built.
#if defined(HPQ_STATIC)
#  define HPQ_EXPORT
#elif defined(HPQ_BUILDING_LIBRARY)
#  define HPQ_EXPORT Q_DECL_EXPORT
#else
#  define HPQ_EXPORT Q_DECL_IMPORT
#endif
