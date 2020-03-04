/*
 * Copyright (C) 2015 ScyllaDB
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

#include <vector>

#include "mutation.hh"
#include <seastar/core/future.hh>
#include <seastar/core/future-util.hh>
#include <seastar/core/do_with.hh>
#include "tracing/trace_state.hh"
#include "flat_mutation_reader.hh"
#include "reader_concurrency_semaphore.hh"

namespace mutation_reader {
    // mutation_reader::forwarding determines whether fast_forward_to() may
    // be used on the mutation reader to change the partition range being
    // read. Enabling forwarding also changes read policy: forwarding::no
    // means we will stop reading from disk at the end of the given range,
    // but with forwarding::yes we may read ahead, anticipating the user to
    // make a small skip with fast_forward_to() and continuing to read.
    //
    // Note that mutation_reader::forwarding is similarly name but different
    // from streamed_mutation::forwarding - the former is about skipping to
    // a different partition range, while the latter is about skipping
    // inside a large partition.
    using forwarding = flat_mutation_reader::partition_range_forwarding;
}

/// A partition_presence_checker quickly returns whether a key is known not to exist
/// in a data source (it may return false positives, but not false negatives).
enum class partition_presence_checker_result {
    definitely_doesnt_exist,
    maybe_exists
};
using partition_presence_checker = std::function<partition_presence_checker_result (const dht::decorated_key& key)>;

partition_presence_checker make_default_partition_presence_checker();

// mutation_source represents source of data in mutation form. The data source
// can be queried multiple times and in parallel. For each query it returns
// independent mutation_reader.
// The reader returns mutations having all the same schema, the one passed
// when invoking the source.
class mutation_source {
    using partition_range = const dht::partition_range&;
    using io_priority = const io_priority_class&;
    using flat_reader_factory_type = std::function<flat_mutation_reader(schema_ptr,
                                                                        reader_permit,
                                                                        partition_range,
                                                                        const query::partition_slice&,
                                                                        io_priority,
                                                                        tracing::trace_state_ptr,
                                                                        streamed_mutation::forwarding,
                                                                        mutation_reader::forwarding)>;
public:
    mutation_source() = default;
    explicit operator bool() const;
    friend class optimized_optional<mutation_source>;
public:
    mutation_source(flat_reader_factory_type fn, std::function<partition_presence_checker()> pcf = [] { return make_default_partition_presence_checker(); });

    // For sources which don't care about the mutation_reader::forwarding flag (always fast forwardable)
    mutation_source(std::function<flat_mutation_reader(schema_ptr, reader_permit, partition_range, const query::partition_slice&, io_priority,
                tracing::trace_state_ptr, streamed_mutation::forwarding)> fn);
    mutation_source(std::function<flat_mutation_reader(schema_ptr, reader_permit, partition_range, const query::partition_slice&, io_priority)> fn);
    mutation_source(std::function<flat_mutation_reader(schema_ptr, reader_permit, partition_range, const query::partition_slice&)> fn);
    mutation_source(std::function<flat_mutation_reader(schema_ptr, reader_permit, partition_range range)> fn);
    mutation_source(const mutation_source& other) = default;
    mutation_source& operator=(const mutation_source& other) = default;
    mutation_source(mutation_source&&) = default;
    mutation_source& operator=(mutation_source&&) = default;

    // Creates a new reader.
    //
    // All parameters captured by reference must remain live as long as returned
    // mutation_reader or streamed_mutation obtained through it are alive.
    flat_mutation_reader
    make_reader(
        schema_ptr s,
        reader_permit permit,
        partition_range range,
        const query::partition_slice& slice,
        io_priority pc = default_priority_class(),
        tracing::trace_state_ptr trace_state = nullptr,
        streamed_mutation::forwarding fwd = streamed_mutation::forwarding::no,
        mutation_reader::forwarding fwd_mr = mutation_reader::forwarding::yes) const;

    flat_mutation_reader
    make_reader(
        schema_ptr s,
        reader_permit permit = no_reader_permit(),
        partition_range range = query::full_partition_range) const;
    partition_presence_checker make_partition_presence_checker();
};


using mutation_source_opt = optimized_optional<mutation_source>;

