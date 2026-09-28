/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

// C++20 module interface unit for yaml-cpp.
//
// yaml-cpp's headers include standard library headers (<memory>, <string>,
// ...) textually. Keeping them inside this module's global module fragment
// lets translation units that `import std;` use yaml-cpp without also
// including the standard library textually.
//
// Like the rapidjson module, this works around a clang bug: in C++26 mode,
// a textual <memory> in a translation unit that also imports std fails in
// libstdc++'s <bits/indirect.h>. The bug is still present in clang 23.1 and
// in clang main as of October 2026.

module;

#include <yaml-cpp/yaml.h>

export module yaml_cpp;

export namespace YAML {
    // node/node.h, node/iterator.h
    using YAML::Node;
    using YAML::NodeType;
    using YAML::iterator;
    using YAML::const_iterator;

    // node/convert.h: specialized by users to convert their own types.
    using YAML::convert;

    // node/parse.h, node/emit.h
    using YAML::Load;
    using YAML::LoadFile;
    using YAML::Dump;

    // emitter.h, emittermanip.h
    using YAML::Emitter;
    using YAML::EMITTER_MANIP;
    using YAML::Key;
    using YAML::Value;
    using YAML::BeginSeq;
    using YAML::EndSeq;
    using YAML::BeginMap;
    using YAML::EndMap;
    using YAML::SingleQuoted;
    using YAML::operator<<;

    // exceptions.h
    using YAML::Exception;
    using YAML::BadConversion;
    using YAML::ParserException;
}
