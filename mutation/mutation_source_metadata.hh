/*
 * Copyright (C) 2019-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

#pragma once

import std.compat;

#include "timestamp.hh"

struct mutation_source_metadata {
    std::optional<api::timestamp_type> min_timestamp;
    std::optional<api::timestamp_type> max_timestamp;
};