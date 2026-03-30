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
#include "cql3/functions/castas_fcts.hh"
#include "lang/lua_scylla_types.hh"
#include <lua.hpp>

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

// JSON: to_json_string produces a human-readable JSON object.
SEASTAR_THREAD_TEST_CASE(bson_to_json_hello_world) {
    auto raw = hello_world_bson();
    auto json = to_json_string(*bson_type, raw);
    BOOST_REQUIRE_EQUAL(json, "{\"hello\": \"world\"}");
}

// JSON: to_json_string with multiple element types.
SEASTAR_THREAD_TEST_CASE(bson_to_json_mixed_types) {
    bson::writer w;
    w.add_int32("i", 42);
    w.add_string("s", "test");
    w.add_bool("b", true);
    w.add_null("n");
    w.add_double("d", 3.14);
    auto doc = std::move(w).finish();
    auto raw = to_bytes(managed_bytes_view(doc.as_managed_bytes()));
    auto json = to_json_string(*bson_type, raw);
    // Verify it starts and ends correctly and contains all keys
    BOOST_REQUIRE(json.find("\"i\": 42") != sstring::npos);
    BOOST_REQUIRE(json.find("\"s\": \"test\"") != sstring::npos);
    BOOST_REQUIRE(json.find("\"b\": true") != sstring::npos);
    BOOST_REQUIRE(json.find("\"n\": null") != sstring::npos);
    BOOST_REQUIRE(json.find("\"d\": 3.14") != sstring::npos);
}

// JSON: to_json_string with nested document.
SEASTAR_THREAD_TEST_CASE(bson_to_json_nested) {
    bson::writer inner;
    inner.add_int32("x", 1);
    auto inner_doc = std::move(inner).finish();
    bson::writer outer;
    outer.add_document("nested", inner_doc);
    auto doc = std::move(outer).finish();
    auto raw = to_bytes(managed_bytes_view(doc.as_managed_bytes()));
    auto json = to_json_string(*bson_type, raw);
    BOOST_REQUIRE_EQUAL(json, "{\"nested\": {\"x\": 1}}");
}

// JSON: to_json_string with array.
SEASTAR_THREAD_TEST_CASE(bson_to_json_array) {
    bson::writer arr;
    arr.add_int32("0", 10);
    arr.add_int32("1", 20);
    arr.add_int32("2", 30);
    auto arr_doc = std::move(arr).finish();
    bson::writer outer;
    outer.add_array("nums", arr_doc);
    auto doc = std::move(outer).finish();
    auto raw = to_bytes(managed_bytes_view(doc.as_managed_bytes()));
    auto json = to_json_string(*bson_type, raw);
    BOOST_REQUIRE_EQUAL(json, "{\"nums\": [10, 20, 30]}");
}

// JSON: to_json_string with empty document.
SEASTAR_THREAD_TEST_CASE(bson_to_json_empty_doc) {
    auto raw = empty_bson();
    auto json = to_json_string(*bson_type, raw);
    BOOST_REQUIRE_EQUAL(json, "{}");
}

// JSON: to_json_string with int64.
SEASTAR_THREAD_TEST_CASE(bson_to_json_int64) {
    bson::writer w;
    w.add_int64("big", int64_t(1) << 40);
    auto doc = std::move(w).finish();
    auto raw = to_bytes(managed_bytes_view(doc.as_managed_bytes()));
    auto json = to_json_string(*bson_type, raw);
    BOOST_REQUIRE_EQUAL(json, "{\"big\": 1099511627776}");
}

// JSON: to_json_string with boolean false.
SEASTAR_THREAD_TEST_CASE(bson_to_json_bool_false) {
    bson::writer w;
    w.add_bool("f", false);
    auto doc = std::move(w).finish();
    auto raw = to_bytes(managed_bytes_view(doc.as_managed_bytes()));
    auto json = to_json_string(*bson_type, raw);
    BOOST_REQUIRE_EQUAL(json, "{\"f\": false}");
}

