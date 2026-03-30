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
