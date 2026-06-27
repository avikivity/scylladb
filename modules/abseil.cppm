/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module interface unit for abseil containers used by ScyllaDB.
//
// Wraps the abseil container and hash headers that ScyllaDB uses so
// that consuming translation units can write `import abseil;` instead
// of textual #includes.  This encapsulates abseil's transitive
// standard library includes inside the module, preventing future
// conflicts when `import std;` is introduced.

module;

// Global module fragment: textual headers go here.
#include <absl/container/btree_set.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/node_hash_map.h>
#include <absl/hash/hash.h>

export module abseil;

// Re-export every declaration that was pulled in from absl::
export namespace absl {
    // btree_set.h
    using absl::btree_set;
    using absl::btree_multiset;

    // flat_hash_map.h
    using absl::flat_hash_map;

    // node_hash_map.h
    using absl::node_hash_map;

    // hash.h
    using absl::Hash;

    // Internal types needed for friend declarations in templates
    // instantiated outside this module.
    namespace container_internal::hashtable_debug_internal {
        using absl::container_internal::hashtable_debug_internal::HashtableDebugAccess;
    }
}