// JSON: to_json_string with string needing escaping.
SEASTAR_THREAD_TEST_CASE(bson_to_json_escape) {
    bson::writer w;
    w.add_string("s", "hello\"world\n");
    auto doc = std::move(w).finish();
    auto raw = to_bytes(managed_bytes_view(doc.as_managed_bytes()));
    auto json = to_json_string(*bson_type, raw);
    BOOST_REQUIRE_EQUAL(json, "{\"s\": \"hello\\\"world\\n\"}");
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

// JSON: from_json_object still accepts "0x"-prefixed hex.
SEASTAR_THREAD_TEST_CASE(bson_from_json_hex) {
    auto raw = hello_world_bson();
    auto hex = to_hex(raw);
    auto json_str = "\"0x" + hex + "\"";
    auto json_val = rjson::parse(json_str);
    auto parsed = from_json_object(*bson_type, json_val);
    BOOST_REQUIRE(bson_type->equal(parsed, raw));
}

// from_json_object rejects strings without the "0x" prefix.
SEASTAR_THREAD_TEST_CASE(bson_from_json_rejects_no_prefix) {
    auto json_val = rjson::parse("\"deadbeef\"");
    BOOST_REQUIRE_THROW(from_json_object(*bson_type, json_val), marshal_exception);
}

// --- CAST function tests ---

// CAST(bson AS text) produces hex string.
SEASTAR_THREAD_TEST_CASE(bson_cast_to_text) {
    auto raw = hello_world_bson();
    auto doc = bson::document::from_managed_bytes_unsafe(managed_bytes(raw));
    auto dv = make_bson_dv(std::move(doc));

    auto fn = cql3::functions::get_castas_fctn(utf8_type, bson_type);
    auto result = fn(std::move(dv));
    auto& s = value_cast<sstring>(result);
    BOOST_REQUIRE_EQUAL(s, to_hex(raw));
}

// CAST(bson AS ascii) also produces hex string.
SEASTAR_THREAD_TEST_CASE(bson_cast_to_ascii) {
    auto raw = hello_world_bson();
    auto doc = bson::document::from_managed_bytes_unsafe(managed_bytes(raw));
    auto dv = make_bson_dv(std::move(doc));

    auto fn = cql3::functions::get_castas_fctn(ascii_type, bson_type);
    auto result = fn(std::move(dv));
    auto& s = value_cast<sstring>(result);
    BOOST_REQUIRE_EQUAL(s, to_hex(raw));
}

// CAST(text AS bson) parses hex string to bson::document.
SEASTAR_THREAD_TEST_CASE(bson_cast_from_text) {
    auto raw = hello_world_bson();
    auto hex = to_hex(raw);
    auto text_dv = data_value(sstring(hex));

    auto fn = cql3::functions::get_castas_fctn(bson_type, utf8_type);
    auto result = fn(std::move(text_dv));
    auto& doc = value_cast<bson::document>(result);
    auto result_bytes = to_bytes(managed_bytes_view(doc.as_managed_bytes()));
    BOOST_REQUIRE_EQUAL(result_bytes, raw);
}

// CAST(text AS bson) round-trips with CAST(bson AS text).
SEASTAR_THREAD_TEST_CASE(bson_cast_text_round_trip) {
    auto raw = hello_world_bson();
    auto doc = bson::document::from_managed_bytes_unsafe(managed_bytes(raw));
    auto dv = make_bson_dv(std::move(doc));

    auto to_text = cql3::functions::get_castas_fctn(utf8_type, bson_type);
    auto from_text = cql3::functions::get_castas_fctn(bson_type, utf8_type);

    auto text_result = to_text(std::move(dv));
    auto bson_result = from_text(std::move(text_result));
    auto& recovered = value_cast<bson::document>(bson_result);
    auto recovered_bytes = to_bytes(managed_bytes_view(recovered.as_managed_bytes()));
    BOOST_REQUIRE_EQUAL(recovered_bytes, raw);
}

// CAST(bson AS blob) extracts raw bytes.
SEASTAR_THREAD_TEST_CASE(bson_cast_to_blob) {
    auto raw = hello_world_bson();
    auto doc = bson::document::from_managed_bytes_unsafe(managed_bytes(raw));
    auto dv = make_bson_dv(std::move(doc));

    auto fn = cql3::functions::get_castas_fctn(bytes_type, bson_type);
    auto result = fn(std::move(dv));
    auto& b = value_cast<bytes>(result);
    BOOST_REQUIRE_EQUAL(b, raw);
}

// CAST(blob AS bson) wraps raw bytes as bson::document.
SEASTAR_THREAD_TEST_CASE(bson_cast_from_blob) {
    auto raw = hello_world_bson();
    auto blob_dv = data_value(bytes(raw));

    auto fn = cql3::functions::get_castas_fctn(bson_type, bytes_type);
    auto result = fn(std::move(blob_dv));
    auto& doc = value_cast<bson::document>(result);
    auto result_bytes = to_bytes(managed_bytes_view(doc.as_managed_bytes()));
    BOOST_REQUIRE_EQUAL(result_bytes, raw);
}

// CAST(bson AS blob) round-trips with CAST(blob AS bson).
SEASTAR_THREAD_TEST_CASE(bson_cast_blob_round_trip) {
    auto raw = hello_world_bson();
    auto doc = bson::document::from_managed_bytes_unsafe(managed_bytes(raw));
    auto dv = make_bson_dv(std::move(doc));

    auto to_blob = cql3::functions::get_castas_fctn(bytes_type, bson_type);
    auto from_blob = cql3::functions::get_castas_fctn(bson_type, bytes_type);

    auto blob_result = to_blob(std::move(dv));
    auto bson_result = from_blob(std::move(blob_result));
    auto& recovered = value_cast<bson::document>(bson_result);
    auto recovered_bytes = to_bytes(managed_bytes_view(recovered.as_managed_bytes()));
    BOOST_REQUIRE_EQUAL(recovered_bytes, raw);
}

// CAST(empty_bson AS blob) produces empty bytes.
SEASTAR_THREAD_TEST_CASE(bson_cast_empty_to_blob) {
    bson::document doc; // default-constructed, empty
    auto dv = make_bson_dv(std::move(doc));

    auto fn = cql3::functions::get_castas_fctn(bytes_type, bson_type);
    auto result = fn(std::move(dv));
    auto& b = value_cast<bytes>(result);
    BOOST_REQUIRE(b.empty());
}

// Unsupported casts throw invalid_request_exception.
SEASTAR_THREAD_TEST_CASE(bson_cast_unsupported) {
    BOOST_REQUIRE_THROW(
        cql3::functions::get_castas_fctn(int32_type, bson_type),
        exceptions::invalid_request_exception);
    BOOST_REQUIRE_THROW(
        cql3::functions::get_castas_fctn(bson_type, int32_type),
        exceptions::invalid_request_exception);
}

// CAST(garbage_text AS bson) rejects invalid BSON.
SEASTAR_THREAD_TEST_CASE(bson_cast_from_text_rejects_invalid) {
    // "010203" is only 3 bytes — too short for any BSON document.
    auto text_dv = data_value(sstring("010203"));
    auto fn = cql3::functions::get_castas_fctn(bson_type, utf8_type);
    BOOST_REQUIRE_THROW(fn(std::move(text_dv)), marshal_exception);
}

// CAST(garbage_blob AS bson) rejects invalid BSON.
SEASTAR_THREAD_TEST_CASE(bson_cast_from_blob_rejects_invalid) {
    auto blob_dv = data_value(bytes{0x01, 0x02, 0x03});
    auto fn = cql3::functions::get_castas_fctn(bson_type, bytes_type);
    BOOST_REQUIRE_THROW(fn(std::move(blob_dv)), marshal_exception);
}

// --- Lua UDF integration tests ---

namespace {

// RAII wrapper for a Lua state used in tests.
struct lua_state_guard {
    lua_State* l;
    lua_state_guard() : l(luaL_newstate()) {
        BOOST_REQUIRE(l != nullptr);
        lua::register_metatables(l);
    }
    ~lua_state_guard() { lua_close(l); }
    operator lua_State*() const { return l; }
};

} // anonymous namespace

// Lua: push a simple BSON document and read it back.
SEASTAR_THREAD_TEST_CASE(bson_lua_round_trip_simple) {
    lua_state_guard L;
    bson::writer w;
    w.add_string("name", "alice");
    w.add_int32("age", 30);
    auto doc = std::move(w).finish();
    auto dv = make_bson_dv(std::move(doc));

    lua::push_data_value(L, dv);

    // The value on the stack should be a table
    BOOST_REQUIRE_EQUAL(lua_type(L, -1), LUA_TTABLE);

    // Check the "name" field
    lua_getfield(L, -1, "name");
    BOOST_REQUIRE_EQUAL(lua_type(L, -1), LUA_TSTRING);
    size_t len;
    const char* s = lua_tolstring(L, -1, &len);
    BOOST_REQUIRE_EQUAL(std::string_view(s, len), "alice");
    lua_pop(L, 1);

    // Check the "age" field
    lua_getfield(L, -1, "age");
    BOOST_REQUIRE_EQUAL(lua_type(L, -1), LUA_TNUMBER);
    BOOST_REQUIRE_EQUAL(lua_tointeger(L, -1), 30);
    lua_pop(L, 1);

    // Now read it back as a BSON document
    auto result = lua::pop_data_value(L, bson_type);
    auto& result_doc = value_cast<bson::document>(result);
    BOOST_REQUIRE(!result_doc.empty());
}

// Lua: push a BSON document with nested document.
SEASTAR_THREAD_TEST_CASE(bson_lua_nested_document) {
    lua_state_guard L;
    bson::writer inner;
    inner.add_int32("x", 42);
    auto inner_doc = std::move(inner).finish();
    bson::writer outer;
    outer.add_document("nested", inner_doc);
    auto doc = std::move(outer).finish();
    auto dv = make_bson_dv(std::move(doc));

    lua::push_data_value(L, dv);

    // Check nested.x
    lua_getfield(L, -1, "nested");
    BOOST_REQUIRE_EQUAL(lua_type(L, -1), LUA_TTABLE);
    lua_getfield(L, -1, "x");
    BOOST_REQUIRE_EQUAL(lua_tointeger(L, -1), 42);
    lua_pop(L, 3);
}

// Lua: push a BSON array → sequential Lua table.
SEASTAR_THREAD_TEST_CASE(bson_lua_array) {
    lua_state_guard L;
    bson::writer arr;
    arr.add_int32("0", 10);
    arr.add_int32("1", 20);
    arr.add_int32("2", 30);
    auto arr_doc = std::move(arr).finish();
    bson::writer outer;
    outer.add_array("nums", arr_doc);
    auto doc = std::move(outer).finish();
    auto dv = make_bson_dv(std::move(doc));

    lua::push_data_value(L, dv);

    lua_getfield(L, -1, "nums");
    BOOST_REQUIRE_EQUAL(lua_type(L, -1), LUA_TTABLE);
    // Lua arrays are 1-indexed
    lua_rawgeti(L, -1, 1);
    BOOST_REQUIRE_EQUAL(lua_tointeger(L, -1), 10);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 2);
    BOOST_REQUIRE_EQUAL(lua_tointeger(L, -1), 20);
    lua_pop(L, 1);
    lua_rawgeti(L, -1, 3);
    BOOST_REQUIRE_EQUAL(lua_tointeger(L, -1), 30);
    lua_pop(L, 2);
    lua_pop(L, 1);
}

