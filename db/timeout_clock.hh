/*
 * Copyright (C) 2017-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

#pragma once

import seastar;

namespace db {
using timeout_clock = seastar::lowres_clock;
using timeout_semaphore = seastar::basic_semaphore<seastar::default_timeout_exception_factory, timeout_clock>;
using timeout_semaphore_units = seastar::semaphore_units<seastar::default_timeout_exception_factory, timeout_clock>;
static constexpr timeout_clock::time_point no_timeout = timeout_clock::time_point::max();
}
