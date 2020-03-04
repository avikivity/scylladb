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


#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>

extern std::atomic<int64_t> clocks_offset;


#include <chrono>
#include <optional>
#include <seastar/core/byteorder.hh>
#include <seastar/core/sstring.hh>

namespace seastar {

template <typename T>
class shared_ptr;

template <typename T>
shared_ptr<T> make_shared(T&&);

template <typename T, typename... A>
shared_ptr<T> make_shared(A&&... a);

}


using namespace seastar;
using seastar::shared_ptr;
using seastar::make_shared;
#include <seastar/util/gcc6-concepts.hh>


//
// This hashing differs from std::hash<> in that it decouples knowledge about
// type structure from the way the hash value is calculated:
//  * appending_hash<T> instantiation knows about what data should be included in the hash for type T.
//  * Hasher object knows how to combine the data into the final hash.
//
// The appending_hash<T> should always feed some data into the hasher, regardless of the state the object is in,
// in order for the hash to be highly sensitive for value changes. For example, vector<optional<T>> should
// ideally feed different values for empty vector and a vector with a single empty optional.
//
// appending_hash<T> is machine-independent.
//

GCC6_CONCEPT(
    template<typename H>
    concept bool Hasher() {
        return requires(H& h, const char* ptr, size_t size) {
            { h.update(ptr, size) } -> void;
        };
    }
)

class hasher {
public:
    virtual ~hasher() = default;
    virtual void update(const char* ptr, size_t size) = 0;
};

GCC6_CONCEPT(static_assert(Hasher<hasher>());)

template<typename T, typename Enable = void>
struct appending_hash;

template<typename H, typename T, typename... Args>
GCC6_CONCEPT(requires Hasher<H>())
void feed_hash(H& h, const T& value, Args&&... args);

#include <seastar/core/lowres_clock.hh>

#include <chrono>
#include <optional>

class gc_clock final {
public:
    using base = seastar::lowres_system_clock;
    using rep = int64_t;
    using period = std::ratio<1, 1>; // seconds
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<gc_clock, duration>;

    static constexpr auto is_steady = base::is_steady;

    static time_point now();

    static int32_t as_int32(duration d);
    static int32_t as_int32(time_point tp);
};

using expiry_opt = std::optional<gc_clock::time_point>;
using ttl_opt = std::optional<gc_clock::duration>;

// 20 years in seconds
static constexpr gc_clock::duration max_ttl = gc_clock::duration{20 * 365 * 24 * 60 * 60};

std::ostream& operator<<(std::ostream& os, gc_clock::time_point tp);




#include <chrono>
#include <cstdint>
#include <ratio>

// the database clock follows Java - 1ms granularity, 64-bit counter, 1970 epoch

class db_clock final {
public:
    using base = std::chrono::system_clock;
    using rep = int64_t;
    using period = std::ratio<1, 1000>; // milliseconds
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<db_clock, duration>;

    static constexpr bool is_steady = base::is_steady;
    static time_point now() {
        return time_point();
    }
};

gc_clock::time_point to_gc_clock(db_clock::time_point tp);
/* For debugging and log messages. */
std::ostream& operator<<(std::ostream&, db_clock::time_point);


#include <chrono>
#include <functional>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string.h>
#include <seastar/core/future.hh>
#include <limits>
#include <cstddef>

#include <seastar/core/shared_ptr.hh>

using column_count_type = uint32_t;

// Column ID, unique within column_kind
using column_id = column_count_type;

class schema;
class schema_extension;

using schema_ptr = seastar::lw_shared_ptr<const schema>;


#include <cstdint>
#include <limits>
#include <chrono>
#include <string>

