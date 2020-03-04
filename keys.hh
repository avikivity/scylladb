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

#include "bytes.hh"
#include "compound_compat.hh"
#include "utils/managed_bytes.hh"
#include "hashing.hh"
#include "database_fwd.hh"
#include "schema_fwd.hh"

//
// This header defines type system for primary key holders.
//
// We distinguish partition keys and clustering keys. API-wise they are almost
// the same, but they're separate type hierarchies.
//
// Clustering keys are further divided into prefixed and non-prefixed (full).
// Non-prefixed keys always have full component set, as defined by schema.
// Prefixed ones can have any number of trailing components missing. They may
// differ in underlying representation.
//
// The main classes are:
//
//   partition_key           - full partition key
//   clustering_key          - full clustering key
//   clustering_key_prefix   - clustering key prefix
//
// These classes wrap only the minimum information required to store the key
// (the key value itself). Any information which can be inferred from schema
// is not stored. Therefore accessors need to be provided with a pointer to
// schema, from which information about structure is extracted.

// Abstracts a view to serialized compound.
template <typename TopLevelView>
class compound_view_wrapper {
protected:
    bytes_view _bytes;
protected:
    compound_view_wrapper(bytes_view v)
        : _bytes(v)
    { }

    static inline const auto& get_compound_type(const schema& s) {
        return TopLevelView::get_compound_type(s);
    }
public:
    std::vector<bytes> explode(const schema& s) const {
        return get_compound_type(s)->deserialize_value(_bytes);
    }

    bytes_view representation() const {
        return _bytes;
    }

    struct less_compare {
        typename TopLevelView::compound _t;
        less_compare(const schema& s) : _t(get_compound_type(s)) {}
        bool operator()(const TopLevelView& k1, const TopLevelView& k2) const {
            return _t->less(k1.representation(), k2.representation());
        }
    };

    struct tri_compare {
        typename TopLevelView::compound _t;
        tri_compare(const schema &s) : _t(get_compound_type(s)) {}
        int operator()(const TopLevelView& k1, const TopLevelView& k2) const {
            return _t->compare(k1.representation(), k2.representation());
        }
    };

    struct hashing {
        typename TopLevelView::compound _t;
        hashing(const schema& s) : _t(get_compound_type(s)) {}
        size_t operator()(const TopLevelView& o) const {
            return _t->hash(o.representation());
        }
    };

    struct equality {
        typename TopLevelView::compound _t;
        equality(const schema& s) : _t(get_compound_type(s)) {}
        bool operator()(const TopLevelView& o1, const TopLevelView& o2) const {
            return _t->equal(o1.representation(), o2.representation());
        }
    };

    bool equal(const schema& s, const TopLevelView& other) const {
        return get_compound_type(s)->equal(representation(), other.representation());
    }

    // begin() and end() return iterators over components of this compound. The iterator yields a bytes_view to the component.
    // The iterators satisfy InputIterator concept.
    auto begin() const {
        return TopLevelView::compound::element_type::begin(representation());
    }

    // See begin()
    auto end() const {
        return TopLevelView::compound::element_type::end(representation());
    }

    // begin() and end() return iterators over components of this compound. The iterator yields a bytes_view to the component.
    // The iterators satisfy InputIterator concept.
    auto begin(const schema& s) const {
        return begin();
    }

    // See begin()
    auto end(const schema& s) const {
        return end();
    }

    bytes_view get_component(const schema& s, size_t idx) const {
        auto it = begin(s);
        std::advance(it, idx);
        return *it;
    }

    // Returns a range of bytes_view
    auto components() const {
        return TopLevelView::compound::element_type::components(representation());
    }

    // Returns a range of bytes_view
    auto components(const schema& s) const {
        return components();
    }

    bool is_empty() const {
        return _bytes.empty();
    }

    explicit operator bool() const {
        return !is_empty();
    }

    // For backward compatibility with existing code.
    bool is_empty(const schema& s) const {
        return is_empty();
    }
};

template <typename TopLevel, typename TopLevelView>
class compound_wrapper {
protected:
    managed_bytes _bytes;
protected:
    compound_wrapper(managed_bytes&& b) : _bytes(std::move(b)) {}

    static inline const auto& get_compound_type(const schema& s) {
        return TopLevel::get_compound_type(s);
    }
public:
    struct with_schema_wrapper {
        with_schema_wrapper(const schema& s, const TopLevel& key) : s(s), key(key) {}
        const schema& s;
        const TopLevel& key;
    };

    with_schema_wrapper with_schema(const schema& s) const {
        return with_schema_wrapper(s, *static_cast<const TopLevel*>(this));
    }

    static TopLevel make_empty() {
        return from_exploded(std::vector<bytes>());
    }