// Lua: push BSON with various scalar types.
SEASTAR_THREAD_TEST_CASE(bson_lua_scalar_types) {
    lua_state_guard L;
    bson::writer w;
    w.add_double("d", 3.14);
    w.add_bool("b", true);
    w.add_null("n");
    w.add_int64("big", int64_t(1) << 40);
    auto doc = std::move(w).finish();
    auto dv = make_bson_dv(std::move(doc));

    lua::push_data_value(L, dv);

    lua_getfield(L, -1, "d");
    BOOST_REQUIRE_CLOSE(lua_tonumber(L, -1), 3.14, 0.001);
    lua_pop(L, 1);

    lua_getfield(L, -1, "b");
    BOOST_REQUIRE_EQUAL(lua_toboolean(L, -1), 1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "n");
    BOOST_REQUIRE_EQUAL(lua_type(L, -1), LUA_TNIL);
    lua_pop(L, 1);

    lua_getfield(L, -1, "big");
    BOOST_REQUIRE_EQUAL(lua_tointeger(L, -1), int64_t(1) << 40);
    lua_pop(L, 2);
}

// Lua: round-trip a Lua table → BSON document → verify contents.
SEASTAR_THREAD_TEST_CASE(bson_lua_from_table) {
    lua_state_guard L;

    // Build a Lua table {name="bob", score=100}
    lua_createtable(L, 0, 2);
    lua_pushstring(L, "bob");
    lua_setfield(L, -2, "name");
    lua_pushinteger(L, 100);
    lua_setfield(L, -2, "score");

    auto result = lua::pop_data_value(L, bson_type);
    auto& doc = value_cast<bson::document>(result);

    // Verify using the reader
    auto raw = to_bytes(managed_bytes_view(doc.as_managed_bytes()));
    auto json = to_json_string(*bson_type, raw);
    // Key order in Lua tables is unspecified, so check both fields exist
    BOOST_REQUIRE(json.find("\"name\": \"bob\"") != sstring::npos);
    BOOST_REQUIRE(json.find("\"score\": 100") != sstring::npos);
}

