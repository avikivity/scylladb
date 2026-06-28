/*
 * Copyright (C) 2025-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

#pragma once


// When passed to sharded<>::start(), translates to "val" passed to sharded instances on shard 0 and T{} on other shards.
template <typename T>
auto only_on_shard0(T val) {
    return sharded_parameter([val] {
        if (seastar::this_shard_id() == 0) {
            return val;
        }
        return T{};
    });
}