    static TopLevel make_empty(const schema&) {
        return make_empty();
    }

    template<typename RangeOfSerializedComponents>
    static TopLevel from_exploded(RangeOfSerializedComponents&& v) {
        return TopLevel::from_range(std::forward<RangeOfSerializedComponents>(v));
    }

    static TopLevel from_exploded(const schema& s, const std::vector<bytes>& v) {
        return from_exploded(v);
    }
    static TopLevel from_exploded_view(const std::vector<bytes_view>& v) {
        return from_exploded(v);
    }

    // We don't allow optional values, but provide this method as an efficient adaptor
    static TopLevel from_optional_exploded(const schema& s, const std::vector<bytes_opt>& v) {
        return TopLevel::from_bytes(get_compound_type(s)->serialize_optionals(v));
    }

    static TopLevel from_deeply_exploded(const schema& s, const std::vector<data_value>& v) {
        return TopLevel::from_bytes(get_compound_type(s)->serialize_value_deep(v));
    }

    static TopLevel from_single_value(const schema& s, bytes v) {
        return TopLevel::from_bytes(get_compound_type(s)->serialize_single(std::move(v)));
    }

    template <typename T>
    static
    TopLevel from_singular(const schema& s, const T& v) {
        auto ct = get_compound_type(s);
        if (!ct->is_singular()) {
            throw std::invalid_argument("compound is not singular");
        }
        auto type = ct->types()[0];
        return from_single_value(s, type->decompose(v));
    }

    TopLevelView view() const {
        return TopLevelView::from_bytes(_bytes);
    }

    operator TopLevelView() const {
        return view();
    }

    // FIXME: return views
    std::vector<bytes> explode(const schema& s) const {
        return get_compound_type(s)->deserialize_value(_bytes);
    }

    std::vector<bytes> explode() const {
        std::vector<bytes> result;
        for (bytes_view c : components()) {
            result.emplace_back(to_bytes(c));
        }
        return result;
    }

    struct tri_compare {
        typename TopLevel::compound _t;
        tri_compare(const schema& s) : _t(get_compound_type(s)) {}
        int operator()(const TopLevel& k1, const TopLevel& k2) const {
            return _t->compare(k1.representation(), k2.representation());
        }
        int operator()(const TopLevelView& k1, const TopLevel& k2) const {
            return _t->compare(k1.representation(), k2.representation());
        }
        int operator()(const TopLevel& k1, const TopLevelView& k2) const {
            return _t->compare(k1.representation(), k2.representation());
        }
    };

    struct less_compare {
        typename TopLevel::compound _t;
        less_compare(const schema& s) : _t(get_compound_type(s)) {}
        bool operator()(const TopLevel& k1, const TopLevel& k2) const {
            return _t->less(k1.representation(), k2.representation());
        }
        bool operator()(const TopLevelView& k1, const TopLevel& k2) const {
            return _t->less(k1.representation(), k2.representation());
        }
        bool operator()(const TopLevel& k1, const TopLevelView& k2) const {
            return _t->less(k1.representation(), k2.representation());
        }
    };

    struct hashing {
        hashing(const schema& s);
        size_t operator()(const TopLevel& o) const;
        size_t operator()(const TopLevelView& o) const;
    };

    struct equality {
        equality(const schema& s);
        bool operator()(const TopLevel& o1, const TopLevel& o2) const;
        bool operator()(const TopLevelView& o1, const TopLevel& o2) const;
        bool operator()(const TopLevel& o1, const TopLevelView& o2) const;
    };

    bool equal(const schema& s, const TopLevel& other) const ;

    bool equal(const schema& s, const TopLevelView& other) const ;

    operator bytes_view() const;

    const managed_bytes& representation() const;

    // begin() and end() return iterators over components of this compound. The iterator yields a bytes_view to the component.
    // The iterators satisfy InputIterator concept.
    auto begin(const schema& s) const {
        return get_compound_type(s)->begin(_bytes);
    }

    // See begin()
    auto end(const schema& s) const {
        return get_compound_type(s)->end(_bytes);
    }

    bool is_empty() const;

    explicit operator bool() const;

    // For backward compatibility with existing code.
    bool is_empty(const schema& s) const;

    // Returns a range of bytes_view
    auto components() const {
        return TopLevelView::compound::element_type::components(representation());
    }

    // Returns a range of bytes_view
    auto components(const schema& s) const;

    bytes_view get_component(const schema& s, size_t idx) const;

    // Returns the number of components of this compound.
    size_t size(const schema& s) const;

    size_t external_memory_usage() const;

    size_t memory_usage() const;
};

