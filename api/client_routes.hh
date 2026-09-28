/*
 * Copyright (C) 2025-present ScyllaDB
 *
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */
#pragma once

#include "api/api_init.hh"

namespace api {

void set_client_routes(http_context& ctx, httpd::routes& r, sharded<service::client_routes_service>& cr);
void unset_client_routes(http_context& ctx, httpd::routes& r);

}