namespace api {

using timestamp_type = int64_t;
timestamp_type constexpr missing_timestamp = std::numeric_limits<timestamp_type>::min();
timestamp_type constexpr min_timestamp = std::numeric_limits<timestamp_type>::min() + 1;
timestamp_type constexpr max_timestamp = std::numeric_limits<timestamp_type>::max();

// Used for generating server-side mutation timestamps.
// Same epoch as Java's System.currentTimeMillis() for compatibility.
// Satisfies requirements of Clock.
class timestamp_clock final {
    using base = std::chrono::system_clock;
public:
    using rep = timestamp_type;
    using duration = std::chrono::microseconds;
    using period = typename duration::period;
    using time_point = std::chrono::time_point<timestamp_clock, duration>;

    static constexpr bool is_steady = base::is_steady;

    static time_point now();
};

timestamp_type new_timestamp();

}

/* For debugging and log messages. */
std::string format_timestamp(api::timestamp_type);


#include <functional>

#include <seastar/util/gcc6-concepts.hh>
#include <type_traits>

GCC6_CONCEPT(
template<typename T>
concept bool HasTriCompare =
    requires(const T& t) {
        { t.compare(t) } -> int;
    } && std::is_same<std::result_of_t<decltype(&T::compare)(T, T)>, int>::value; //FIXME: #1449
)

template<typename T>
class with_relational_operators {
private:
    template<typename U>
    GCC6_CONCEPT( requires HasTriCompare<U> )
    int do_compare(const U& t) const;
public:
    bool operator<(const T& t) const ;

    bool operator<=(const T& t) const;

    bool operator>(const T& t) const;

    bool operator>=(const T& t) const;

    bool operator==(const T& t) const;

    bool operator!=(const T& t) const;
};

/**
 * Represents deletion operation. Can be commuted with other tombstones via apply() method.
 * Can be empty.
 */
struct tombstone final : public with_relational_operators<tombstone> {
    api::timestamp_type timestamp;
    gc_clock::time_point deletion_time;

    tombstone(api::timestamp_type timestamp, gc_clock::time_point deletion_time);

    tombstone();
    int compare(const tombstone& t) const;
    explicit operator bool() const;
    void apply(const tombstone& t) noexcept;

    // See reversibly_mergeable.hh
    void apply_reversibly(tombstone& t) noexcept;

    // See reversibly_mergeable.hh
    void revert(tombstone& t) noexcept;
    tombstone operator+(const tombstone& t);

    friend std::ostream& operator<<(std::ostream& out, const tombstone& t);
};

template<>
struct appending_hash<tombstone> {
    template<typename Hasher>
    void operator()(Hasher& h, const tombstone& t) const;
};

// Determines whether tombstone may be GC-ed.
using can_gc_fn = std::function<bool(tombstone)>;

static can_gc_fn always_gc = [] (tombstone) { return true; };

#include <iosfwd>

#include "mutation_partition.hh"
#include "keys.hh"
#include "schema_fwd.hh"
#include "dht/i_partitioner.hh"
#include "hashing.hh"
#include "mutation_fragment.hh"

#include <seastar/util/optimized_optional.hh>

class mutation final {
    mutation() = default;
    explicit operator bool();
    friend class optimized_optional<mutation>;
public:
    mutation(schema_ptr schema, dht::decorated_key key);
    mutation(schema_ptr schema, partition_key key_);
    mutation(schema_ptr schema, dht::decorated_key key, const mutation_partition& mp);
    mutation(schema_ptr schema, dht::decorated_key key, mutation_partition&& mp);
    mutation(const mutation& m);
    mutation(mutation&&) = default;
    mutation& operator=(mutation&& x) = default;
    mutation& operator=(const mutation& m);

    void set_static_cell(const column_definition& def, atomic_cell_or_collection&& value);
    void set_static_cell(const bytes& name, const data_value& value, api::timestamp_type timestamp, ttl_opt ttl = {});
    void set_clustered_cell(const clustering_key& key, const bytes& name, const data_value& value, api::timestamp_type timestamp, ttl_opt ttl = {});
    void set_clustered_cell(const clustering_key& key, const column_definition& def, atomic_cell_or_collection&& value);
    void set_cell(const clustering_key_prefix& prefix, const bytes& name, const data_value& value, api::timestamp_type timestamp, ttl_opt ttl = {});
    void set_cell(const clustering_key_prefix& prefix, const column_definition& def, atomic_cell_or_collection&& value);