template <typename TopLevel, typename PrefixTopLevel>
class prefix_view_on_full_compound {
public:
    using iterator = typename compound_type<allow_prefixes::no>::iterator;
    prefix_view_on_full_compound(const schema& s, bytes_view b, unsigned prefix_len);

    iterator begin() const;
    iterator end() const;

    struct less_compare_with_prefix {

        less_compare_with_prefix(const schema& s);

        bool operator()(const prefix_view_on_full_compound& k1, const PrefixTopLevel& k2) const;

        bool operator()(const PrefixTopLevel& k1, const prefix_view_on_full_compound& k2) const;
    };
};

template <typename TopLevel>
class prefix_view_on_prefix_compound {
public:
    using iterator = typename compound_type<allow_prefixes::yes>::iterator;
    prefix_view_on_prefix_compound(const schema& s, bytes_view b, unsigned prefix_len);

    iterator begin() const;
    iterator end() const;

    struct less_compare_with_prefix {
        less_compare_with_prefix(const schema& s);

        bool operator()(const prefix_view_on_prefix_compound& k1, const TopLevel& k2) const;

        bool operator()(const TopLevel& k1, const prefix_view_on_prefix_compound& k2) const;
    };
};

template <typename TopLevel, typename TopLevelView, typename PrefixTopLevel>
class prefixable_full_compound : public compound_wrapper<TopLevel, TopLevelView> {
    using base = compound_wrapper<TopLevel, TopLevelView>;
protected:
    prefixable_full_compound(bytes&& b) : base(std::move(b)) {}
public:
    using prefix_view_type = prefix_view_on_full_compound<TopLevel, PrefixTopLevel>;

    bool is_prefixed_by(const schema& s, const PrefixTopLevel& prefix) const;

    struct less_compare_with_prefix {

        less_compare_with_prefix(const schema& s);

        bool operator()(const TopLevel& k1, const PrefixTopLevel& k2) const;

        bool operator()(const PrefixTopLevel& k1, const TopLevel& k2) const;
    };

    // In prefix equality two sequences are equal if any of them is a prefix
    // of the other. Otherwise lexicographical ordering is applied.
    // Note: full compounds sorted according to lexicographical ordering are also
    // sorted according to prefix equality ordering.
    struct prefix_equality_less_compare {
        prefix_equality_less_compare(const schema& s);

        bool operator()(const TopLevel& k1, const PrefixTopLevel& k2) const;

        bool operator()(const PrefixTopLevel& k1, const TopLevel& k2) const;
    };

    prefix_view_type prefix_view(const schema& s, unsigned prefix_len) const;
};

template <typename TopLevel, typename FullTopLevel>
class prefix_compound_view_wrapper : public compound_view_wrapper<TopLevel> {
    using base = compound_view_wrapper<TopLevel>;
protected:
    prefix_compound_view_wrapper(bytes_view v);

public:
    bool is_full(const schema& s) const;
};

template <typename TopLevel, typename TopLevelView, typename FullTopLevel>
class prefix_compound_wrapper : public compound_wrapper<TopLevel, TopLevelView> {
    using base = compound_wrapper<TopLevel, TopLevelView>;
protected:
    prefix_compound_wrapper(managed_bytes&& b) : base(std::move(b)) {}
public:
    using prefix_view_type = prefix_view_on_prefix_compound<TopLevel>;

    prefix_view_type prefix_view(const schema& s, unsigned prefix_len) const;

    bool is_full(const schema& s) const;

    // Can be called only if is_full()
    FullTopLevel to_full(const schema& s) const;

    bool is_prefixed_by(const schema& s, const TopLevel& prefix) const;

    // In prefix equality two sequences are equal if any of them is a prefix
    // of the other. Otherwise lexicographical ordering is applied.
    // Note: full compounds sorted according to lexicographical ordering are also
    // sorted according to prefix equality ordering.
    struct prefix_equality_less_compare {
        prefix_equality_less_compare(const schema& s);

        bool operator()(const TopLevel& k1, const TopLevel& k2) const;
    };

    // See prefix_equality_less_compare.
    struct prefix_equal_tri_compare {
        prefix_equal_tri_compare(const schema& s);

        int operator()(const TopLevel& k1, const TopLevel& k2) const;
    };
};

class partition_key_view : public compound_view_wrapper<partition_key_view> {
public:
    using c_type = compound_type<allow_prefixes::no>;
private:
    partition_key_view(bytes_view v);
public:
    using compound = lw_shared_ptr<c_type>;

    static partition_key_view from_bytes(bytes_view v);
    static const compound& get_compound_type(const schema& s);
    // Returns key's representation which is compatible with Origin.
    // The result is valid as long as the schema is live.
    const legacy_compound_view<c_type> legacy_form(const schema& s) const;

