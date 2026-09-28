/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

// C++20 module interface unit for Crypto++.
//
// Crypto++'s headers include standard library headers (<memory>, <string>,
// ...) textually. Keeping them inside this module's global module fragment
// lets translation units that `import std;` use Crypto++ without also
// including the standard library textually.
//
// Like the rapidjson module, this works around a clang bug: in C++26 mode,
// a textual <memory> in a translation unit that also imports std fails in
// libstdc++'s <bits/indirect.h>. The bug is still present in clang 23.1 and
// in clang main as of October 2026.

module;

// MD5 is only available in the Weak namespace.
#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#include <cryptopp/sha.h>

export module cryptopp;

export namespace CryptoPP {
    using CryptoPP::byte;
    using CryptoPP::SHA256;
}

// CryptoPP::Weak pulls in CryptoPP::Weak1 with a using-directive.
export namespace CryptoPP::Weak {
    using CryptoPP::Weak1::MD5;
}
