
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

#include <optional>

#include "keys.hh"
#include "dht/i_partitioner.hh"
#include "enum_set.hh"
#include "range.hh"
#include "tracing/tracing.hh"

class position_in_partition_view;

namespace query {

using column_id_vector = utils::small_vector<column_id, 8>;

template <typename T>
using range = wrapping_range<T>;

using ring_position = dht::ring_position;
using clustering_range = nonwrapping_range<clustering_key_prefix>;

extern const dht::partition_range full_partition_range;
extern const clustering_range full_clustering_range;


typedef std::vector<clustering_range> clustering_row_ranges;

/// Trim the clustering ranges.
///
/// Equivalent of intersecting each clustering range with [pos, +inf) position
/// in partition range, or (-inf, pos] position in partition range if
/// reversed == true. Ranges that do not intersect are dropped. Ranges that
/// partially overlap are trimmed.
/// Result: each range will overlap fully with [pos, +inf), or (-int, pos] if
/// reversed is true.
void trim_clustering_row_ranges_to(const schema& s, clustering_row_ranges& ranges, position_in_partition_view pos, bool reversed = false);

/// Trim the clustering ranges.
///
/// Equivalent of intersecting each clustering range with (key, +inf) clustering
/// range, or (-inf, key) clustering range if reversed == true. Ranges that do
/// not intersect are dropped. Ranges that partially overlap are trimmed.
/// Result: each range will overlap fully with (key, +inf), or (-int, key) if
/// reversed is true.
void trim_clustering_row_ranges_to(const schema& s, clustering_row_ranges& ranges, const clustering_key& key, bool reversed = false);

class specific_ranges {
};

constexpr auto max_rows = std::numeric_limits<uint32_t>::max();

// Specifies subset of rows, columns and cell attributes to be returned in a query.
// Can be accessed across cores.
// Schema-dependent.
class partition_slice {
public:
    enum class option {
        send_clustering_key,
        send_partition_key,
        send_timestamp,
        send_expiry,
        reversed,
        distinct,
        collections_as_maps,
        send_ttl,
        allow_short_read,
        with_digest,
        bypass_cache,
        // Normally, we don't return static row if the request has clustering
        // key restrictions and the partition doesn't have any rows matching
        // the restrictions, see #589. This flag overrides this behavior.
        always_return_static_content,
    };
    using option_set = enum_set<super_enum<option,
        option::send_clustering_key,
        option::send_partition_key,
        option::send_timestamp,
        option::send_expiry,
        option::reversed,
        option::distinct,
        option::collections_as_maps,
        option::send_ttl,
        option::allow_short_read,
        option::with_digest,
        option::bypass_cache,
        option::always_return_static_content>>;
public:
    partition_slice(clustering_row_ranges row_ranges, column_id_vector static_columns,
        column_id_vector regular_columns, option_set options,
        std::unique_ptr<specific_ranges> specific_ranges = nullptr,
        cql_serialization_format = cql_serialization_format::internal(),
        uint32_t partition_row_limit = max_rows);
    partition_slice(clustering_row_ranges ranges, const schema& schema, const column_set& mask, option_set options);
    partition_slice(const partition_slice&);
    partition_slice(partition_slice&&);

    partition_slice& operator=(partition_slice&& other) noexcept;

    const clustering_row_ranges& row_ranges(const schema&, const partition_key&) const;
    void set_range(const schema&, const partition_key&, clustering_row_ranges);
    void clear_range(const schema&, const partition_key&);
    void clear_ranges();    // FIXME: possibly make this function return a const ref instead.
    clustering_row_ranges get_all_ranges() const;

    const clustering_row_ranges& default_row_ranges() const;
    const std::unique_ptr<specific_ranges>& get_specific_ranges() const;
    const cql_serialization_format& cql_format() const;
    const uint32_t partition_row_limit() const;
    void set_partition_row_limit(uint32_t limit);

    friend std::ostream& operator<<(std::ostream& out, const partition_slice& ps);
    friend std::ostream& operator<<(std::ostream& out, const specific_ranges& ps);
};

constexpr auto max_partitions = std::numeric_limits<uint32_t>::max();


}
