/*
 * Copyright 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

#pragma once

// rapidjson configuration, shared by the rapidjson module (which compiles
// rapidjson with it) and by code that uses rapidjson's configuration macros
// directly. It must not include any header, so that it can be included
// after `import std;`.

namespace rjson {

// Thrown instead of rapidjson's default assert(): assert() can be turned off
// with -DNDEBUG, and it crashes the program. Throws rjson::error.
[[noreturn]] void throw_assertion_failure(const char* condition);

}

#define RAPIDJSON_HAS_STDSTRING 1
#define RAPIDJSON_ASSERT(x) do { if (!(x)) ::rjson::throw_assertion_failure(#x); } while (0)
// This macro is used for functions which are called for every json char making it
// quite costly if not inlined, by default rapidjson only enables it if NDEBUG
// is defined which isn't the case for us.
#define RAPIDJSON_FORCEINLINE __attribute__((always_inline))
