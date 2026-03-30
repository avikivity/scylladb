# Copyright 2025-present ScyllaDB
#
# SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0

#############################################################################
# Tests for the json CQL data type (BSON-backed document storage) with
# field selection and array access using the (?type)col.field[idx] syntax.
#############################################################################

import pytest
from cassandra.protocol import InvalidRequest
from .util import unique_name, unique_key_int, new_test_table


# --- SELECT tests: (?type)doc.field and (?type)doc.arr[idx] in selectors ---

# Extract an integer field from a JSON document.
def test_json_select_field_int(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc) VALUES ({p}, {{'x': 42, 'y': 'hello'}})")
        assert list(cql.execute(f"SELECT (?int)doc.x FROM {table} WHERE p={p}")) == [(42,)]

# Extract a text field from a JSON document.
def test_json_select_field_text(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc) VALUES ({p}, {{'name': 'Alice'}})")
        assert list(cql.execute(f"SELECT (?text)doc.name FROM {table} WHERE p={p}")) == [('Alice',)]

# Missing field returns NULL.
def test_json_select_field_missing(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc) VALUES ({p}, {{'a': 1}})")
        assert list(cql.execute(f"SELECT (?int)doc.nonexistent FROM {table} WHERE p={p}")) == [(None,)]

# Nested field access: (?text)doc.outer.inner
def test_json_select_field_nested(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc) VALUES ({p}, {{'outer': {{'inner': 'deep'}}}})")
        assert list(cql.execute(f"SELECT (?text)doc.outer.inner FROM {table} WHERE p={p}")) == [('deep',)]

# Array subscript: (?int)doc.arr[1]
def test_json_select_array_subscript(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc) VALUES ({p}, {{'arr': [10, 20, 30]}})")
        assert list(cql.execute(f"SELECT (?int)doc.arr[1] FROM {table} WHERE p={p}")) == [(20,)]

# Array subscript out of range returns NULL.
def test_json_select_array_oob(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc) VALUES ({p}, {{'arr': [10]}})")
        assert list(cql.execute(f"SELECT (?int)doc.arr[99] FROM {table} WHERE p={p}")) == [(None,)]

# Mixed path: (?bigint)doc.users[1].age
def test_json_select_mixed_path(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc) VALUES ({p}, "
                    f"{{'users': [{{'name': 'Bob', 'age': 25}}, {{'name': 'Eve', 'age': 30}}]}})")
        assert list(cql.execute(f"SELECT (?bigint)doc.users[1].age FROM {table} WHERE p={p}")) == [(30,)]

# Type mismatch returns NULL (asking for int but field is a string).
def test_json_select_type_mismatch(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc) VALUES ({p}, {{'name': 'Alice'}})")
        assert list(cql.execute(f"SELECT (?int)doc.name FROM {table} WHERE p={p}")) == [(None,)]

# Boolean extraction: (?boolean)doc.flag
def test_json_select_boolean(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc) VALUES ({p}, {{'flag': true}})")
        assert list(cql.execute(f"SELECT (?boolean)doc.flag FROM {table} WHERE p={p}")) == [(True,)]

# Double extraction: (?double)doc.val
def test_json_select_double(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc) VALUES ({p}, {{'val': 3.14}})")
        assert list(cql.execute(f"SELECT (?double)doc.val FROM {table} WHERE p={p}")) == [(3.14,)]

# NULL column: (?int)doc.x on a NULL doc returns NULL.
def test_json_select_null_column(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p) VALUES ({p})")
        assert list(cql.execute(f"SELECT (?int)doc.x FROM {table} WHERE p={p}")) == [(None,)]


# --- IF condition (LWT) tests: (?type)doc.field in UPDATE ... IF ---

# IF (?int)doc.field = value — condition matches, update applied.
def test_json_if_field_int_match(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json, v int") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc, v) VALUES ({p}, {{'age': 42}}, 0)")
        cql.execute(f"UPDATE {table} SET v = 1 WHERE p = {p} IF (?int)doc.age = 42")
        assert list(cql.execute(f"SELECT v FROM {table} WHERE p={p}")) == [(1,)]

# IF (?int)doc.field = value — condition does not match, update not applied.
def test_json_if_field_int_no_match(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json, v int") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc, v) VALUES ({p}, {{'age': 42}}, 0)")
        cql.execute(f"UPDATE {table} SET v = 1 WHERE p = {p} IF (?int)doc.age = 99")
        assert list(cql.execute(f"SELECT v FROM {table} WHERE p={p}")) == [(0,)]

# IF (?text)doc.name = 'Alice' — text field comparison.
def test_json_if_field_text(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json, v int") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc, v) VALUES ({p}, {{'name': 'Alice'}}, 0)")
        cql.execute(f"UPDATE {table} SET v = 1 WHERE p = {p} IF (?text)doc.name = 'Alice'")
        assert list(cql.execute(f"SELECT v FROM {table} WHERE p={p}")) == [(1,)]

# IF (?int)doc.nested.val = 7 — nested field access.
def test_json_if_field_nested(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json, v int") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc, v) VALUES ({p}, {{'nested': {{'val': 7}}}}, 0)")
        cql.execute(f"UPDATE {table} SET v = 1 WHERE p = {p} IF (?int)doc.nested.val = 7")
        assert list(cql.execute(f"SELECT v FROM {table} WHERE p={p}")) == [(1,)]

# IF (?int)doc.arr[1] = 20 — array subscript in condition.
def test_json_if_field_array(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json, v int") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc, v) VALUES ({p}, {{'arr': [10, 20, 30]}}, 0)")
        cql.execute(f"UPDATE {table} SET v = 1 WHERE p = {p} IF (?int)doc.arr[1] = 20")
        assert list(cql.execute(f"SELECT v FROM {table} WHERE p={p}")) == [(1,)]

# IF (?int)doc.missing = null — missing field yields NULL, NULL = NULL is true in LWT.
def test_json_if_field_missing_null(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json, v int") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc, v) VALUES ({p}, {{'a': 1}}, 0)")
        cql.execute(f"UPDATE {table} SET v = 1 WHERE p = {p} IF (?int)doc.missing = null")
        assert list(cql.execute(f"SELECT v FROM {table} WHERE p={p}")) == [(1,)]

# IF (?int)doc.field != value — not-equal operator.
def test_json_if_field_neq(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json, v int") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc, v) VALUES ({p}, {{'x': 5}}, 0)")
        cql.execute(f"UPDATE {table} SET v = 1 WHERE p = {p} IF (?int)doc.x != 99")
        assert list(cql.execute(f"SELECT v FROM {table} WHERE p={p}")) == [(1,)]

# IF (?text)doc[0] = 'zero' — plain subscript on BSON column.
def test_json_if_subscript_only(cql, test_keyspace, scylla_only):
    with new_test_table(cql, test_keyspace, "p int PRIMARY KEY, doc json, v int") as table:
        p = unique_key_int()
        cql.execute(f"INSERT INTO {table}(p, doc, v) VALUES ({p}, {{'0': 'zero', '1': 'one'}}, 0)")
        cql.execute(f"UPDATE {table} SET v = 1 WHERE p = {p} IF (?text)doc[0] = 'zero'")
        assert list(cql.execute(f"SELECT v FROM {table} WHERE p={p}")) == [(1,)]