// Lua: round-trip a Lua array table → BSON array → verify.
SEASTAR_THREAD_TEST_CASE(bson_lua_from_array_table) {
    lua_state_guard L;

    // Build a Lua array {10, 20, 30}
    lua_createtable(L, 3, 0);
    lua_pushinteger(L, 10);
    lua_rawseti(L, -2, 1);
    lua_pushinteger(L, 20);
    lua_rawseti(L, -2, 2);
    lua_pushinteger(L, 30);
    lua_rawseti(L, -2, 3);

    auto result = lua::pop_data_value(L, bson_type);
    auto& doc = value_cast<bson::document>(result);

    // This should be an array with 0-based string keys
    auto mbv = managed_bytes_view(doc.as_managed_bytes());
    with_simplified(mbv, [](auto view) {
        bson::reader rdr(view);
        auto e0 = rdr.next();
        BOOST_REQUIRE_EQUAL(e0.key, "0");
        BOOST_REQUIRE_EQUAL(e0.as_int32(), 10);
        auto e1 = rdr.next();
        BOOST_REQUIRE_EQUAL(e1.key, "1");
        BOOST_REQUIRE_EQUAL(e1.as_int32(), 20);
        auto e2 = rdr.next();
        BOOST_REQUIRE_EQUAL(e2.key, "2");
        BOOST_REQUIRE_EQUAL(e2.as_int32(), 30);
        BOOST_REQUIRE(!rdr.has_next());
    });
}

