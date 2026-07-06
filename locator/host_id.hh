/*
 * Copyright (C) 2015-present ScyllaDB
 *
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

#pragma once

#include "utils/UUID.hh"


import std.compat;

namespace locator {

using host_id = utils::tagged_uuid<struct host_id_tag>;
using host_id_or_exception = std::variant<host_id, std::exception_ptr>;
using host_id_or_exception_callback = noncopyable_function<void(host_id_or_exception)>;

}

