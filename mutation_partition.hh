/*
 * Copyright (C) 2014 ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <iosfwd>

#include <seastar/core/bitset-iter.hh>
#include <seastar/util/optimized_optional.hh>

#include "keys.hh"
#include "position_in_partition.hh"
#include "atomic_cell_or_collection.hh"
#include "query-result.hh"
#include "mutation_partition_view.hh"
#include "utils/managed_vector.hh"
#include "range_tombstone_list.hh"
#include "clustering_key_filter.hh"
#include "utils/with_relational_operators.hh"
#include "utils/managed_ref.hh"

class mutation_fragment;
class clustering_row;

struct cell_hash {
    using size_type = uint64_t;
    static constexpr size_type no_hash = 0;

    size_type hash = no_hash;

    explicit operator bool() const noexcept;
};

using cell_hash_opt = seastar::optimized_optional<cell_hash>;

struct cell_and_hash {
    atomic_cell_or_collection cell;
    mutable cell_hash_opt hash;

    cell_and_hash() = default;
    cell_and_hash(cell_and_hash&&) noexcept = default;
    cell_and_hash& operator=(cell_and_hash&&) noexcept = default;

    cell_and_hash(atomic_cell_or_collection&& cell, cell_hash_opt hash);
};

class compaction_garbage_collector;

//
// Container for cells of a row. Cells are identified by column_id.
//
// All cells must belong to a single column_kind. The kind is not stored
// for space-efficiency reasons. Whenever a method accepts a column_kind,
// the caller must always supply the same column_kind.
//
// Can be used as a range of row::cell_entry.
//
class row {

    class cell_entry {
        friend class row;
    public:
        cell_entry(column_id id, cell_and_hash c_a_h);
        cell_entry(column_id id, atomic_cell_or_collection cell);
        cell_entry(column_id id);
        cell_entry(cell_entry&&) noexcept;
        cell_entry(const abstract_type&, const cell_entry&);

        column_id id() const;
        const atomic_cell_or_collection& cell() const;
        atomic_cell_or_collection& cell();
        const cell_hash_opt& hash() const;
        const cell_and_hash& get_cell_and_hash() const;
        cell_and_hash& get_cell_and_hash();

        struct compare {
            bool operator()(const cell_entry& e1, const cell_entry& e2) const ;
            bool operator()(column_id id1, const cell_entry& e2) const;
            bool operator()(const cell_entry& e1, column_id id2) const;
        };
    };

    using size_type = std::make_unsigned_t<column_id>;

public:
    static constexpr size_t max_vector_size = 32;
    static constexpr size_t internal_count = 5;
private:
public:
    row();
    ~row();
    row(const schema&, column_kind, const row&);
    row(row&& other) noexcept;
    row& operator=(row&& other) noexcept;
    size_t size() const;
    bool empty() const;

    void reserve(column_id);

    const atomic_cell_or_collection& cell_at(column_id id) const;

    // Returns a pointer to cell's value or nullptr if column is not set.
    const atomic_cell_or_collection* find_cell(column_id id) const;
    // Returns a pointer to cell's value and hash or nullptr if column is not set.
    const cell_and_hash* find_cell_and_hash(column_id id) const;
private:
    template<typename Func>
    void remove_if(Func&& func);
private:
    template<typename Func>
    auto with_both_ranges(const row& other, Func&& func) const;

    void vector_to_set();

    template<typename Func>
    void consume_with(Func&&);

public:
    // Calls Func(column_id, cell_and_hash&) or Func(column_id, atomic_cell_and_collection&)
    // for each cell in this row, depending on the concrete Func type.
    // noexcept if Func doesn't throw.
    template<typename Func>
    void for_each_cell(Func&& func);

    template<typename Func>
    void for_each_cell(Func&& func) const;

    template<typename Func>
    void for_each_cell_until(Func&& func) const;

    // Merges cell's value into the row.
    // Weak exception guarantees.
    void apply(const column_definition& column, const atomic_cell_or_collection& cell, cell_hash_opt hash = cell_hash_opt());

    // Merges cell's value into the row.
    // Weak exception guarantees.
    void apply(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());

    // Monotonic exception guarantees. In case of exception the sum of cell and this remains the same as before the exception.
    void apply_monotonically(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());

    // Adds cell to the row. The column must not be already set.
    void append_cell(column_id id, atomic_cell_or_collection cell);

    // Weak exception guarantees
    void apply(const schema&, column_kind, const row& src);
    // Weak exception guarantees
    void apply(const schema&, column_kind, row&& src);
    // Monotonic exception guarantees
    void apply_monotonically(const schema&, column_kind, row&& src);

    // Expires cells based on query_time. Expires tombstones based on gc_before
    // and max_purgeable. Removes cells covered by tomb.
    // Returns true iff there are any live cells left.
    bool compact_and_expire(
            const schema& s,
            column_kind kind,
            row_tombstone tomb,
            gc_clock::time_point query_time,
            can_gc_fn&,
            gc_clock::time_point gc_before,
            const row_marker& marker,
            compaction_garbage_collector* collector = nullptr);

    bool compact_and_expire(
            const schema& s,
            column_kind kind,
            row_tombstone tomb,
            gc_clock::time_point query_time,
            can_gc_fn&,
            gc_clock::time_point gc_before,
            compaction_garbage_collector* collector = nullptr);

    row difference(const schema&, column_kind, const row& other) const;

    bool equal(column_kind kind, const schema& this_schema, const row& other, const schema& other_schema) const;

    size_t external_memory_usage(const schema&, column_kind) const;

    cell_hash_opt cell_hash_for(column_id id) const;

    void prepare_hash(const schema& s, column_kind kind) const;
    void clear_hash() const;

    bool is_live(const schema&, column_kind kind, tombstone tomb = tombstone(), gc_clock::time_point now = gc_clock::time_point::min()) const;

    class printer {
    public:
        printer(const schema& s, column_kind k, const row& r);
        printer(const printer&) = delete;
        printer(printer&&) = delete;

        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
};

// Like row, but optimized for the case where the row doesn't exist (e.g. static rows)
class lazy_row {
public:
    lazy_row() = default;
    explicit lazy_row(row&& r);
    lazy_row(const schema& s, column_kind kind, const lazy_row& r);
    lazy_row(const schema& s, column_kind kind, const row& r);
    row& maybe_create();

    const row& get_existing() const &;
    row& get_existing() &;
    row&& get_existing() &&;
    const row& get() const;
    size_t size() const;

    bool empty() const;

    void reserve(column_id nr);

    const atomic_cell_or_collection& cell_at(column_id id) const;

    // Returns a pointer to cell's value or nullptr if column is not set.
    const atomic_cell_or_collection* find_cell(column_id id) const;

    // Returns a pointer to cell's value and hash or nullptr if column is not set.
    const cell_and_hash* find_cell_and_hash(column_id id) const;

    // Calls Func(column_id, cell_and_hash&) or Func(column_id, atomic_cell_and_collection&)
    // for each cell in this row, depending on the concrete Func type.
    // noexcept if Func doesn't throw.
    template<typename Func>
    void for_each_cell(Func&& func);

    template<typename Func>
    void for_each_cell(Func&& func) const;

    template<typename Func>
    void for_each_cell_until(Func&& func) const;

    // Merges cell's value into the row.
    // Weak exception guarantees.
    void apply(const column_definition& column, const atomic_cell_or_collection& cell, cell_hash_opt hash = cell_hash_opt());
    // Merges cell's value into the row.
    // Weak exception guarantees.
    void apply(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());

    // Monotonic exception guarantees. In case of exception the sum of cell and this remains the same as before the exception.
    void apply_monotonically(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());
    // Adds cell to the row. The column must not be already set.
    void append_cell(column_id id, atomic_cell_or_collection cell);

    // Weak exception guarantees
    void apply(const schema& s, column_kind kind, const row& src);
    // Weak exception guarantees
    void apply(const schema& s, column_kind kind, const lazy_row& src);

    // Weak exception guarantees
    void apply(const schema& s, column_kind kind, row&& src);

    // Monotonic exception guarantees
    void apply_monotonically(const schema& s, column_kind kind, row&& src);
    // Monotonic exception guarantees
    void apply_monotonically(const schema& s, column_kind kind, lazy_row&& src);
    // Expires cells based on query_time. Expires tombstones based on gc_before
    // and max_purgeable. Removes cells covered by tomb.
    // Returns true iff there are any live cells left.
    bool compact_and_expire(
            const schema& s,
            column_kind kind,
            row_tombstone tomb,
            gc_clock::time_point query_time,
            can_gc_fn& can_gc,
            gc_clock::time_point gc_before,
            const row_marker& marker,
            compaction_garbage_collector* collector = nullptr);

    bool compact_and_expire(
            const schema& s,
            column_kind kind,
            row_tombstone tomb,
            gc_clock::time_point query_time,
            can_gc_fn& can_gc,
            gc_clock::time_point gc_before,
            compaction_garbage_collector* collector = nullptr);

    lazy_row difference(const schema& s, column_kind kind, const lazy_row& other) const;
    bool equal(column_kind kind, const schema& this_schema, const lazy_row& other, const schema& other_schema) const;
    size_t external_memory_usage(const schema& s, column_kind kind) const;

    cell_hash_opt cell_hash_for(column_id id) const;
    void prepare_hash(const schema& s, column_kind kind) const;

    void clear_hash() const;

    bool is_live(const schema& s, column_kind kind, tombstone tomb = tombstone(), gc_clock::time_point now = gc_clock::time_point::min()) const;

    class printer {
    public:
        printer(const schema& s, column_kind k, const lazy_row& r);
        printer(const printer&) = delete;
        printer(printer&&) = delete;

        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
};

class row_marker;
int compare_row_marker_for_merge(const row_marker& left, const row_marker& right) noexcept;

class row_marker {
    static constexpr gc_clock::duration no_ttl { 0 };
    static constexpr gc_clock::duration dead { -1 };
    static constexpr gc_clock::time_point no_expiry { gc_clock::duration(0) };
    api::timestamp_type _timestamp = api::missing_timestamp;
    gc_clock::duration _ttl = no_ttl;
    gc_clock::time_point _expiry = no_expiry;
public:
    row_marker() = default;
    explicit row_marker(api::timestamp_type created_at) : _timestamp(created_at) { }
    row_marker(api::timestamp_type created_at, gc_clock::duration ttl, gc_clock::time_point expiry)
        : _timestamp(created_at), _ttl(ttl), _expiry(expiry)
    { }
    explicit row_marker(tombstone deleted_at)
        : _timestamp(deleted_at.timestamp), _ttl(dead), _expiry(deleted_at.deletion_time)
    { }
    bool is_missing() const {
        return _timestamp == api::missing_timestamp;
    }
    bool is_live() const {
        return !is_missing() && _ttl != dead;
    }
    bool is_live(tombstone t, gc_clock::time_point now) const {
        if (is_missing() || _ttl == dead) {
            return false;
        }
        if (_ttl != no_ttl && _expiry <= now) {
            return false;
        }
        return _timestamp > t.timestamp;
    }
    // Can be called only when !is_missing().
    bool is_dead(gc_clock::time_point now) const {
        if (_ttl == dead) {
            return true;
        }
        return _ttl != no_ttl && _expiry <= now;
    }
    // Can be called only when is_live().
    bool is_expiring() const {
        return _ttl != no_ttl;
    }
    // Can be called only when is_expiring().
    gc_clock::duration ttl() const {
        return _ttl;
    }
    // Can be called only when is_expiring().
    gc_clock::time_point expiry() const {
        return _expiry;
    }
    // Should be called when is_dead() or is_expiring().
    // Safe to be called when is_missing().
    // When is_expiring(), returns the the deletion time of the marker when it finally expires.
    gc_clock::time_point deletion_time() const {
        return _ttl == dead ? _expiry : _expiry - _ttl;
    }
    api::timestamp_type timestamp() const {
        return _timestamp;
    }
    void apply(const row_marker& rm) {
        if (compare_row_marker_for_merge(*this, rm) < 0) {
            *this = rm;
        }
    }
    // Expires cells and tombstones. Removes items covered by higher level
    // tombstones.
    // Returns true if row marker is live.
    bool compact_and_expire(tombstone tomb, gc_clock::time_point now,
            can_gc_fn& can_gc, gc_clock::time_point gc_before, compaction_garbage_collector* collector = nullptr);
    // Consistent with feed_hash()
    bool operator==(const row_marker& other) const {
        if (_timestamp != other._timestamp) {
            return false;
        }
        if (is_missing()) {
            return true;
        }
        if (_ttl != other._ttl) {
            return false;
        }
        return _ttl == no_ttl || _expiry == other._expiry;
    }
    bool operator!=(const row_marker& other) const {
        return !(*this == other);
    }
    // Consistent with operator==()
    template<typename Hasher>
    void feed_hash(Hasher& h) const {
        ::feed_hash(h, _timestamp);
        if (!is_missing()) {
            ::feed_hash(h, _ttl);
            if (_ttl != no_ttl) {
                ::feed_hash(h, _expiry);
            }
        }
    }
    friend std::ostream& operator<<(std::ostream& os, const row_marker& rm);
};


class clustering_row;

class shadowable_tombstone : public with_relational_operators<shadowable_tombstone> {
    tombstone _tomb;
public:

    explicit shadowable_tombstone(api::timestamp_type timestamp, gc_clock::time_point deletion_time)
            : _tomb(timestamp, deletion_time) {
    }

    explicit shadowable_tombstone(tombstone tomb = tombstone())
            : _tomb(std::move(tomb)) {
    }

    int compare(const shadowable_tombstone& t) const {
        return _tomb.compare(t._tomb);
    }

    explicit operator bool() const {
        return bool(_tomb);
    }

    const tombstone& tomb() const {
        return _tomb;
    }

    // A shadowable row tombstone is valid only if the row has no live marker. In other words,
    // the row tombstone is only valid as long as no newer insert is done (thus setting a
    // live row marker; note that if the row timestamp set is lower than the tombstone's,
    // then the tombstone remains in effect as usual). If a row has a shadowable tombstone
    // with timestamp Ti and that row is updated with a timestamp Tj, such that Tj > Ti
    // (and that update sets the row marker), then the shadowable tombstone is shadowed by
    // that update. A concrete consequence is that if the update has cells with timestamp
    // lower than Ti, then those cells are preserved (since the deletion is removed), and
    // this is contrary to a regular, non-shadowable row tombstone where the tombstone is
    // preserved and such cells are removed.
    bool is_shadowed_by(const row_marker& marker) const {
        return marker.is_live() && marker.timestamp() > _tomb.timestamp;
    }

    void maybe_shadow(tombstone t, row_marker marker) noexcept {
        if (is_shadowed_by(marker)) {
            _tomb = std::move(t);
        }
    }

    void apply(tombstone t) noexcept {
        _tomb.apply(t);
    }

    void apply(shadowable_tombstone t) noexcept {
        _tomb.apply(t._tomb);
    }

    friend std::ostream& operator<<(std::ostream& out, const shadowable_tombstone& t) {
        if (t) {
            return out << "{shadowable tombstone: timestamp=" << t.tomb().timestamp
                   << ", deletion_time=" << t.tomb().deletion_time.time_since_epoch().count()
                   << "}";
        } else {
            return out << "{shadowable tombstone: none}";
        }
    }
};


/*
The rules for row_tombstones are as follows:
  - The shadowable tombstone is always >= than the regular one;
  - The regular tombstone works as expected;
  - The shadowable tombstone doesn't erase or compact away the regular
    row tombstone, nor dead cells;
  - The shadowable tombstone can erase live cells, but only provided they
    can be recovered (e.g., by including all cells in a MV update, both
    updated cells and pre-existing ones);
  - The shadowable tombstone can be erased or compacted away by a newer
    row marker.
*/
class row_tombstone : public with_relational_operators<row_tombstone> {
    tombstone _regular;
    shadowable_tombstone _shadowable; // _shadowable is always >= _regular
public:
    explicit row_tombstone(tombstone regular, shadowable_tombstone shadowable)
            : _regular(std::move(regular))
            , _shadowable(std::move(shadowable)) {
    }

