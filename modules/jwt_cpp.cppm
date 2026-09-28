/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

// C++20 module interface unit for cpp-jwt.
//
// cpp-jwt's headers include standard library headers (<memory>, <string>,
// ...) textually. Keeping them inside this module's global module fragment
// lets translation units that `import std;` use cpp-jwt without also
// including the standard library textually.
//
// Like the rapidjson module, this works around a clang bug: in C++26 mode,
// a textual <memory> in a translation unit that also imports std fails in
// libstdc++'s <bits/indirect.h>. The bug is still present in clang 23.1 and
// in clang main as of October 2026.

module;

// Use cpp-jwt's bundled nlohmann/json rather than a system one.
#define CPP_JWT_USE_VENDORED_NLOHMANN_JSON
#include <jwt/jwt.hpp>

export module jwt_cpp;

export namespace jwt {
    using jwt::jwt_object;
}

export namespace jwt::params {
    using jwt::params::algorithm;
    using jwt::params::secret;
    using jwt::params::headers;
    using jwt::params::payload;
}
