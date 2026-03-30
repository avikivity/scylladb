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

// --- BSON field selection tests ---

// Simple field extraction: (?int)doc.field
SEASTAR_TEST_CASE(json_field_select_int) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs1 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs1 (id, doc) VALUES (1, {'x': 42, 'y': 'hello'})").get();

        auto msg = e.execute_cql("SELECT (?int)doc.x FROM ks.jfs1 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{
            int32_type->decompose(int32_t(42))
        }});
    });
}

// String field extraction: (?text)doc.field
SEASTAR_TEST_CASE(json_field_select_text) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs2 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs2 (id, doc) VALUES (1, {'name': 'Alice'})").get();

        auto msg = e.execute_cql("SELECT (?text)doc.name FROM ks.jfs2 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{
            utf8_type->decompose(sstring("Alice"))
        }});
    });
}

// Missing field returns NULL.
SEASTAR_TEST_CASE(json_field_select_missing) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs3 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs3 (id, doc) VALUES (1, {'a': 1})").get();

        auto msg = e.execute_cql("SELECT (?int)doc.nonexistent FROM ks.jfs3 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{bytes_opt()}});
    });
}

// Nested field access: (?text)doc.outer.inner
SEASTAR_TEST_CASE(json_field_select_nested) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs4 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs4 (id, doc) VALUES (1, {'outer': {'inner': 'deep'}})").get();

        auto msg = e.execute_cql("SELECT (?text)doc.outer.inner FROM ks.jfs4 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{
            utf8_type->decompose(sstring("deep"))
        }});
    });
}

// Array subscript: (?int)doc.arr[1]
SEASTAR_TEST_CASE(json_field_select_array) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs5 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs5 (id, doc) VALUES (1, {'arr': [10, 20, 30]})").get();

        auto msg = e.execute_cql("SELECT (?int)doc.arr[1] FROM ks.jfs5 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{
            int32_type->decompose(int32_t(20))
        }});
    });
}

// Array subscript out of range returns NULL.
SEASTAR_TEST_CASE(json_field_select_array_oob) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs6 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs6 (id, doc) VALUES (1, {'arr': [10]})").get();

        auto msg = e.execute_cql("SELECT (?int)doc.arr[99] FROM ks.jfs6 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{bytes_opt()}});
    });
}

// Mixed path: (?bigint)doc.users[0].age
SEASTAR_TEST_CASE(json_field_select_mixed_path) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs7 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs7 (id, doc) VALUES (1, "
                      "{'users': [{'name': 'Bob', 'age': 25}, {'name': 'Eve', 'age': 30}]})").get();

        auto msg = e.execute_cql("SELECT (?bigint)doc.users[1].age FROM ks.jfs7 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{
            long_type->decompose(int64_t(30))
        }});
    });
}

// Type mismatch returns NULL: asking for int but field is a string.
SEASTAR_TEST_CASE(json_field_select_type_mismatch) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs8 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs8 (id, doc) VALUES (1, {'name': 'Alice'})").get();

        auto msg = e.execute_cql("SELECT (?int)doc.name FROM ks.jfs8 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{bytes_opt()}});
    });
}

// Boolean extraction: (?boolean)doc.flag
SEASTAR_TEST_CASE(json_field_select_boolean) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs9 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs9 (id, doc) VALUES (1, {'flag': true})").get();

        auto msg = e.execute_cql("SELECT (?boolean)doc.flag FROM ks.jfs9 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{
            boolean_type->decompose(true)
        }});
    });
}

// Sub-document extraction as json: (?json)doc.sub
SEASTAR_TEST_CASE(json_field_select_subdoc) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs10 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs10 (id, doc) VALUES (1, {'sub': {'a': 1}})").get();

        bson::writer inner;
        inner.add_int32("a", 1);
        auto inner_doc = std::move(inner).finish();
        auto exp = bytes_opt(to_bytes(managed_bytes_view(inner_doc.as_managed_bytes())));

        auto msg = e.execute_cql("SELECT (?json)doc.sub FROM ks.jfs10 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{exp}});
    });
}

// Double extraction: (?double)doc.val
SEASTAR_TEST_CASE(json_field_select_double) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs11 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs11 (id, doc) VALUES (1, {'val': 3.14})").get();

        auto msg = e.execute_cql("SELECT (?double)doc.val FROM ks.jfs11 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{
            double_type->decompose(3.14)
        }});
    });
}

// NULL column: (?int)doc.x on a NULL doc returns NULL.
SEASTAR_TEST_CASE(json_field_select_null_column) {
    return do_with_cql_env_thread([] (cql_test_env& e) {
        e.execute_cql("CREATE TABLE ks.jfs12 (id int PRIMARY KEY, doc json)").get();
        e.execute_cql("INSERT INTO ks.jfs12 (id, doc) VALUES (1, null)").get();

        auto msg = e.execute_cql("SELECT (?int)doc.x FROM ks.jfs12 WHERE id = 1").get();
        assert_that(msg).is_rows().with_rows({{bytes_opt()}});
    });
}
