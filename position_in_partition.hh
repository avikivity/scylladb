/*
 * Copyright (C) 2017 ScyllaDB
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

#include "types.hh"
#include "keys.hh"
#include "clustering_bounds_comparator.hh"
#include "query-request.hh"

#include <optional>

lexicographical_relation relation_for_lower_bound(composite_view v);
lexicographical_relation relation_for_upper_bound(composite_view v);

enum class bound_weight : int8_t {
    before_all_prefixed = -1,
    equal = 0,
    after_all_prefixed = 1,
};

bound_weight position_weight(bound_kind k);

enum class partition_region : uint8_t {
    partition_start,
    static_row,
    clustered,
    partition_end,
};

class position_in_partition_view {
    friend class position_in_partition;
public:
    position_in_partition_view(partition_region type, bound_weight weight, const clustering_key_prefix* ck);
    bool is_before_key() const;
    bool is_after_key() const;
private:
    // Returns placement of this position_in_partition relative to *_ck,
    // or lexicographical_relation::at_prefix if !_ck.
    lexicographical_relation relation() const;
public:
    struct partition_start_tag_t { };
    struct end_of_partition_tag_t { };
    struct static_row_tag_t { };
    struct clustering_row_tag_t { };
    struct range_tag_t { };
    using range_tombstone_tag_t = range_tag_t;

    explicit position_in_partition_view(partition_start_tag_t);
    explicit position_in_partition_view(end_of_partition_tag_t);
    explicit position_in_partition_view(static_row_tag_t);
    position_in_partition_view(clustering_row_tag_t, const clustering_key_prefix& ck);
    position_in_partition_view(const clustering_key_prefix& ck);
    position_in_partition_view(range_tag_t, bound_view bv);
    position_in_partition_view(const clustering_key_prefix& ck, bound_weight w);

    static position_in_partition_view for_range_start(const query::clustering_range& r);

    static position_in_partition_view for_range_end(const query::clustering_range& r);

    static position_in_partition_view before_all_clustered_rows();

    static position_in_partition_view after_all_clustered_rows();

    static position_in_partition_view for_static_row();

    static position_in_partition_view for_key(const clustering_key& ck);

    static position_in_partition_view after_key(const clustering_key& ck);

    static position_in_partition_view before_key(const clustering_key& ck);

    partition_region region() const;
    bound_weight get_bound_weight() const;
    bool is_partition_start() const;
    bool is_partition_end() const;
    bool is_static_row() const;
    bool is_clustering_row() const;
    bool has_clustering_key() const;

    // Returns true if all fragments that can be seen for given schema have
    // positions >= than this. partition_start is ignored.
    bool is_before_all_fragments(const schema& s) const;

    bool is_after_all_clustered_rows(const schema& s) const;

    // Valid when >= before_all_clustered_rows()
    const clustering_key_prefix& key() const;

    // Can be called only when !is_static_row && !is_clustering_row().
    bound_view as_start_bound_view() const;

    bound_view as_end_bound_view() const;

    class printer {
    public:
        printer(const schema& schema, const position_in_partition_view& pipv);
        friend std::ostream& operator<<(std::ostream& os, printer p);
    };

    friend std::ostream& operator<<(std::ostream& os, printer p);
    friend std::ostream& operator<<(std::ostream&, position_in_partition_view);
    friend bool no_clustering_row_between(const schema&, position_in_partition_view, position_in_partition_view);
};

class position_in_partition {
public:
    friend class clustering_interval_set;
    struct partition_start_tag_t { };
    struct end_of_partition_tag_t { };
    struct static_row_tag_t { };
    struct after_static_row_tag_t { };
    struct clustering_row_tag_t { };
    struct after_clustering_row_tag_t { };
    struct before_clustering_row_tag_t { };
    struct range_tag_t { };
    using range_tombstone_tag_t = range_tag_t;
    partition_region get_type() const;
    bound_weight get_bound_weight() const;
    const std::optional<clustering_key_prefix>& get_clustering_key_prefix() const;
    position_in_partition(partition_region type, bound_weight weight, std::optional<clustering_key_prefix> ck);
    explicit position_in_partition(partition_start_tag_t);
    explicit position_in_partition(end_of_partition_tag_t);
    explicit position_in_partition(static_row_tag_t);
    position_in_partition(clustering_row_tag_t, clustering_key_prefix ck);
    position_in_partition(after_clustering_row_tag_t, clustering_key_prefix ck);
    position_in_partition(after_clustering_row_tag_t, position_in_partition_view pos);
    position_in_partition(before_clustering_row_tag_t, clustering_key_prefix ck);
    position_in_partition(range_tag_t, bound_view bv);
    position_in_partition(range_tag_t, bound_kind kind, clustering_key_prefix&& prefix);
    position_in_partition(after_static_row_tag_t);
    explicit position_in_partition(position_in_partition_view view);
    position_in_partition& operator=(position_in_partition_view view);

    static position_in_partition before_all_clustered_rows();

    static position_in_partition after_all_clustered_rows();

    static position_in_partition before_key(clustering_key ck);

    static position_in_partition after_key(clustering_key ck);

    // If given position is a clustering row position, returns a position
    // right after it. Otherwise returns it unchanged.
    // The position "pos" must be a clustering position.
    static position_in_partition after_key(position_in_partition_view pos);

    static position_in_partition for_key(clustering_key ck);

    static position_in_partition for_partition_start();

    static position_in_partition for_static_row();

    static position_in_partition min();

    static position_in_partition for_range_start(const query::clustering_range&);
    static position_in_partition for_range_end(const query::clustering_range&);

    partition_region region() const;
    bool is_partition_start() const;
    bool is_partition_end() const;
    bool is_static_row() const;
    bool is_clustering_row() const;
    bool has_clustering_key() const;

    bool is_after_all_clustered_rows(const schema& s) const;
    bool is_before_all_clustered_rows(const schema& s) const;

    template<typename Hasher>
    void feed_hash(Hasher& hasher, const schema& s) const;
    const clustering_key_prefix& key() const;
    operator position_in_partition_view() const;

    // Defines total order on the union of position_and_partition and composite objects.
    //
    // The ordering is compatible with position_range (r). The following is satisfied for
    // all cells with name c included by the range:
    //
    //   r.start() <= c < r.end()
    //
    // The ordering on composites given by this is compatible with but weaker than the cell name order.
    //
    // The ordering on position_in_partition given by this is compatible but weaker than the ordering
    // given by position_in_partition::tri_compare.
    //
    class composite_tri_compare {
    public:
        static int rank(partition_region t);

        composite_tri_compare(const schema& s);

        int operator()(position_in_partition_view a, position_in_partition_view b) const;

        int operator()(position_in_partition_view a, composite_view b) const;
        int operator()(composite_view a, position_in_partition_view b) const;
    };

    // Less comparator giving the same order as composite_tri_compare.
    class composite_less_compare {
    public:
        composite_less_compare(const schema& s);

        template<typename T, typename U>
        bool operator()(const T& a, const U& b) const;
    };

    class tri_compare {
    public:
        tri_compare(const schema& s);
        int operator()(const position_in_partition& a, const position_in_partition& b) const;
        int operator()(const position_in_partition_view& a, const position_in_partition_view& b) const;
        int operator()(const position_in_partition& a, const position_in_partition_view& b) const;
        int operator()(const position_in_partition_view& a, const position_in_partition& b) const;
    };
    class less_compare {
    public:
        less_compare(const schema& s);
        bool operator()(const position_in_partition& a, const position_in_partition& b) const;
        bool operator()(const position_in_partition_view& a, const position_in_partition_view& b) const;
        bool operator()(const position_in_partition& a, const position_in_partition_view& b) const;
        bool operator()(const position_in_partition_view& a, const position_in_partition& b) const;
    };
    class equal_compare {
        template<typename T, typename U>
        bool compare(const T& a, const U& b) const;
    public:
        equal_compare(const schema& s);
        bool operator()(const position_in_partition& a, const position_in_partition& b) const;
        bool operator()(const position_in_partition_view& a, const position_in_partition_view& b) const;
        bool operator()(const position_in_partition_view& a, const position_in_partition& b) const;
        bool operator()(const position_in_partition& a, const position_in_partition_view& b) const;
    };
    friend std::ostream& operator<<(std::ostream&, const position_in_partition&);
};

// Returns true if and only if there can't be any clustering_row with position > a and < b.
// It is assumed that a <= b.
bool no_clustering_row_between(const schema& s, position_in_partition_view a, position_in_partition_view b);

// Includes all position_in_partition objects "p" for which: start <= p < end
// And only those.
class position_range {
public:
    static position_range from_range(const query::clustering_range&);

    static position_range for_static_row();

    static position_range full();

    static position_range all_clustered_rows();

    position_range(position_range&&) = default;
    position_range& operator=(position_range&&) = default;
    position_range(const position_range&) = default;
    position_range& operator=(const position_range&) = default;

    // Constructs position_range which covers the same rows as given clustering_range.
    // position_range includes a fragment if it includes position of that fragment.
    position_range(const query::clustering_range&);
    position_range(query::clustering_range&&);

    position_range(position_in_partition start, position_in_partition end);

    const position_in_partition& start() const&;
    position_in_partition&& start() &&;
    const position_in_partition& end() const&;
    position_in_partition&& end() &&;
    bool contains(const schema& s, position_in_partition_view pos) const;
    bool overlaps(const schema& s, position_in_partition_view start, position_in_partition_view end) const;

    friend std::ostream& operator<<(std::ostream&, const position_range&);
};

class clustering_interval_set;
