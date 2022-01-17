/*
 * Copyright (C) 2015-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#pragma once

#include "api.hh"

namespace gms {

class gossiper;

}

namespace api {

void set_hinted_handoff(http_context& ctx, routes& r, gms::gossiper& g);
void unset_hinted_handoff(http_context& ctx, routes& r);

}
