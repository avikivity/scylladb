/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

// C++20 module interface unit for rapidjson.
//
// rapidjson's headers include standard library headers (<memory>, <string>,
// ...) textually. Keeping them inside this module's global module fragment
// lets translation units that `import std;` use rapidjson without also
// including the standard library textually.
//
// This works around a clang bug: in C++26 mode, a textual <memory> in a
// translation unit that also imports std fails in libstdc++'s
// <bits/indirect.h> ("too many template arguments for alias template
// 'indirect'"), whichever comes first. The bug is still present in clang
// 23.1 and in clang main as of October 2026.

module;

// Unlike the other third-party wrappers, this one includes a Scylla header:
// rapidjson is configured by macros that must be defined before its headers
// are included, and they have to be the same ones rjson.cc sees. That header
// includes nothing, so it adds no textual standard library includes.
#include "utils/rjson_config.hh"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/allocators.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/stream.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/error/en.h>

export module rapidjson;

export namespace rapidjson {
    using rapidjson::SizeType;

    // rapidjson.h
    using rapidjson::Type;
    using rapidjson::kNullType;
    using rapidjson::kFalseType;
    using rapidjson::kTrueType;
    using rapidjson::kObjectType;
    using rapidjson::kArrayType;
    using rapidjson::kStringType;
    using rapidjson::kNumberType;

    // encodings.h
    using rapidjson::UTF8;

    // allocators.h
    using rapidjson::CrtAllocator;

    // document.h
    using rapidjson::GenericValue;
    using rapidjson::Value;
    using rapidjson::GenericDocument;
    using rapidjson::Document;

    // reader.h
    using rapidjson::GenericReader;
    using rapidjson::ParseErrorCode;

    // writer.h, stringbuffer.h, ostreamwrapper.h
    using rapidjson::Writer;
    using rapidjson::GenericStringBuffer;
    using rapidjson::BasicOStreamWrapper;

    // memorystream.h, encodedstream.h
    using rapidjson::MemoryStream;
    using rapidjson::EncodedInputStream;

    // error/en.h
    using rapidjson::GetParseError_En;
}

export namespace rapidjson::internal {
    // Specialized by users to convert their own types to/from GenericValue.
    using rapidjson::internal::TypeHelper;
    using rapidjson::internal::Stack;
}
