/*
 * Copyright (C) 2025-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// Tests for bson_type_impl — the CQL type system integration layer.
// The lower-level bson::writer / bson::document tests are in bson_test.cc.

#include "test/lib/scylla_test_case.hh"

#include "types/types.hh"
#include "types/concrete_types.hh"
#include "types/json_utils.hh"
#include "utils/bson.hh"
#include "db/marshal/type_parser.hh"

namespace {

// Helper: create a data_value from a bson::document.
// bson::document is not in data_value's explicit constructor list,
// so we must go through concrete_type<bson::document>::make_value().
data_value make_bson_dv(bson::document doc) {
    return static_cast<const bson_type_impl&>(*bson_type).make_value(std::move(doc));
}

// Build a simple BSON document {"hello": "world"} and return its raw bytes.
bytes hello_world_bson() {
    bson::writer w;
    w.add_string("hello", "world");
    auto doc = std::move(w).finish();
    return to_bytes(managed_bytes_view(doc.as_managed_bytes()));
}

// Build an empty BSON document (5 bytes: length + terminator).
bytes empty_bson() {
    bson::writer w;
    auto doc = std::move(w).finish();
    return to_bytes(managed_bytes_view(doc.as_managed_bytes()));
}

} // anonymous namespace

// bson_type is a distinct type from bytes_type.
SEASTAR_THREAD_TEST_CASE(bson_type_identity) {
    BOOST_REQUIRE(bson_type != bytes_type);
    BOOST_REQUIRE_EQUAL(bson_type->name(), "com.scylladb.db.marshal.BsonType");
}

// data_type_for_v<bson::document> maps to bson_type, not bytes_type.
SEASTAR_THREAD_TEST_CASE(bson_data_type_for) {
    BOOST_REQUIRE(data_type_for_v<bson::document> == bson_type);
    BOOST_REQUIRE(data_type_for_v<bytes> == bytes_type);
    BOOST_REQUIRE(data_type_for_v<bson::document> != data_type_for_v<bytes>);
}

// parse_type resolves the Java class name to bson_type.
SEASTAR_THREAD_TEST_CASE(bson_parse_type) {
    auto parsed = db::marshal::type_parser::parse(sstring("com.scylladb.db.marshal.BsonType"));
    BOOST_REQUIRE(parsed == bson_type);
}

// Serialize/deserialize round-trip through decompose → deserialize → value_cast.
SEASTAR_THREAD_TEST_CASE(bson_serialize_deserialize_round_trip) {
    auto raw = hello_world_bson();
    auto doc = bson::document::from_managed_bytes_unsafe(managed_bytes(raw));

    // decompose: native → serialized bytes
    auto dv = make_bson_dv(std::move(doc));
    auto serialized = bson_type->decompose(dv);

    // deserialize: serialized bytes → data_value → native
    auto deserialized = bson_type->deserialize(serialized);
    auto& recovered = value_cast<bson::document>(deserialized);

    // The recovered document should have the same bytes.
    auto recovered_bytes = to_bytes(managed_bytes_view(recovered.as_managed_bytes()));
    BOOST_REQUIRE_EQUAL(recovered_bytes, raw);
}

// Empty serialized value round-trips correctly.
SEASTAR_THREAD_TEST_CASE(bson_serialize_deserialize_empty) {
    bson::document doc; // default-constructed, empty
    BOOST_REQUIRE(doc.empty());

    auto serialized = bson_type->decompose(make_bson_dv(std::move(doc)));
    BOOST_REQUIRE(serialized.empty());

    auto deserialized = bson_type->deserialize(serialized);
    auto& recovered = value_cast<bson::document>(deserialized);
    BOOST_REQUIRE(recovered.empty());
}

// A valid BSON document (5 bytes minimum) round-trips.
SEASTAR_THREAD_TEST_CASE(bson_serialize_deserialize_minimal_doc) {
    auto raw = empty_bson();
    BOOST_REQUIRE_EQUAL(raw.size(), 5u);

    auto doc = bson::document::from_managed_bytes_unsafe(managed_bytes(raw));
    auto serialized = bson_type->decompose(make_bson_dv(std::move(doc)));
    auto deserialized = bson_type->deserialize(serialized);
    auto& recovered = value_cast<bson::document>(deserialized);

    auto recovered_bytes = to_bytes(managed_bytes_view(recovered.as_managed_bytes()));
    BOOST_REQUIRE_EQUAL(recovered_bytes, raw);
}

// from_string accepts hex input and produces the correct bytes.
SEASTAR_THREAD_TEST_CASE(bson_from_string) {
    auto raw = hello_world_bson();
    auto hex = to_hex(raw);

    auto parsed = bson_type->from_string(hex);
    BOOST_REQUIRE(bson_type->equal(parsed, raw));
}

// to_string produces hex output that round-trips through from_string.
SEASTAR_THREAD_TEST_CASE(bson_to_string) {
    auto raw = hello_world_bson();
    auto str = bson_type->to_string(raw);
    auto round_tripped = bson_type->from_string(str);
    BOOST_REQUIRE(bson_type->equal(round_tripped, raw));
}

// to_string of empty value produces empty string.
SEASTAR_THREAD_TEST_CASE(bson_to_string_empty) {
    bytes empty;
    auto str = bson_type->to_string(empty);
    BOOST_REQUIRE_EQUAL(str, "");
}

// Unsigned byte-order comparison.
SEASTAR_THREAD_TEST_CASE(bson_compare) {
    // {"a": 1} vs {"a": 2} — should differ in the int32 value byte.
    bson::writer w1;
    w1.add_int32("a", 1);
    auto b1 = bson_type->decompose(make_bson_dv(std::move(w1).finish()));

    bson::writer w2;
    w2.add_int32("a", 2);
    auto b2 = bson_type->decompose(make_bson_dv(std::move(w2).finish()));

    BOOST_REQUIRE(bson_type->less(b1, b2));
    BOOST_REQUIRE(!bson_type->less(b2, b1));
    BOOST_REQUIRE(!bson_type->less(b1, b1));
}

// Equal documents compare equal.
SEASTAR_THREAD_TEST_CASE(bson_compare_equal) {
    auto raw = hello_world_bson();
    BOOST_REQUIRE(bson_type->equal(raw, raw));
    BOOST_REQUIRE(!bson_type->less(raw, raw));
}

// Empty values sort before non-empty.
SEASTAR_THREAD_TEST_CASE(bson_compare_empty) {
    bytes empty;
    auto raw = hello_world_bson();

    BOOST_REQUIRE(bson_type->less(empty, raw));
    BOOST_REQUIRE(!bson_type->less(raw, empty));
}

// JSON: to_json_string produces "0x" + hex.
SEASTAR_THREAD_TEST_CASE(bson_to_json) {
    auto raw = hello_world_bson();
    auto json = to_json_string(*bson_type, raw);

    auto hex = to_hex(raw);
    auto expected = "\"0x" + hex + "\"";
    BOOST_REQUIRE_EQUAL(json, expected);
}

// JSON: from_json_object parses "0x"-prefixed hex back to bytes.
SEASTAR_THREAD_TEST_CASE(bson_from_json) {
    auto raw = hello_world_bson();
    auto hex = to_hex(raw);

    auto json_str = "\"0x" + hex + "\"";
    auto json_val = rjson::parse(json_str);
    auto parsed = from_json_object(*bson_type, json_val);

    BOOST_REQUIRE(bson_type->equal(parsed, raw));
}

// JSON round-trip: to_json_string → parse → from_json_object.
SEASTAR_THREAD_TEST_CASE(bson_json_round_trip) {
    auto raw = hello_world_bson();
    auto json = to_json_string(*bson_type, raw);
    auto json_val = rjson::parse(json);
    auto recovered = from_json_object(*bson_type, json_val);
    BOOST_REQUIRE(bson_type->equal(recovered, raw));
}

// from_json_object rejects strings without the "0x" prefix.
SEASTAR_THREAD_TEST_CASE(bson_from_json_rejects_no_prefix) {
    auto json_val = rjson::parse("\"deadbeef\"");
    BOOST_REQUIRE_THROW(from_json_object(*bson_type, json_val), marshal_exception);
}