// Lua: full push/pop round-trip preserves structure.
SEASTAR_THREAD_TEST_CASE(bson_lua_full_round_trip) {
    lua_state_guard L;
    bson::writer w;
    w.add_string("key", "value");
    w.add_int32("num", 42);
    auto original_doc = std::move(w).finish();
    auto original_bytes = to_bytes(managed_bytes_view(original_doc.as_managed_bytes()));
    auto dv = make_bson_dv(bson::document(original_doc));

    // Push to Lua, then pop back
    lua::push_data_value(L, dv);
    auto result = lua::pop_data_value(L, bson_type);
    auto& result_doc = value_cast<bson::document>(result);

    // Verify contents via JSON (key order may differ)
    auto result_bytes = to_bytes(managed_bytes_view(result_doc.as_managed_bytes()));
    auto result_json = to_json_string(*bson_type, result_bytes);
    BOOST_REQUIRE(result_json.find("\"key\": \"value\"") != sstring::npos);
    BOOST_REQUIRE(result_json.find("\"num\": 42") != sstring::npos);
}

// Lua: empty BSON document → empty Lua table → empty BSON document.
SEASTAR_THREAD_TEST_CASE(bson_lua_empty_document) {
    lua_state_guard L;
    bson::writer w;
    auto doc = std::move(w).finish();
    auto dv = make_bson_dv(std::move(doc));

    lua::push_data_value(L, dv);
    BOOST_REQUIRE_EQUAL(lua_type(L, -1), LUA_TTABLE);

    // Empty table should produce a document (not array)
    auto result = lua::pop_data_value(L, bson_type);
    auto& result_doc = value_cast<bson::document>(result);
    auto result_bytes = to_bytes(managed_bytes_view(result_doc.as_managed_bytes()));
    auto result_json = to_json_string(*bson_type, result_bytes);
    BOOST_REQUIRE_EQUAL(result_json, "{}");
}

// Lua: nested table round-trip.
SEASTAR_THREAD_TEST_CASE(bson_lua_nested_table_round_trip) {
    lua_state_guard L;

    // Build {outer: {inner: 99}}
    lua_createtable(L, 0, 1);
    lua_createtable(L, 0, 1);
    lua_pushinteger(L, 99);
    lua_setfield(L, -2, "inner");
    lua_setfield(L, -2, "outer");

    auto result = lua::pop_data_value(L, bson_type);
    auto& doc = value_cast<bson::document>(result);
    auto raw = to_bytes(managed_bytes_view(doc.as_managed_bytes()));
    auto json = to_json_string(*bson_type, raw);
    BOOST_REQUIRE_EQUAL(json, "{\"outer\": {\"inner\": 99}}");
}

// Lua: BSON from non-table is rejected.
SEASTAR_THREAD_TEST_CASE(bson_lua_rejects_non_table) {
    lua_state_guard L;
    lua_pushstring(L, "not a table");
    BOOST_REQUIRE_THROW(lua::pop_data_value(L, bson_type), exceptions::invalid_request_exception);
}
