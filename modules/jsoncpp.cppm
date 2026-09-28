/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

// C++20 module interface unit for jsoncpp.
//
// jsoncpp's headers include standard library headers (<memory>, <string>,
// ...) textually. Keeping them inside this module's global module fragment
// lets translation units that `import std;` use jsoncpp without also
// including the standard library textually.
//
// Like the rapidjson module, this works around a clang bug: in C++26 mode,
// a textual <memory> in a translation unit that also imports std fails in
// libstdc++'s <bits/indirect.h>. The bug is still present in clang 23.1 and
// in clang main as of October 2026.

module;

#include <json/json.h>

export module jsoncpp;

export namespace Json {
    // value.h
    using Json::Value;
    using Json::ValueType;
    using Json::objectValue;
    using Json::UInt64;

    // writer.h
    using Json::StreamWriterBuilder;
    using Json::writeString;
    using Json::operator<<;

    // reader.h
    using Json::operator>>;
}
