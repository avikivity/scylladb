/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

// C++20 module interface unit for xxHash.
//
// The build defines XXH_PRIVATE_API, which makes <xxhash.h> define its whole
// implementation (about 7000 lines) as static inline functions in every
// translation unit that includes it; through utils/simple_hashers.hh and
// bytes.hh that is most of them. Static functions can't be exported from a
// module, so this module undefines XXH_PRIVATE_API and exports only the
// declarations of the functions Scylla uses; their definitions come from
// libxxhash, which Scylla links. XXH_STATIC_LINKING_ONLY exposes the
// definitions of the streaming state types, so that callers can keep them by
// value.

module;

#undef XXH_PRIVATE_API
#define XXH_STATIC_LINKING_ONLY
#include <xxhash.h>

export module xxhash;

export {
    using ::XXH_errorcode;
    using ::XXH64_hash_t;
    using ::XXH128_hash_t;

    using ::XXH64;
    using ::XXH64_state_t;
    using ::XXH64_reset;
    using ::XXH64_update;
    using ::XXH64_digest;

    using ::XXH3_state_t;
    using ::XXH3_128bits_withSeed;
    using ::XXH3_128bits_reset_withSeed;
    using ::XXH3_128bits_update;
    using ::XXH3_128bits_digest;
}
