/*
 * Copyright (C) 2025-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

#define BOOST_TEST_MODULE core

#include "utils/bson.hh"

#include <boost/test/unit_test.hpp>

#include <cstring>

#include "marshal_exception.hh"

namespace {

// Linearize a bson::document into a contiguous bytes value for easy inspection.
bytes linearize(const bson::document& doc) {
    return to_bytes(managed_bytes_view(doc.as_managed_bytes()));
}

// Read a little-endian int32 from a byte pointer.
int32_t read_le_int32(const int8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, sizeof(v));
    return seastar::le_to_cpu(v);
}

// Read a little-endian int64 from a byte pointer.
int64_t read_le_int64(const int8_t* p) {
    uint64_t v;
    std::memcpy(&v, p, sizeof(v));
    return seastar::le_to_cpu(v);
}

// Read a little-endian uint64 from a byte pointer.
uint64_t read_le_uint64(const int8_t* p) {
    uint64_t v;
    std::memcpy(&v, p, sizeof(v));
    return seastar::le_to_cpu(v);
}

// Read a little-endian IEEE 754 double from a byte pointer.
double read_le_double(const int8_t* p) {
    uint64_t v;
    std::memcpy(&v, p, sizeof(v));
    v = seastar::le_to_cpu(v);
    return std::bit_cast<double>(v);
}

} // anonymous namespace

// An empty document is 5 bytes: int32 length (= 5) + 0x00 terminator.
BOOST_AUTO_TEST_CASE(empty_document) {
    bson::writer w;
    auto doc = std::move(w).finish();

    BOOST_REQUIRE_EQUAL(doc.size(), 5u);
    BOOST_REQUIRE(!doc.empty());

    auto raw = linearize(doc);
    BOOST_REQUIRE_EQUAL(read_le_int32(raw.data()), 5);
    BOOST_REQUIRE_EQUAL(raw[4], 0x00);
}

// Verify the length prefix always matches document::size().
BOOST_AUTO_TEST_CASE(length_prefix_matches_size) {
    bson::writer w;
    w.add_int32("x", 42);
    w.add_string("msg", "hello");
    auto doc = std::move(w).finish();

    auto raw = linearize(doc);
    BOOST_REQUIRE_EQUAL(static_cast<size_t>(read_le_int32(raw.data())), doc.size());
}

BOOST_AUTO_TEST_CASE(int32_element) {
    bson::writer w;
    w.add_int32("x", 42);
    auto doc = std::move(w).finish();

    // Layout: 4 (length) + 1 (type) + 2 (key "x\0") + 4 (value) + 1 (terminator) = 12
    BOOST_REQUIRE_EQUAL(doc.size(), 12u);

    auto raw = linearize(doc);
    size_t off = 4; // skip length prefix

    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::int32));
    off += 1;

    BOOST_REQUIRE_EQUAL(raw[off], 'x');
    BOOST_REQUIRE_EQUAL(raw[off + 1], 0x00);
    off += 2;

    BOOST_REQUIRE_EQUAL(read_le_int32(raw.data() + off), 42);
    off += 4;

    BOOST_REQUIRE_EQUAL(raw[off], 0x00); // document terminator
}

BOOST_AUTO_TEST_CASE(int64_element) {
    bson::writer w;
    w.add_int64("n", int64_t(1) << 40);
    auto doc = std::move(w).finish();

    // Layout: 4 + 1 + 2 ("n\0") + 8 + 1 = 16
    BOOST_REQUIRE_EQUAL(doc.size(), 16u);

    auto raw = linearize(doc);
    size_t off = 4;

    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::int64));
    off += 1 + 2; // type + key

    BOOST_REQUIRE_EQUAL(read_le_int64(raw.data() + off), int64_t(1) << 40);
}

BOOST_AUTO_TEST_CASE(double_element) {
    bson::writer w;
    w.add_double("pi", 3.14159);
    auto doc = std::move(w).finish();

    // Layout: 4 + 1 + 3 ("pi\0") + 8 + 1 = 17
    BOOST_REQUIRE_EQUAL(doc.size(), 17u);

    auto raw = linearize(doc);
    size_t off = 4 + 1 + 3; // skip length + type + key

    BOOST_REQUIRE_EQUAL(read_le_double(raw.data() + off), 3.14159);
}

BOOST_AUTO_TEST_CASE(bool_element) {
    bson::writer w;
    w.add_bool("t", true);
    w.add_bool("f", false);
    auto doc = std::move(w).finish();

    // Layout: 4 + (1+2+1) + (1+2+1) + 1 = 13
    BOOST_REQUIRE_EQUAL(doc.size(), 13u);

    auto raw = linearize(doc);

    // First element: true
    size_t off = 4;
    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::boolean));
    off += 1 + 2; // type + key "t\0"
    BOOST_REQUIRE_EQUAL(raw[off], 0x01);

    // Second element: false
    off += 1;
    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::boolean));
    off += 1 + 2; // type + key "f\0"
    BOOST_REQUIRE_EQUAL(raw[off], 0x00);
}

BOOST_AUTO_TEST_CASE(null_element) {
    bson::writer w;
    w.add_null("n");
    auto doc = std::move(w).finish();

    // Layout: 4 + 1 + 2 ("n\0") + 0 (no value) + 1 = 8
    BOOST_REQUIRE_EQUAL(doc.size(), 8u);

    auto raw = linearize(doc);
    BOOST_REQUIRE_EQUAL(raw[4], static_cast<int8_t>(bson::type::null));
    BOOST_REQUIRE_EQUAL(raw[5], 'n');
    BOOST_REQUIRE_EQUAL(raw[6], 0x00);
    BOOST_REQUIRE_EQUAL(raw[7], 0x00); // document terminator
}

BOOST_AUTO_TEST_CASE(string_element) {
    bson::writer w;
    w.add_string("k", "hello");
    auto doc = std::move(w).finish();

    // Layout: 4 + 1 + 2 ("k\0") + 4 (string length=6) + 5 ("hello") + 1 (string null) + 1 = 18
    BOOST_REQUIRE_EQUAL(doc.size(), 18u);

    auto raw = linearize(doc);
    size_t off = 4;

    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::string));
    off += 1 + 2; // type + key "k\0"

    // BSON string length includes the trailing null.
    BOOST_REQUIRE_EQUAL(read_le_int32(raw.data() + off), 6);
    off += 4;

    BOOST_REQUIRE_EQUAL(raw[off + 0], 'h');
    BOOST_REQUIRE_EQUAL(raw[off + 1], 'e');
    BOOST_REQUIRE_EQUAL(raw[off + 2], 'l');
    BOOST_REQUIRE_EQUAL(raw[off + 3], 'l');
    BOOST_REQUIRE_EQUAL(raw[off + 4], 'o');
    BOOST_REQUIRE_EQUAL(raw[off + 5], 0x00);
}

BOOST_AUTO_TEST_CASE(binary_element) {
    const int8_t bin_data[] = {0x01, 0x02, 0x03};
    bytes_view bv(bin_data, 3);

    bson::writer w;
    w.add_binary("b", bv, 0x80); // user-defined subtype
    auto doc = std::move(w).finish();

    // Layout: 4 + 1 + 2 ("b\0") + 4 (length=3) + 1 (subtype) + 3 (data) + 1 = 16
    BOOST_REQUIRE_EQUAL(doc.size(), 16u);

    auto raw = linearize(doc);
    size_t off = 4;

    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::binary));
    off += 1 + 2; // type + key "b\0"

    BOOST_REQUIRE_EQUAL(read_le_int32(raw.data() + off), 3);
    off += 4;

    BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(raw[off]), 0x80); // subtype
    off += 1;

    BOOST_REQUIRE_EQUAL(raw[off + 0], 0x01);
    BOOST_REQUIRE_EQUAL(raw[off + 1], 0x02);
    BOOST_REQUIRE_EQUAL(raw[off + 2], 0x03);
}

BOOST_AUTO_TEST_CASE(datetime_element) {
    bson::writer w;
    w.add_datetime("ts", 1700000000000LL); // some millisecond timestamp
    auto doc = std::move(w).finish();

    // Layout: 4 + 1 + 3 ("ts\0") + 8 + 1 = 17
    BOOST_REQUIRE_EQUAL(doc.size(), 17u);

    auto raw = linearize(doc);
    size_t off = 4;
    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::datetime));
    off += 1 + 3;

    BOOST_REQUIRE_EQUAL(read_le_int64(raw.data() + off), 1700000000000LL);
}

BOOST_AUTO_TEST_CASE(timestamp_element) {
    bson::writer w;
    w.add_timestamp("t", uint64_t(0xDEADBEEF12345678ULL));
    auto doc = std::move(w).finish();

    auto raw = linearize(doc);
    size_t off = 4 + 1 + 2; // length + type + key "t\0"

    BOOST_REQUIRE_EQUAL(read_le_uint64(raw.data() + off), uint64_t(0xDEADBEEF12345678ULL));
}

BOOST_AUTO_TEST_CASE(nested_document) {
    // Build inner document: {"y": 7}
    bson::writer inner;
    inner.add_int32("y", 7);
    auto inner_doc = std::move(inner).finish();
    auto inner_size = inner_doc.size();

    // Build outer document: {"sub": {"y": 7}}
    bson::writer outer;
    outer.add_document("sub", inner_doc);
    auto doc = std::move(outer).finish();

    // Outer layout: 4 + 1 + 4 ("sub\0") + inner_size + 1 = 10 + inner_size
    BOOST_REQUIRE_EQUAL(doc.size(), 10u + inner_size);

    auto raw = linearize(doc);
    size_t off = 4;

    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::document));
    off += 1 + 4; // type + key "sub\0"

    // The nested document's length prefix should match inner_size.
    BOOST_REQUIRE_EQUAL(static_cast<size_t>(read_le_int32(raw.data() + off)), inner_size);
}

BOOST_AUTO_TEST_CASE(nested_array) {
    // Build an array document with index keys: {"0": "a", "1": "b"}
    bson::writer arr;
    arr.add_string("0", "a");
    arr.add_string("1", "b");
    auto arr_doc = std::move(arr).finish();
    auto arr_size = arr_doc.size();

    bson::writer outer;
    outer.add_array("items", arr_doc);
    auto doc = std::move(outer).finish();

    auto raw = linearize(doc);
    // Verify the type tag is array (0x04), not document (0x03).
    BOOST_REQUIRE_EQUAL(raw[4], static_cast<int8_t>(bson::type::array));

    // The embedded array data should match arr_size.
    size_t off = 4 + 1 + 6; // length + type + key "items\0"
    BOOST_REQUIRE_EQUAL(static_cast<size_t>(read_le_int32(raw.data() + off)), arr_size);
}

BOOST_AUTO_TEST_CASE(multiple_elements) {
    bson::writer w;
    w.add_int32("a", 1);
    w.add_string("b", "two");
    w.add_bool("c", true);
    w.add_null("d");
    w.add_double("e", 5.0);
    auto doc = std::move(w).finish();

    // Verify the length prefix matches.
    auto raw = linearize(doc);
    BOOST_REQUIRE_EQUAL(static_cast<size_t>(read_le_int32(raw.data())), doc.size());

    // Verify the document terminator.
    BOOST_REQUIRE_EQUAL(raw[doc.size() - 1], 0x00);

    // Walk through and verify element type tags appear in order.
    size_t off = 4;

    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::int32));   // "a"
    off += 1 + 2 + 4; // type + "a\0" + int32

    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::string));  // "b"
    off += 1 + 2 + 4 + 3 + 1; // type + "b\0" + strlen(=4) + "two" + null

    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::boolean)); // "c"
    off += 1 + 2 + 1; // type + "c\0" + bool

    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::null));    // "d"
    off += 1 + 2; // type + "d\0" + 0 value bytes

    BOOST_REQUIRE_EQUAL(raw[off], static_cast<int8_t>(bson::type::double_value)); // "e"
}

// Verify our output matches the canonical BSON encoding of {"hello": "world"}
// from the BSON spec (https://bsonspec.org/spec.html).
BOOST_AUTO_TEST_CASE(spec_example_hello_world) {
    bson::writer w;
    w.add_string("hello", "world");
    auto doc = std::move(w).finish();

    BOOST_REQUIRE_EQUAL(doc.size(), 22u);

    auto raw = linearize(doc);

    // Expected bytes from the spec:
    // \x16\x00\x00\x00                    int32 = 22
    // \x02                                type = string
    // hello\x00                           key
    // \x06\x00\x00\x00                    string length = 6
    // world\x00                           string value
    // \x00                                document terminator
    const int8_t expected[] = {
        0x16, 0x00, 0x00, 0x00,
        0x02,
        'h', 'e', 'l', 'l', 'o', 0x00,
        0x06, 0x00, 0x00, 0x00,
        'w', 'o', 'r', 'l', 'd', 0x00,
        0x00,
    };

    BOOST_REQUIRE_EQUAL(doc.size(), sizeof(expected));
    BOOST_REQUIRE(std::memcmp(raw.data(), expected, sizeof(expected)) == 0);
}

// --- validate() tests ---

namespace {

// Helper: make a bytes_view from a raw uint8_t array for validate().
bytes_view bv_from(const uint8_t* p, size_t n) {
    return bytes_view(reinterpret_cast<const int8_t*>(p), n);
}

} // anonymous namespace

// validate() accepts a well-formed document produced by the writer.
BOOST_AUTO_TEST_CASE(validate_accepts_writer_output) {
    bson::writer w;
    w.add_int32("x", 42);
    w.add_string("msg", "hello");
    w.add_bool("flag", true);
    w.add_null("n");
    w.add_double("pi", 3.14);
    w.add_int64("big", int64_t(1) << 40);
    w.add_datetime("ts", 1700000000000LL);
    w.add_timestamp("t", 0xDEADBEEFULL);
    auto doc = std::move(w).finish();
    auto raw = linearize(doc);
    BOOST_REQUIRE_NO_THROW(bson::validate(bv_from(
        reinterpret_cast<const uint8_t*>(raw.data()), raw.size())));
}

// validate() accepts nested documents.
BOOST_AUTO_TEST_CASE(validate_accepts_nested) {
    bson::writer inner;
    inner.add_int32("y", 7);
    auto inner_doc = std::move(inner).finish();

    bson::writer outer;
    outer.add_document("sub", inner_doc);
    auto doc = std::move(outer).finish();
    auto raw = linearize(doc);
    BOOST_REQUIRE_NO_THROW(bson::validate(bv_from(
        reinterpret_cast<const uint8_t*>(raw.data()), raw.size())));
}

// validate() accepts nested arrays.
BOOST_AUTO_TEST_CASE(validate_accepts_array) {
    bson::writer arr;
    arr.add_string("0", "a");
    arr.add_string("1", "b");
    auto arr_doc = std::move(arr).finish();

    bson::writer outer;
    outer.add_array("items", arr_doc);
    auto doc = std::move(outer).finish();
    auto raw = linearize(doc);
    BOOST_REQUIRE_NO_THROW(bson::validate(bv_from(
        reinterpret_cast<const uint8_t*>(raw.data()), raw.size())));
}

// validate() accepts binary elements.
BOOST_AUTO_TEST_CASE(validate_accepts_binary) {
    const int8_t bin[] = {0x01, 0x02, 0x03};
    bson::writer w;
    w.add_binary("b", bytes_view(bin, 3), 0x80);
    auto doc = std::move(w).finish();
    auto raw = linearize(doc);
    BOOST_REQUIRE_NO_THROW(bson::validate(bv_from(
        reinterpret_cast<const uint8_t*>(raw.data()), raw.size())));
}

// validate() accepts an empty document (5 bytes).
BOOST_AUTO_TEST_CASE(validate_accepts_empty) {
    bson::writer w;
    auto doc = std::move(w).finish();
    auto raw = linearize(doc);
    BOOST_REQUIRE_NO_THROW(bson::validate(bv_from(
        reinterpret_cast<const uint8_t*>(raw.data()), raw.size())));
}

// Rejects data shorter than 5 bytes.
BOOST_AUTO_TEST_CASE(validate_rejects_too_short) {
    uint8_t data[] = {0x04, 0x00, 0x00, 0x00};
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, 4)), marshal_exception);
}

// Rejects mismatched length prefix.
BOOST_AUTO_TEST_CASE(validate_rejects_length_mismatch) {
    // Valid empty doc is {05 00 00 00 00}, change prefix to 06.
    uint8_t data[] = {0x06, 0x00, 0x00, 0x00, 0x00};
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, 5)), marshal_exception);
}

// Rejects missing terminator.
BOOST_AUTO_TEST_CASE(validate_rejects_no_terminator) {
    uint8_t data[] = {0x05, 0x00, 0x00, 0x00, 0x01};
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, 5)), marshal_exception);
}

// Rejects unknown element type.
BOOST_AUTO_TEST_CASE(validate_rejects_unknown_type) {
    // Fabricate: length=8, type=0x06 (undefined/deprecated), key="a\0", no value, terminator.
    uint8_t data[] = {0x08, 0x00, 0x00, 0x00, 0x06, 'a', 0x00, 0x00};
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, 8)), marshal_exception);
}

// Rejects truncated int32 value.
BOOST_AUTO_TEST_CASE(validate_rejects_truncated_int32) {
    // type=int32, key="x\0", but only 2 value bytes instead of 4.
    uint8_t data[] = {0x09, 0x00, 0x00, 0x00,
                      0x10, 'x', 0x00, 0x01, 0x00};
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, sizeof(data))), marshal_exception);
}

// Rejects truncated string (declared length extends past document).
BOOST_AUTO_TEST_CASE(validate_rejects_truncated_string) {
    // type=string, key="k\0", string length=99 (way too big).
    uint8_t data[] = {0x0C, 0x00, 0x00, 0x00,
                      0x02, 'k', 0x00,
                      0x63, 0x00, 0x00, 0x00, // string length = 99
                      0x00};
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, sizeof(data))), marshal_exception);
}

// Rejects string with length < 1.
BOOST_AUTO_TEST_CASE(validate_rejects_string_length_zero) {
    // type=string, key="k\0", string length=0 (must be >= 1).
    uint8_t data[] = {0x0C, 0x00, 0x00, 0x00,
                      0x02, 'k', 0x00,
                      0x00, 0x00, 0x00, 0x00, // string length = 0
                      0x00};
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, sizeof(data))), marshal_exception);
}

// Rejects string missing its own null terminator.
BOOST_AUTO_TEST_CASE(validate_rejects_string_no_null) {
    // type=string, key="k\0", length=2, "ab" (second byte should be 0x00).
    uint8_t data[] = {0x0E, 0x00, 0x00, 0x00,
                      0x02, 'k', 0x00,
                      0x02, 0x00, 0x00, 0x00, // string length = 2
                      'a', 'b', // 'b' should be 0x00
                      0x00};
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, sizeof(data))), marshal_exception);
}

// Rejects boolean with value other than 0x00 or 0x01.
BOOST_AUTO_TEST_CASE(validate_rejects_bad_boolean) {
    // total size = 4 + 1 + 2 + 1 + 1(terminator) = 9
    uint8_t data[] = {0x09, 0x00, 0x00, 0x00,
                      0x08, 'b', 0x00, 0x02,
                      0x00};
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, sizeof(data))), marshal_exception);
}

// Rejects unterminated key c-string.
BOOST_AUTO_TEST_CASE(validate_rejects_unterminated_key) {
    // type=null, key="abc" (no null terminator before document end).
    uint8_t data[] = {0x09, 0x00, 0x00, 0x00,
                      0x0A, 'a', 'b', 'c',
                      0x00};
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, sizeof(data))), marshal_exception);
}

// Rejects invalid embedded document (bad inner length).
BOOST_AUTO_TEST_CASE(validate_rejects_bad_embedded_doc) {
    // Outer: type=document, key="d\0", inner has wrong length prefix.
    uint8_t data[] = {0x11, 0x00, 0x00, 0x00,         // outer length = 17
                      0x03, 'd', 0x00,                 // type=document, key="d"
                      0xFF, 0x00, 0x00, 0x00,          // inner length = 255 (wrong)
                      0x10, 'x', 0x00,                 // int32 "x"
                      0x01, 0x00, 0x00, 0x00,          // value = 1
                      0x00,                             // inner terminator
                      0x00};                            // outer terminator
    BOOST_REQUIRE_THROW(bson::validate(bv_from(data, sizeof(data))), marshal_exception);
}

// from_managed_bytes() validates and accepts good data.
BOOST_AUTO_TEST_CASE(from_managed_bytes_accepts_valid) {
    bson::writer w;
    w.add_string("hello", "world");
    auto doc = std::move(w).finish();
    auto raw = linearize(doc);
    BOOST_REQUIRE_NO_THROW(bson::from_managed_bytes(managed_bytes(raw)));
}

// from_managed_bytes() rejects garbage.
BOOST_AUTO_TEST_CASE(from_managed_bytes_rejects_garbage) {
    bytes garbage = {0x01, 0x02, 0x03};
    BOOST_REQUIRE_THROW(bson::from_managed_bytes(managed_bytes(garbage)), marshal_exception);
}

// --- reader tests ---

// Helper: run a function against a reader, using with_simplified for
// optimal dispatch on both the single-fragment and multi-fragment paths.
template <typename Func>
void with_reader(const bson::document& doc, Func&& fn) {
    with_simplified(managed_bytes_view(doc.as_managed_bytes()),
        [&](FragmentedView auto v) {
            bson::reader r(v);
            fn(r);
        });
}

BOOST_AUTO_TEST_CASE(reader_empty_document) {
    bson::writer w;
    auto doc = std::move(w).finish();
    with_reader(doc, [](auto& r) {
        BOOST_REQUIRE(!r.has_next());
    });
}

BOOST_AUTO_TEST_CASE(reader_int32) {
    bson::writer w;
    w.add_int32("x", 42);
    auto doc = std::move(w).finish();
    with_reader(doc, [](auto& r) {
        BOOST_REQUIRE(r.has_next());
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e.type),
                            static_cast<uint8_t>(bson::type::int32));
        BOOST_REQUIRE_EQUAL(e.key, "x");
        BOOST_REQUIRE_EQUAL(e.as_int32(), 42);
        BOOST_REQUIRE(!r.has_next());
    });
}

BOOST_AUTO_TEST_CASE(reader_int64) {
    bson::writer w;
    w.add_int64("n", int64_t(1) << 40);
    auto doc = std::move(w).finish();
    with_reader(doc, [](auto& r) {
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e.type),
                            static_cast<uint8_t>(bson::type::int64));
        BOOST_REQUIRE_EQUAL(e.key, "n");
        BOOST_REQUIRE_EQUAL(e.as_int64(), int64_t(1) << 40);
    });
}

BOOST_AUTO_TEST_CASE(reader_double) {
    bson::writer w;
    w.add_double("pi", 3.14159);
    auto doc = std::move(w).finish();
    with_reader(doc, [](auto& r) {
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e.type),
                            static_cast<uint8_t>(bson::type::double_value));
        BOOST_REQUIRE_EQUAL(e.key, "pi");
        BOOST_REQUIRE_EQUAL(e.as_double(), 3.14159);
    });
}

BOOST_AUTO_TEST_CASE(reader_string) {
    bson::writer w;
    w.add_string("msg", "hello world");
    auto doc = std::move(w).finish();
    with_reader(doc, [](auto& r) {
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e.type),
                            static_cast<uint8_t>(bson::type::string));
        BOOST_REQUIRE_EQUAL(e.key, "msg");
        BOOST_REQUIRE_EQUAL(e.as_string(), "hello world");
    });
}

BOOST_AUTO_TEST_CASE(reader_bool) {
    bson::writer w;
    w.add_bool("t", true);
    w.add_bool("f", false);
    auto doc = std::move(w).finish();
    with_reader(doc, [](auto& r) {
        auto e1 = r.next();
        BOOST_REQUIRE_EQUAL(e1.key, "t");
        BOOST_REQUIRE_EQUAL(e1.as_bool(), true);
        auto e2 = r.next();
        BOOST_REQUIRE_EQUAL(e2.key, "f");
        BOOST_REQUIRE_EQUAL(e2.as_bool(), false);
    });
}

BOOST_AUTO_TEST_CASE(reader_null) {
    bson::writer w;
    w.add_null("n");
    auto doc = std::move(w).finish();
    with_reader(doc, [](auto& r) {
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e.type),
                            static_cast<uint8_t>(bson::type::null));
        BOOST_REQUIRE_EQUAL(e.key, "n");
    });
}

BOOST_AUTO_TEST_CASE(reader_datetime) {
    bson::writer w;
    w.add_datetime("ts", 1700000000000LL);
    auto doc = std::move(w).finish();
    with_reader(doc, [](auto& r) {
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e.type),
                            static_cast<uint8_t>(bson::type::datetime));
        BOOST_REQUIRE_EQUAL(e.as_datetime(), 1700000000000LL);
    });
}

BOOST_AUTO_TEST_CASE(reader_timestamp) {
    bson::writer w;
    w.add_timestamp("t", 0xDEADBEEF12345678ULL);
    auto doc = std::move(w).finish();
    with_reader(doc, [](auto& r) {
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(e.as_timestamp(), 0xDEADBEEF12345678ULL);
    });
}

BOOST_AUTO_TEST_CASE(reader_binary) {
    const int8_t data[] = {0x01, 0x02, 0x03, 0x04};
    bson::writer w;
    w.add_binary("b", bytes_view(data, 4), 0x80);
    auto doc = std::move(w).finish();
    with_reader(doc, [](auto& r) {
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e.type),
                            static_cast<uint8_t>(bson::type::binary));
        BOOST_REQUIRE_EQUAL(e.binary_subtype(), 0x80);
        auto bin = e.as_binary();
        BOOST_REQUIRE_EQUAL(bin.size_bytes(), 4u);
        // Verify contents by reading the sub-view.
        auto frag = bin.current_fragment();
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(frag[0]), 0x01);
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(frag[1]), 0x02);
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(frag[2]), 0x03);
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(frag[3]), 0x04);
    });
}

BOOST_AUTO_TEST_CASE(reader_nested_document) {
    bson::writer inner;
    inner.add_int32("y", 7);
    auto inner_doc = std::move(inner).finish();

    bson::writer outer;
    outer.add_document("sub", inner_doc);
    auto doc = std::move(outer).finish();

    with_reader(doc, [](auto& r) {
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e.type),
                            static_cast<uint8_t>(bson::type::document));
        BOOST_REQUIRE_EQUAL(e.key, "sub");

        // Read the embedded document via a nested reader.
        auto inner_view = e.as_document();
        bson::reader inner_r(inner_view);
        BOOST_REQUIRE(inner_r.has_next());
        auto ie = inner_r.next();
        BOOST_REQUIRE_EQUAL(ie.key, "y");
        BOOST_REQUIRE_EQUAL(ie.as_int32(), 7);
        BOOST_REQUIRE(!inner_r.has_next());
    });
}

BOOST_AUTO_TEST_CASE(reader_array) {
    bson::writer arr;
    arr.add_string("0", "a");
    arr.add_string("1", "b");
    auto arr_doc = std::move(arr).finish();

    bson::writer outer;
    outer.add_array("items", arr_doc);
    auto doc = std::move(outer).finish();

    with_reader(doc, [](auto& r) {
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e.type),
                            static_cast<uint8_t>(bson::type::array));
        BOOST_REQUIRE_EQUAL(e.key, "items");

        auto arr_view = e.as_document();
        bson::reader arr_r(arr_view);
        auto a0 = arr_r.next();
        BOOST_REQUIRE_EQUAL(a0.key, "0");
        BOOST_REQUIRE_EQUAL(a0.as_string(), "a");
        auto a1 = arr_r.next();
        BOOST_REQUIRE_EQUAL(a1.key, "1");
        BOOST_REQUIRE_EQUAL(a1.as_string(), "b");
        BOOST_REQUIRE(!arr_r.has_next());
    });
}

BOOST_AUTO_TEST_CASE(reader_multiple_elements) {
    bson::writer w;
    w.add_int32("a", 1);
    w.add_string("b", "two");
    w.add_bool("c", true);
    w.add_null("d");
    w.add_double("e", 5.0);
    auto doc = std::move(w).finish();

    with_reader(doc, [](auto& r) {
        auto e1 = r.next();
        BOOST_REQUIRE_EQUAL(e1.key, "a");
        BOOST_REQUIRE_EQUAL(e1.as_int32(), 1);

        auto e2 = r.next();
        BOOST_REQUIRE_EQUAL(e2.key, "b");
        BOOST_REQUIRE_EQUAL(e2.as_string(), "two");

        auto e3 = r.next();
        BOOST_REQUIRE_EQUAL(e3.key, "c");
        BOOST_REQUIRE_EQUAL(e3.as_bool(), true);

        auto e4 = r.next();
        BOOST_REQUIRE_EQUAL(e4.key, "d");
        BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e4.type),
                            static_cast<uint8_t>(bson::type::null));

        auto e5 = r.next();
        BOOST_REQUIRE_EQUAL(e5.key, "e");
        BOOST_REQUIRE_EQUAL(e5.as_double(), 5.0);

        BOOST_REQUIRE(!r.has_next());
    });
}

// Verify range-for iteration works.
BOOST_AUTO_TEST_CASE(reader_range_for) {
    bson::writer w;
    w.add_int32("x", 10);
    w.add_int32("y", 20);
    w.add_int32("z", 30);
    auto doc = std::move(w).finish();

    with_simplified(managed_bytes_view(doc.as_managed_bytes()),
        [](FragmentedView auto v) {
            bson::reader r(v);
            int count = 0;
            int sum = 0;
            for (auto&& e : r) {
                BOOST_REQUIRE_EQUAL(static_cast<uint8_t>(e.type),
                                    static_cast<uint8_t>(bson::type::int32));
                sum += e.as_int32();
                ++count;
            }
            BOOST_REQUIRE_EQUAL(count, 3);
            BOOST_REQUIRE_EQUAL(sum, 60);
        });
}

// Round-trip: writer → reader for the BSON spec example {"hello": "world"}.
BOOST_AUTO_TEST_CASE(reader_spec_example_roundtrip) {
    bson::writer w;
    w.add_string("hello", "world");
    auto doc = std::move(w).finish();

    with_reader(doc, [](auto& r) {
        auto e = r.next();
        BOOST_REQUIRE_EQUAL(e.key, "hello");
        BOOST_REQUIRE_EQUAL(e.as_string(), "world");
        BOOST_REQUIRE(!r.has_next());
    });
}