    explicit row_tombstone(tombstone regular)
            : row_tombstone(regular, shadowable_tombstone(regular)) {
    }

    row_tombstone() = default;

    int compare(const row_tombstone& t) const {
        return _shadowable.compare(t._shadowable);
    }

    explicit operator bool() const {
        return bool(_shadowable);
    }

    const tombstone& tomb() const {
        return _shadowable.tomb();
    }

    const gc_clock::time_point max_deletion_time() const {
        return std::max(_regular.deletion_time, _shadowable.tomb().deletion_time);
    }

    const tombstone& regular() const {
        return _regular;
    }

    const shadowable_tombstone& shadowable() const {
        return _shadowable;
    }

    bool is_shadowable() const {
        return _shadowable.tomb() > _regular;
    }

    void maybe_shadow(const row_marker& marker) noexcept {
        _shadowable.maybe_shadow(_regular, marker);
    }

    void apply(tombstone regular) noexcept {
        _shadowable.apply(regular);
        _regular.apply(regular);
    }

    void apply(shadowable_tombstone shadowable, row_marker marker) noexcept {
        _shadowable.apply(shadowable.tomb());
        _shadowable.maybe_shadow(_regular, marker);
    }

    void apply(row_tombstone t, row_marker marker) noexcept {
        _regular.apply(t._regular);
        _shadowable.apply(t._shadowable);
        _shadowable.maybe_shadow(_regular, marker);
    }

