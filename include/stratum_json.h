#ifndef STRATUM_JSON_H
#define STRATUM_JSON_H

#include "stratum_actor.h"

namespace stratum_json {
    bool stratum_handle_method(Job &job, Global_state_info &global_state_info, const char *s);
}

#endif