    // Upgrades this mutation to a newer schema. The new schema must
    // be obtained using only valid schema transformation:
    //  * primary key column count must not change
    //  * column types may only change to those with compatible representations
    //
    // After upgrade, mutation's partition should only be accessed using the new schema. User must
    // ensure proper isolation of accesses.
    //
    // Strong exception guarantees.
    //
    // Note that the conversion may lose information, it's possible that m1 != m2 after:
    //
    //   auto m2 = m1;
    //   m2.upgrade(s2);
    //   m2.upgrade(m1.schema());
    //
    void upgrade(const schema_ptr&);

    const partition_key& key() const;
    const dht::decorated_key& decorated_key() const;
    dht::ring_position ring_position() const;
    const dht::token& token() const;
    const schema_ptr& schema() const;
    const mutation_partition& partition() const;
    mutation_partition& partition();
    const utils::UUID& column_family_id() const;
    // Consistent with hash<canonical_mutation>
    bool operator==(const mutation&) const;
    bool operator!=(const mutation&) const;
public:
    // The supplied partition_slice must be governed by this mutation's schema
    query::result query(const query::partition_slice&,
        query::result_options opts = query::result_options::only_result(),
        gc_clock::time_point now = gc_clock::now(),
        uint32_t row_limit = query::max_rows) &&;

    // The supplied partition_slice must be governed by this mutation's schema
    // FIXME: Slower than the r-value version
    query::result query(const query::partition_slice&,
        query::result_options opts = query::result_options::only_result(),
        gc_clock::time_point now = gc_clock::now(),
        uint32_t row_limit = query::max_rows) const&;

    // The supplied partition_slice must be governed by this mutation's schema
    void query(query::result::builder& builder,
        const query::partition_slice& slice,
        gc_clock::time_point now = gc_clock::now(),
        uint32_t row_limit = query::max_rows) &&;

    // See mutation_partition::live_row_count()
    size_t live_row_count(gc_clock::time_point query_time = gc_clock::time_point::min()) const;

    void apply(mutation&&);
    void apply(const mutation&);
    void apply(const mutation_fragment&);

    mutation operator+(const mutation& other) const;
    mutation& operator+=(const mutation& other);
    mutation& operator+=(mutation&& other);

    // Returns a subset of this mutation holding only information relevant for given clustering ranges.
    // Range tombstones will be trimmed to the boundaries of the clustering ranges.
    mutation sliced(const query::clustering_row_ranges&) const;
private:
    friend std::ostream& operator<<(std::ostream& os, const mutation& m);
};

struct mutation_equals_by_key {
    bool operator()(const mutation& m1, const mutation& m2) const;
};

struct mutation_hash_by_key {
    size_t operator()(const mutation& m) const;
};

struct mutation_decorated_key_less_comparator {
    bool operator()(const mutation& m1, const mutation& m2) const;
};

using mutation_opt = optimized_optional<mutation>;

// Consistent with operator==()
// Consistent across the cluster, so should not rely on particular
// serialization format, only on actual data stored.
template<>
struct appending_hash<mutation> {
    template<typename Hasher>
    void operator()(Hasher& h, const mutation& m) const;
};

void apply(mutation_opt& dst, mutation&& src);

void apply(mutation_opt& dst, mutation_opt&& src);

// Returns a range into partitions containing mutations covered by the range.
// partitions must be sorted according to decorated key.
// range must not wrap around.
boost::iterator_range<std::vector<mutation>::const_iterator> slice(
    const std::vector<mutation>& partitions,
    const dht::partition_range&);

class flat_mutation_reader;

// Reads a single partition from a reader. Returns empty optional if there are no more partitions to be read.
future<mutation_opt> read_mutation_from_flat_mutation_reader(flat_mutation_reader& reader, db::timeout_clock::time_point timeout);