    friend std::ostream& operator<<(std::ostream& out, const row_tombstone& t) {
        if (t) {
            return out << "{row_tombstone: " << t._regular << (t.is_shadowable() ? t._shadowable : shadowable_tombstone()) << "}";
        } else {
            return out << "{row_tombstone: none}";
        }
    }
};

class deletable_row final {
    row_tombstone _deleted_at;
    row_marker _marker;
    row _cells;
public:
    deletable_row() {}
    explicit deletable_row(clustering_row&&);
    deletable_row(const schema& s, const deletable_row& other)
        : _deleted_at(other._deleted_at)
        , _marker(other._marker)
        , _cells(s, column_kind::regular_column, other._cells)
    { }
    deletable_row(const schema& s, row_tombstone tomb, const row_marker& marker, const row& cells)
        : _deleted_at(tomb), _marker(marker), _cells(s, column_kind::regular_column, cells)
    {}

    void apply(const schema&, clustering_row);

    void apply(tombstone deleted_at) {
        _deleted_at.apply(deleted_at);
    }

    void apply(shadowable_tombstone deleted_at) {
        _deleted_at.apply(deleted_at, _marker);
    }

    void apply(row_tombstone deleted_at) {
        _deleted_at.apply(deleted_at, _marker);
    }

    void apply(const row_marker& rm) {
        _marker.apply(rm);
        _deleted_at.maybe_shadow(_marker);
    }

