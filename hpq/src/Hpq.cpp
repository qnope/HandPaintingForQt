#include <hpq/Hpq.h>
#include <hpq/Version.h>

namespace hpq {

const char* library_name() { return "hpq"; }

const char* version() { return HPQ_VERSION_STRING; }

}  // namespace hpq