class cell_locker;
class cell_locker_stats;
class locked_cell;

class frozen_mutation;
class reconcilable_result;

namespace service {
class storage_proxy;
class migration_notifier;
class migration_manager;
}

namespace netw {
class messaging_service;
}

namespace gms {
class feature_service;
}

namespace sstables {

class sstable;
class entry_descriptor;
class compaction_descriptor;
class compaction_completion_desc;
class foreign_sstable_open_info;
class sstables_manager;

}

class compaction_manager;

namespace ser {
template<typename T>
class serializer;
}

namespace db {
class commitlog;
class config;
class extensions;
class rp_handle;
class data_listeners;
class large_data_handler;
}

class mutation_reordered_with_truncate_exception : public std::exception {};

using shared_memtable = lw_shared_ptr<memtable>;
class memtable_list;

// We could just add all memtables, regardless of types, to a single list, and
// then filter them out when we read them. Here's why I have chosen not to do
// it:
//
// First, some of the methods in which a memtable is involved (like seal) are
// assume a commitlog, and go through great care of updating the replay
// position, flushing the log, etc.  We want to bypass those, and that has to
// be done either by sprikling the seal code with conditionals, or having a
// separate method for each seal.
//
// Also, if we ever want to put some of the memtables in as separate allocator
// region group to provide for extra QoS, having the classes properly wrapped
// will make that trivial: just pass a version of new_memtable() that puts it
// in a different region, while the list approach would require a lot of
// conditionals as well.
//
// If we are going to have different methods, better have different instances
// of a common class.
class memtable_list {
};

using sstable_list = sstables::sstable_list;


class table;
using column_family = table;
struct table_stats;
using column_family_stats = table_stats;

class database_sstable_write_monitor;


class table {
public:
    future<std::vector<locked_cell>> lock_counter_cells(const mutation& m, db::timeout_clock::time_point timeout);

};

class user_types_metadata;

class keyspace_metadata final {
};

class keyspace {
public:
};


// Policy for distributed<database>:
//   broadcast metadata writes
//   local metadata reads
//   use shard_of() for data

class database {
private:
    future<mutation> do_apply_counter_update(column_family& cf, const frozen_mutation& fm, schema_ptr m_schema, db::timeout_clock::time_point timeout,
                                             tracing::trace_state_ptr trace_state);
public:

};



#include <seastar/core/future-util.hh>
#include "frozen_mutation.hh"
#include <seastar/core/do_with.hh>
#include "mutation_query.hh"

using namespace std::chrono_literals;
using namespace db;


class locked_cell {
};


future<mutation> database::do_apply_counter_update(column_family& cf, const frozen_mutation& fm, schema_ptr m_schema,
                                                   db::timeout_clock::time_point timeout,tracing::trace_state_ptr trace_state) {
    auto m = fm.unfreeze(m_schema);

    query::column_id_vector static_columns;
    query::clustering_row_ranges cr_ranges;
    query::column_id_vector regular_columns;


    auto slice = query::partition_slice(std::move(cr_ranges), std::move(static_columns),
        std::move(regular_columns), { }, { }, cql_serialization_format::internal(), query::max_rows);

    return do_with(std::move(slice), std::move(m), std::vector<locked_cell>(),
                   [this, &cf, timeout] (const query::partition_slice& slice, mutation& m, std::vector<locked_cell>& locks) mutable {
        return cf.lock_counter_cells(m, timeout).then([&, timeout, this] (std::vector<locked_cell> lcs) mutable {
            locks = std::move(lcs);

            // Before counter update is applied it needs to be transformed from
            // deltas to counter shards. To do that, we need to read the current
            // counter state for each modified cell...

            return counter_write_query(schema_ptr(), mutation_source(), m.decorated_key(), slice, nullptr)
                    .then([this, &cf, &m, timeout] (auto mopt) {
                // ...now, that we got existing state of all affected counter
                // cells we can look for our shard in each of them, increment
                // its clock and apply the delta.
                return std::move(m);
            });
        });
    });
}