    void remove_tombstone() {
        _deleted_at = {};
    }

    // Weak exception guarantees. After exception, both src and this will commute to the same value as
    // they would should the exception not happen.
    void apply(const schema& s, deletable_row&& src);
    void apply_monotonically(const schema& s, deletable_row&& src);
public:
    row_tombstone deleted_at() const { return _deleted_at; }
    api::timestamp_type created_at() const { return _marker.timestamp(); }
    row_marker& marker() { return _marker; }
    const row_marker& marker() const { return _marker; }
    const row& cells() const { return _cells; }
    row& cells() { return _cells; }
    bool equal(column_kind, const schema& s, const deletable_row& other, const schema& other_schema) const;
    bool is_live(const schema& s, tombstone base_tombstone = tombstone(), gc_clock::time_point query_time = gc_clock::time_point::min()) const;
    bool empty() const { return !_deleted_at && _marker.is_missing() && !_cells.size(); }
    deletable_row difference(const schema&, column_kind, const deletable_row& other) const;

    class printer {
        const schema& _schema;
        const deletable_row& _deletable_row;
    public:
        printer(const schema& s, const deletable_row& r) : _schema(s), _deletable_row(r) { }
        printer(const printer&) = delete;
        printer(printer&&) = delete;

        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
};

class cache_tracker;

class rows_entry {
    friend class cache_tracker;
    friend class size_calculator;
    struct flags {};
    friend class mutation_partition;
public:
    struct last_dummy_tag {};
    explicit rows_entry(clustering_key&& key);
    explicit rows_entry(const clustering_key& key);
    rows_entry(const schema& s, position_in_partition_view pos, is_dummy dummy, is_continuous continuous);
    rows_entry(const schema& s, last_dummy_tag, is_continuous continuous);
    rows_entry(const clustering_key& key, deletable_row&& row);
    rows_entry(const schema& s, const clustering_key& key, const deletable_row& row);
    rows_entry(const schema& s, const clustering_key& key, row_tombstone tomb, const row_marker& marker, const row& row);
    rows_entry(rows_entry&& o) noexcept;
    rows_entry(const schema& s, const rows_entry& e);
    // Valid only if !dummy()
    clustering_key& key();
    // Valid only if !dummy()
    const clustering_key& key() const;
    deletable_row& row();
    const deletable_row& row() const;
    position_in_partition_view position() const;

