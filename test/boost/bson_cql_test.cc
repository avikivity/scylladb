/*
 * Copyright (C) 2025-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// CQL-level tests for the json (BSON) type: JSON literal syntax,
// CREATE TABLE with json columns, INSERT/SELECT round-trips.

#include <boost/test/unit_test.hpp>

#undef SEASTAR_TESTING_MAIN
#include <seastar/testing/test_case.hh>
#include "test/lib/cql_test_env.hh"
#include "test/lib/cql_assertions.hh"

#include "types/types.hh"
#include "types/concrete_types.hh"
#include "utils/bson.hh"

namespace {

// Build expected BSON bytes and wrap in bytes_opt for assertion comparison.
// The bson_type serialization is identity (managed_bytes round-trip).
bytes_opt expected_bson(bson::document doc) {
    auto dv = static_cast<const bson_type_impl&>(*bson_type).make_value(std::move(doc));
    auto mb = bson_type->decompose(dv);
    return to_bytes(managed_bytes_view(mb));
}

} // anonymous namespace

// CREATE TABLE with a json column and INSERT a simple map literal.
SEASTAR_TEST_CASE(json_literal_simple_map) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jt1 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jt1 (id, doc) VALUES (1, {'hello': 'world'})").get();

        bson::writer w;
        w.add_string("hello", "world");
        auto exp = expected_bson(std::move(w).finish());

        auto msg = e.execute_cql("SELECT doc FROM ks.jt1 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{exp}});
    });
}

// Integer values: int32 when fits, int64 when large.
SEASTAR_TEST_CASE(json_literal_integers) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jt2 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jt2 (id, doc) VALUES (1, {'small': 42, 'big': 3000000000})").get();

        bson::writer w;
        w.add_int32("small", 42);
        w.add_int64("big", 3000000000LL);
        auto exp = expected_bson(std::move(w).finish());

        auto msg = e.execute_cql("SELECT doc FROM ks.jt2 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{exp}});
    });
}

// Float, boolean, and null values.
SEASTAR_TEST_CASE(json_literal_mixed_scalars) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jt3 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jt3 (id, doc) VALUES (1, {'pi': 3.14, 'flag': true, 'nothing': null})").get();

        bson::writer w;
        w.add_double("pi", 3.14);
        w.add_bool("flag", true);
        w.add_null("nothing");
        auto exp = expected_bson(std::move(w).finish());

        auto msg = e.execute_cql("SELECT doc FROM ks.jt3 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{exp}});
    });
}

// Nested document.
SEASTAR_TEST_CASE(json_literal_nested_document) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jt4 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jt4 (id, doc) VALUES (1, {'outer': {'inner': 1}})").get();

        bson::writer inner;
        inner.add_int32("inner", 1);
        bson::writer outer;
        outer.add_document("outer", std::move(inner).finish());
        auto exp = expected_bson(std::move(outer).finish());

        auto msg = e.execute_cql("SELECT doc FROM ks.jt4 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{exp}});
    });
}

// Array (list literal) as a value.
SEASTAR_TEST_CASE(json_literal_array_value) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jt5 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jt5 (id, doc) VALUES (1, {'tags': [1, 2, 3]})").get();

        bson::writer arr;
        arr.add_int32("0", 1);
        arr.add_int32("1", 2);
        arr.add_int32("2", 3);
        bson::writer doc;
        doc.add_array("tags", std::move(arr).finish());
        auto exp = expected_bson(std::move(doc).finish());

        auto msg = e.execute_cql("SELECT doc FROM ks.jt5 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{exp}});
    });
}

// Empty document {}.
SEASTAR_TEST_CASE(json_literal_empty_document) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jt6 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jt6 (id, doc) VALUES (1, {})").get();

        bson::writer w;
        auto exp = expected_bson(std::move(w).finish());

        auto msg = e.execute_cql("SELECT doc FROM ks.jt6 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{exp}});
    });
}

// Complex nested structure with mixed types.
SEASTAR_TEST_CASE(json_literal_complex) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jt7 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql(
            "INSERT INTO ks.jt7 (id, doc) VALUES (1, "
            "{'name': 'Alice', 'age': 30, 'scores': [95, 87], 'addr': {'city': 'NYC'}})"
        ).get();

        bson::writer scores;
        scores.add_int32("0", 95);
        scores.add_int32("1", 87);
        bson::writer addr;
        addr.add_string("city", "NYC");
        bson::writer doc;
        doc.add_string("name", "Alice");
        doc.add_int32("age", 30);
        doc.add_array("scores", std::move(scores).finish());
        doc.add_document("addr", std::move(addr).finish());
        auto exp = expected_bson(std::move(doc).finish());

        auto msg = e.execute_cql("SELECT doc FROM ks.jt7 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{exp}});
    });
}

// NULL value for a json column works.
SEASTAR_TEST_CASE(json_literal_null_column) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jt8 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jt8 (id, doc) VALUES (1, null)").get();

        auto msg = e.execute_cql("SELECT doc FROM ks.jt8 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{bytes_opt()}});
    });
}

// Negative integer.
SEASTAR_TEST_CASE(json_literal_negative_int) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jt9 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jt9 (id, doc) VALUES (1, {'v': -42})").get();

        bson::writer w;
        w.add_int32("v", -42);
        auto exp = expected_bson(std::move(w).finish());

        auto msg = e.execute_cql("SELECT doc FROM ks.jt9 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{exp}});
    });
}