    // A trichotomic comparator for ordering compatible with Origin.
    int legacy_tri_compare(const schema& s, partition_key_view o) const;

    // Checks if keys are equal in a way which is compatible with Origin.
    bool legacy_equal(const schema& s, partition_key_view o) const;
    // A trichotomic comparator which orders keys according to their ordering on the ring.
    int ring_order_tri_compare(const schema& s, partition_key_view o) const;

    friend std::ostream& operator<<(std::ostream& out, const partition_key_view& pk);
};

class partition_key : public compound_wrapper<partition_key, partition_key_view> {
public:
    using c_type = compound_type<allow_prefixes::no>;

    template<typename RangeOfSerializedComponents>
    static partition_key from_range(RangeOfSerializedComponents&& v);
    /*!
     * \brief create a partition_key from a nodetool style string
     * takes a nodetool style string representation of a partition key and returns a partition_key.
     * With composite keys, columns are concatenate using ':'.
     * For example if a composite key is has two columns (col1, col2) to get the partition key that
     * have col1=val1 and col2=val2 use the string 'val1:val2'
     */
    static partition_key from_nodetool_style_string(const schema_ptr s, const sstring& key);

    partition_key(std::vector<bytes> v);
    partition_key(partition_key&& v) = default;
    partition_key(const partition_key& v) = default;
    partition_key(partition_key& v) = default;
    partition_key& operator=(const partition_key&) = default;
    partition_key& operator=(partition_key&) = default;
    partition_key& operator=(partition_key&&) = default;

    partition_key(partition_key_view key);

    using compound = lw_shared_ptr<c_type>;

    static partition_key from_bytes(bytes_view b);
    static const compound& get_compound_type(const schema& s);
    // Returns key's representation which is compatible with Origin.
    // The result is valid as long as the schema is live.
    const legacy_compound_view<c_type> legacy_form(const schema& s) const;
    // A trichotomic comparator for ordering compatible with Origin.
    int legacy_tri_compare(const schema& s, const partition_key& o) const;
    // Checks if keys are equal in a way which is compatible with Origin.
    bool legacy_equal(const schema& s, const partition_key& o) const;
    void validate(const schema& s) const;
    friend std::ostream& operator<<(std::ostream& out, const partition_key& pk);
};

std::ostream& operator<<(std::ostream& out, const partition_key::with_schema_wrapper& pk);

class exploded_clustering_prefix {
public:
    exploded_clustering_prefix(std::vector<bytes>&& v);
    exploded_clustering_prefix();
    size_t size() const;
    auto const& components() const;
    explicit operator bool() const;
    bool is_full(const schema& s) const;
    friend std::ostream& operator<<(std::ostream& os, const exploded_clustering_prefix& ecp);
};

class clustering_key_prefix_view : public prefix_compound_view_wrapper<clustering_key_prefix_view, clustering_key> {
public:
    static clustering_key_prefix_view from_bytes(bytes_view v);
    using compound = lw_shared_ptr<compound_type<allow_prefixes::yes>>;

    static const compound& get_compound_type(const schema& s);
    static clustering_key_prefix_view make_empty();};

class clustering_key_prefix : public prefix_compound_wrapper<clustering_key_prefix, clustering_key_prefix_view, clustering_key> {
public:
    template<typename RangeOfSerializedComponents>
    static clustering_key_prefix from_range(RangeOfSerializedComponents&& v);

    clustering_key_prefix(std::vector<bytes> v);

    clustering_key_prefix(clustering_key_prefix&& v) = default;
    clustering_key_prefix(const clustering_key_prefix& v) = default;
    clustering_key_prefix(clustering_key_prefix& v) = default;
    clustering_key_prefix& operator=(const clustering_key_prefix&) = default;
    clustering_key_prefix& operator=(clustering_key_prefix&) = default;
    clustering_key_prefix& operator=(clustering_key_prefix&&) = default;

    clustering_key_prefix(clustering_key_prefix_view v);
    using compound = lw_shared_ptr<compound_type<allow_prefixes::yes>>;

    static clustering_key_prefix from_bytes(bytes_view b);
    static const compound& get_compound_type(const schema& s);
    static clustering_key_prefix from_clustering_prefix(const schema& s, const exploded_clustering_prefix& prefix);
    /* This function makes the passed clustering key full by filling its
     * missing trailing components with empty values.
     * This is used to represesent clustering keys of rows in compact tables that may be non-full.
     * Returns whether a key wasn't full before the call.
     */
    static bool make_full(const schema& s, clustering_key_prefix& ck);    friend std::ostream& operator<<(std::ostream& out, const clustering_key_prefix& ckp);
};