    is_continuous continuous() const;
    void set_continuous(bool value);
    void set_continuous(is_continuous value);
    is_dummy dummy() const;
    bool is_last_dummy() const;
    void set_dummy(bool value);
    void set_dummy(is_dummy value);
    void apply(row_tombstone t);
    void apply_monotonically(const schema& s, rows_entry&& e);
    bool empty() const;
    struct tri_compare {
        explicit tri_compare(const schema& s);
        int operator()(const rows_entry& e1, const rows_entry& e2) const;
        int operator()(const clustering_key& key, const rows_entry& e) const;
        int operator()(const rows_entry& e, const clustering_key& key) const;
        int operator()(const rows_entry& e, position_in_partition_view p) const;
        int operator()(position_in_partition_view p, const rows_entry& e) const;
        int operator()(position_in_partition_view p1, position_in_partition_view p2) const;
    };
    struct compare {
        explicit compare(const schema& s);
        bool operator()(const rows_entry& e1, const rows_entry& e2) const;
        bool operator()(const clustering_key& key, const rows_entry& e) const;
        bool operator()(const rows_entry& e, const clustering_key& key) const;
        bool operator()(const clustering_key_view& key, const rows_entry& e) const;
        bool operator()(const rows_entry& e, const clustering_key_view& key) const;
        bool operator()(const rows_entry& e, position_in_partition_view p) const;
        bool operator()(position_in_partition_view p, const rows_entry& e) const;
        bool operator()(position_in_partition_view p1, position_in_partition_view p2) const;
    };
    bool equal(const schema& s, const rows_entry& other) const;
    bool equal(const schema& s, const rows_entry& other, const schema& other_schema) const;

