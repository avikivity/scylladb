// Copyright (C) 2024-present ScyllaDB
// SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1

#pragma once

import std.compat;

namespace db {

enum class auth_version_t: int64_t {
    v1 = 1,
    v2 = 2,
};

}