    size_t memory_usage(const schema&) const;
    void on_evicted(cache_tracker&) noexcept;

    class printer {
    public:
        printer(const schema& s, const rows_entry& r);
        printer(const printer&) = delete;
        printer(printer&&) = delete;

        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
};

struct mutation_application_stats {
};

// Represents a set of writes made to a single partition.
//
// The object is schema-dependent. Each instance is governed by some
// specific schema version. Accessors require a reference to the schema object
// of that version.
//
// There is an operation of addition defined on mutation_partition objects
// (also called "apply"), which gives as a result an object representing the
// sum of writes contained in the addends. For instances governed by the same
// schema, addition is commutative and associative.
//
// In addition to representing writes, the object supports specifying a set of
// partition elements called "continuity". This set can be used to represent
// lack of information about certain parts of the partition. It can be
// specified which ranges of clustering keys belong to that set. We say that a
// key range is continuous if all keys in that range belong to the continuity
// set, and discontinuous otherwise. By default everything is continuous.
// The static row may be also continuous or not.
// Partition tombstone is always continuous.
//
// Continuity is ignored by instance equality. It's also transient, not
// preserved by serialization.
//
// Continuity is represented internally using flags on row entries. The key
// range between two consecutive entries (both ends exclusive) is continuous
// if and only if rows_entry::continuous() is true for the later entry. The
// range starting after the last entry is assumed to be continuous. The range
// corresponding to the key of the entry is continuous if and only if
// rows_entry::dummy() is false.
//
// Adding two fully-continuous instances gives a fully-continuous instance.
// Continuity doesn't affect how the write part is added.
//
// Addition of continuity is not commutative in general, but is associative.
// The default continuity merging rules are those required by MVCC to
// preserve its invariants. For details, refer to "Continuity merging rules" section
// in the doc in partition_version.hh.
class mutation_partition final {
public:
    friend class rows_entry;
    friend class size_calculator;
    friend class converting_mutation_partition_applier;
public:
    struct copy_comparators_only {};
    struct incomplete_tag {};
    // Constructs an empty instance which is fully discontinuous except for the partition tombstone.
    mutation_partition(incomplete_tag, const schema& s, tombstone);
    static mutation_partition make_incomplete(const schema& s, tombstone t = {});
    mutation_partition(schema_ptr s);
    mutation_partition(mutation_partition& other, copy_comparators_only);
    mutation_partition(mutation_partition&&) = default;
    mutation_partition(const schema& s, const mutation_partition&);
    mutation_partition(const mutation_partition&, const schema&, query::clustering_key_filter_ranges);
    mutation_partition(mutation_partition&&, const schema&, query::clustering_key_filter_ranges);
    ~mutation_partition();
    mutation_partition& operator=(mutation_partition&& x) noexcept;
    bool equal(const schema&, const mutation_partition&) const;
    bool equal(const schema& this_schema, const mutation_partition& p, const schema& p_schema) const;
    bool equal_continuity(const schema&, const mutation_partition&) const;
    // Consistent with equal()
    template<typename Hasher>
    void feed_hash(Hasher& h, const schema& s) const;

    class printer {
    public:
        printer(const schema& s, const mutation_partition& mp);
        printer(const printer&) = delete;
        printer(printer&&) = delete;

        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
public:
    // Makes sure there is a dummy entry after all clustered rows. Doesn't affect continuity.
    // Doesn't invalidate iterators.
    void ensure_last_dummy(const schema&);
    bool static_row_continuous() const;
    void set_static_row_continuous(bool value);
    bool is_fully_continuous() const;
    void make_fully_continuous();
    // Sets or clears continuity of clustering ranges between existing rows.
    void set_continuity(const schema&, const position_range& pr, is_continuous);
    // Returns clustering row ranges which have continuity matching the is_continuous argument.
    clustering_interval_set get_continuity(const schema&, is_continuous = is_continuous::yes) const;
    // Returns true iff all keys from given range are marked as continuous, or range is empty.
    bool fully_continuous(const schema&, const position_range&);
    // Returns true iff all keys from given range are marked as not continuous and range is not empty.
    bool fully_discontinuous(const schema&, const position_range&);
    // Returns true iff all keys from given range have continuity membership as specified by is_continuous.
    bool check_continuity(const schema&, const position_range&, is_continuous) const;
    // Frees elements of the partition in batches.
    // Returns stop_iteration::yes iff there are no more elements to free.
    // Continuity is unspecified after this.
    stop_iteration clear_gently(cache_tracker*) noexcept;
    // Applies mutation_fragment.
    // The fragment must be goverened by the same schema as this object.
    void apply(const schema& s, const mutation_fragment&);
    void apply(tombstone t);
    void apply_delete(const schema& schema, const clustering_key_prefix& prefix, tombstone t);
    void apply_delete(const schema& schema, range_tombstone rt);
    void apply_delete(const schema& schema, clustering_key_prefix&& prefix, tombstone t);
    void apply_delete(const schema& schema, clustering_key_prefix_view prefix, tombstone t);
    // Equivalent to applying a mutation with an empty row, created with given timestamp
    void apply_insert(const schema& s, clustering_key_view, api::timestamp_type created_at);
    void apply_insert(const schema& s, clustering_key_view, api::timestamp_type created_at,
                      gc_clock::duration ttl, gc_clock::time_point expiry);
    // prefix must not be full
    void apply_row_tombstone(const schema& schema, clustering_key_prefix prefix, tombstone t);
    void apply_row_tombstone(const schema& schema, range_tombstone rt);
    //
    // Applies p to current object.
    //
    // Commutative when this_schema == p_schema. If schemas differ, data in p which
    // is not representable in this_schema is dropped, thus apply() loses commutativity.
    //
    // Weak exception guarantees.
    void apply(const schema& this_schema, const mutation_partition& p, const schema& p_schema,
            mutation_application_stats& app_stats);
    // Use in case this instance and p share the same schema.
    // Same guarantees as apply(const schema&, mutation_partition&&, const schema&);
    void apply(const schema& s, mutation_partition&& p, mutation_application_stats& app_stats);
    // Same guarantees and constraints as for apply(const schema&, const mutation_partition&, const schema&).
    void apply(const schema& this_schema, mutation_partition_view p, const schema& p_schema,
            mutation_application_stats& app_stats);

    // Applies p to this instance.
    //
    // Monotonic exception guarantees. In case of exception the sum of p and this remains the same as before the exception.
    // This instance and p are governed by the same schema.
    //
    // Must be provided with a pointer to the cache_tracker, which owns both this and p.
    //
    // Returns stop_iteration::no if the operation was preempted before finished, and stop_iteration::yes otherwise.
    // On preemption the sum of this and p stays the same (represents the same set of writes), and the state of this
    // object contains at least all the writes it contained before the call (monotonicity). It may contain partial writes.
    // Also, some progress is always guaranteed (liveness).
    //
    // The operation can be drien to completion like this:
    //
    //   while (apply_monotonically(..., is_preemtable::yes) == stop_iteration::no) { }
    //
    // If is_preemptible::no is passed as argument then stop_iteration::no is never returned.
    stop_iteration apply_monotonically(const schema& s, mutation_partition&& p, cache_tracker*,
            mutation_application_stats& app_stats, is_preemptible = is_preemptible::no);
    stop_iteration apply_monotonically(const schema& s, mutation_partition&& p, const schema& p_schema,
            mutation_application_stats& app_stats, is_preemptible = is_preemptible::no);

    // Weak exception guarantees.
    // Assumes this and p are not owned by a cache_tracker.
    void apply_weak(const schema& s, const mutation_partition& p, const schema& p_schema,
            mutation_application_stats& app_stats);
    void apply_weak(const schema& s, mutation_partition&&,
            mutation_application_stats& app_stats);
    void apply_weak(const schema& s, mutation_partition_view p, const schema& p_schema,
            mutation_application_stats& app_stats);

    // Converts partition to the new schema. When succeeds the partition should only be accessed
    // using the new schema.
    //
    // Strong exception guarantees.
    void upgrade(const schema& old_schema, const schema& new_schema);
private:
    void insert_row(const schema& s, const clustering_key& key, deletable_row&& row);
    void insert_row(const schema& s, const clustering_key& key, const deletable_row& row);

    uint32_t do_compact(const schema& s,
        gc_clock::time_point now,
        const std::vector<query::clustering_range>& row_ranges,
        bool always_return_static_content,
        bool reverse,
        uint32_t row_limit,
        can_gc_fn&);

    // Calls func for each row entry inside row_ranges until func returns stop_iteration::yes.
    // Removes all entries for which func didn't return stop_iteration::no or wasn't called at all.
    // Removes all entries that are empty, check rows_entry::empty().
    // If reversed is true, func will be called on entries in reverse order. In that case row_ranges
    // must be already in reverse order.
    template<bool reversed, typename Func>
    void trim_rows(const schema& s,
        const std::vector<query::clustering_range>& row_ranges,
        Func&& func);
public:
    // Performs the following:
    //   - throws out data which doesn't belong to row_ranges
    //   - expires cells and tombstones based on query_time
    //   - drops cells covered by higher-level tombstones (compaction)
    //   - leaves at most row_limit live rows
    //
    // Note: a partition with a static row which has any cell live but no
    // clustered rows still counts as one row, according to the CQL row
    // counting rules.
    //
    // Returns the count of CQL rows which remained. If the returned number is
    // smaller than the row_limit it means that there was no more data
    // satisfying the query left.
    //
    // The row_limit parameter must be > 0.
    //
    uint32_t compact_for_query(const schema& s, gc_clock::time_point query_time,
        const std::vector<query::clustering_range>& row_ranges, bool always_return_static_content,
        bool reversed, uint32_t row_limit);

    // Performs the following:
    //   - expires cells based on compaction_time
    //   - drops cells covered by higher-level tombstones
    //   - drops expired tombstones which timestamp is before max_purgeable
    void compact_for_compaction(const schema& s, can_gc_fn&,
        gc_clock::time_point compaction_time);

    // Returns the minimal mutation_partition that when applied to "other" will
    // create a mutation_partition equal to the sum of other and this one.
    // This and other must both be governed by the same schema s.
    mutation_partition difference(schema_ptr s, const mutation_partition& other) const;

    // Returns a subset of this mutation holding only information relevant for given clustering ranges.
    // Range tombstones will be trimmed to the boundaries of the clustering ranges.
    mutation_partition sliced(const schema& s, const query::clustering_row_ranges&) const;

    // Returns true if the mutation_partition represents no writes.
    bool empty() const;
public:
    deletable_row& clustered_row(const schema& s, const clustering_key& key);
    deletable_row& clustered_row(const schema& s, clustering_key&& key);
    deletable_row& clustered_row(const schema& s, clustering_key_view key);
    deletable_row& clustered_row(const schema& s, position_in_partition_view pos, is_dummy, is_continuous);
public:
    tombstone partition_tombstone() const;
    lazy_row& static_row();
    const lazy_row& static_row() const;
    // return a set of rows_entry where each entry represents a CQL row sharing the same clustering key.
    const range_tombstone_list& row_tombstones() const;
    range_tombstone_list& row_tombstones();
    const row* find_row(const schema& s, const clustering_key& key) const;
    tombstone range_tombstone_for_row(const schema& schema, const clustering_key& key) const;
    row_tombstone tombstone_for_row(const schema& schema, const clustering_key& key) const;
    // Can be called only for non-dummy entries
    row_tombstone tombstone_for_row(const schema& schema, const rows_entry& e) const;
    // Returns an iterator range of rows_entry, with only non-dummy entries.
    // Writes this partition using supplied query result writer.
    // The partition should be first compacted with compact_for_query(), otherwise
    // results may include data which is deleted/expired.
    // At most row_limit CQL rows will be written and digested.
    void query_compacted(query::result::partition_writer& pw, const schema& s, uint32_t row_limit) const;
    void accept(const schema&, mutation_partition_visitor&) const;

    // Returns the number of live CQL rows in this partition.
    //
    // Note: If no regular rows are live, but there's something live in the
    // static row, the static row counts as one row. If there is at least one
    // regular row live, static row doesn't count.
    //
    size_t live_row_count(const schema&,
        gc_clock::time_point query_time = gc_clock::time_point::min()) const;

    bool is_static_row_live(const schema&,
        gc_clock::time_point query_time = gc_clock::time_point::min()) const;

    size_t row_count() const;

    size_t external_memory_usage(const schema&) const;
private:
    template<typename Func>
    void for_each_row(const schema& schema, const query::clustering_range& row_range, bool reversed, Func&& func) const;
    friend class counter_write_query_result_builder;

    void check_schema(const schema& s) const;
};

