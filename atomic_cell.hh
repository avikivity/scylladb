/*
 * Copyright (C) 2015 Cloudius Systems, Ltd.
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
#include "timestamp.hh"
#include "tombstone.hh"
#include "gc_clock.hh"
#include "utils/managed_bytes.hh"
#include "net/byteorder.hh"
#include "types.hh"
#include <boost/variant.hpp>
#include <cstdint>
#include <iostream>

/*
 * Represents atomic cell layout. Works on serialized form.
 *
 * Layout:
 *
 *  <live>  := <int8_t:flags><int64_t:timestamp>(<int32_t:expiry><int32_t:ttl>)?<value>
 *  <dead>  := <int8_t:    0><int64_t:timestamp><int32_t:deletion_time>
 */
class atomic_cell {
private:
    bool _live : 1;  // bit 0 in flags
    bool _has_ttl : 1; // bit 1 in flags
    api::timestamp_type _timestamp = 0;
    gc_clock::time_point _expiry_or_deletion_time = {};  // _expiry if _has_ttl, deletion_time is !_live
    gc_clock::duration _ttl { 0 }; // if _live && _has_ttl
    data_type _type;
    void* _value = nullptr;  // using data_type storage_* methods
public:
    atomic_cell() {
        _live = _has_ttl = false;
    }
    atomic_cell(const atomic_cell& x)
            : _live(x._live), _has_ttl(x._has_ttl), _timestamp(x._timestamp)
            , _expiry_or_deletion_time(x._expiry_or_deletion_time), _ttl(x._ttl)
            , _type(x._type) {
        auto& alctr = current_allocator();
        _value = alctr.alloc(_type->storage_migrate_fn(),
                _type->storage_size(),
                _type->storage_alignment());
        try {
            _type->storage_copy(x._value, _value);
        } catch (...) {
            alctr.free(_value);
            throw;
        }
    }
    atomic_cell(atomic_cell&& x) noexcept
            : _live(x._live), _has_ttl(x._has_ttl), _timestamp(x._timestamp)
            , _expiry_or_deletion_time(x._expiry_or_deletion_time), _type(std::move(x._type))
            , _value(x._value) {
        x._type = nullptr;
        x._value = nullptr;
    }
    ~atomic_cell() {
        if (_value) {
            _type->storage_destroy(_value);
            current_allocator().free(_value);
        }
    }
    atomic_cell& operator=(const atomic_cell& x) {
        if (this != &x) {
            auto n = x;
            std::swap(n, *this);
        }
        return *this;
    }
    atomic_cell& operator=(atomic_cell&& x) noexcept {
        if (this != &x) {
            auto n = std::move(x);
            std::swap(n, *this);
        }
        return *this;
    }
    const data_type& type() const { return _type; }
    bool is_live() const {
        return _live;
    }
    bool is_live_and_has_ttl() const {
        return _has_ttl;
    }
    bool is_live(tombstone t) const {
        return is_live() && !is_covered_by(t);
    }
    bool is_live(tombstone t, gc_clock::time_point now) const {
        return is_live() && !is_covered_by(t) && !has_expired(now);
    }
    bool is_dead() const {
        return !_live;
    }
    bool is_dead(gc_clock::time_point now) const {
        return is_dead() || has_expired(now);
    }
    bool is_covered_by(tombstone t) const {
        return timestamp() <= t.timestamp;
    }
    // Can be called on live and dead cells
    api::timestamp_type timestamp() const {
        return _timestamp;
    }
    // Can be called on live cells only
    data_value value() const {
        auto* mem = ::operator new(_type->native_value_size());
        try {
            _type->storage_copy_to_native(_value, mem);
            return data_value(mem, _type);
        } catch (...) {
            ::operator delete(mem);
            throw;
        }
    }
    // Can be called on live and dead cells
    bool has_expired(gc_clock::time_point now) const {
        return is_live_and_has_ttl() && expiry() < now;
    }
    // Can be called only when is_dead() is true.
    gc_clock::time_point deletion_time() const {
        assert(is_dead());
        return _expiry_or_deletion_time;
    }
    // Can be called only when is_live_and_has_ttl() is true.
    gc_clock::time_point expiry() const {
        assert(is_live_and_has_ttl());
        return _expiry_or_deletion_time;
    }
    // Can be called only when is_live_and_has_ttl() is true.
    gc_clock::duration ttl() const {
        assert(is_live_and_has_ttl());
        return gc_clock::duration(_ttl);
    }
    bool operator==(const atomic_cell& x) const {
        auto simple =
             _live == x._live
             && _has_ttl == x._has_ttl
             && _timestamp == x._timestamp
             && _expiry_or_deletion_time == x._expiry_or_deletion_time
             && _ttl == x._ttl
             && _type == x._type // FIXME: illegal to compare otherwise?
             && bool(_value) == bool(x._value);
        if (!simple) {
            return false;
        }
        // FIXME: compare natively
        auto v1 = _type->decompose(value());
        auto v2 = x._type->decompose(value());
        return v1 == v2;
    }
    bool operator!=(const atomic_cell& x) const {
        return !operator==(x);
    }
    bytes serialize() const;
    static atomic_cell from_bytes(data_type type, bytes_view v);
    static atomic_cell make_dead(api::timestamp_type timestamp, gc_clock::time_point deletion_time, data_type type) {
        auto ret = atomic_cell();
        ret._live = false;
        ret._timestamp = timestamp;
        ret._expiry_or_deletion_time = deletion_time;
        ret._type = std::move(type);
        return ret;
    }
    static atomic_cell make_live(api::timestamp_type timestamp, data_value value) {
        auto ret = atomic_cell();
        ret._live = true;
        ret._has_ttl = false;
        ret._timestamp = timestamp;
        ret._type = value.type();
        auto& alctr = current_allocator();
        auto v = alctr.alloc(ret._type->storage_migrate_fn(),
                ret._type->storage_size(),
                ret._type->storage_alignment());
        try {
            ret._type->storage_move_from_native(value._value, v);
            ret._value = v;
        } catch (...) {
            alctr.free(v);
            throw;
        }
        return ret;
    }
    static atomic_cell make_live(api::timestamp_type timestamp, data_value value, gc_clock::time_point expiry, gc_clock::duration ttl) {
        auto ret = make_live(timestamp, std::move(value));
        ret._has_ttl = true;
        ret._expiry_or_deletion_time = expiry;
        ret._ttl = ttl;
        return ret;
    }
    static atomic_cell make_live(api::timestamp_type timestamp, data_value value, ttl_opt ttl) {
        if (!ttl) {
            return make_live(timestamp, std::move(value));
        } else {
            return make_live(timestamp, std::move(value), gc_clock::now() + *ttl, *ttl);
        }
    }
    friend std::ostream& operator<<(std::ostream&, const atomic_cell&);
};

using atomic_cell_view = const atomic_cell&;

class collection_mutation_view;

// Represents a mutation of a collection.  Actual format is determined by collection type,
// and is:
//   set:  list of atomic_cell
//   map:  list of pair<atomic_cell, bytes> (for key/value)
//   list: tbd, probably ugly
class collection_mutation {
public:
    managed_bytes data;
    collection_mutation() {}
    collection_mutation(managed_bytes b) : data(std::move(b)) {}
    collection_mutation(collection_mutation_view v);
    operator collection_mutation_view() const;
};

class collection_mutation_view {
public:
    bytes_view data;
    bytes_view serialize() const { return data; }
    static collection_mutation_view from_bytes(bytes_view v) { return { v }; }
};

inline
collection_mutation::collection_mutation(collection_mutation_view v)
        : data(v.data) {
}

inline
collection_mutation::operator collection_mutation_view() const {
    return { data };
}

namespace db {
template<typename T>
class serializer;
}

// A variant type that can hold either an atomic_cell, or a serialized collection.
// Which type is stored is determined by the schema.
class atomic_cell_or_collection final {
    boost::variant<atomic_cell, managed_bytes> _data;

    template<typename T>
    friend class db::serializer;
private:
    atomic_cell_or_collection(managed_bytes&& data) : _data(std::move(data)) {}
public:
    atomic_cell_or_collection() = default;
    atomic_cell_or_collection(atomic_cell ac) : _data(std::move(ac)) {}
    static atomic_cell_or_collection from_atomic_cell(atomic_cell data) { return { std::move(data) }; }
    atomic_cell_view as_atomic_cell() const { return boost::get<atomic_cell_view>(_data); }
    atomic_cell_or_collection(collection_mutation cm) : _data(std::move(cm.data)) {}
    explicit operator bool() const {
        if (_data.empty()) {
            return false;
        }
        switch (_data.which()) {
        case 0: {
            auto& x = boost::get<const managed_bytes&>(_data);
            return !x.empty();
        }
        case 1:
            return !as_atomic_cell().serialize().empty();
        default:
            abort();
        }
    }
    static atomic_cell_or_collection from_collection_mutation(collection_mutation data) {
        return std::move(data.data);
    }
    collection_mutation_view as_collection_mutation() const {
        return collection_mutation_view{boost::get<const managed_bytes&>(_data)};
    }
    bytes serialize() const {
        switch (_data.which()) {
        case 0: {
            auto& x = boost::get<const managed_bytes&>(_data);
            bytes ret(bytes::initialized_later(), x.size());
            std::copy(x.begin(), x.end(), ret.begin());
            return ret;
        }
        case 1:
            return as_atomic_cell().serialize();
        default:
            abort();
        }
    }
    bool operator==(const atomic_cell_or_collection& other) const {
        return _data == other._data;
    }
    friend std::ostream& operator<<(std::ostream&, const atomic_cell_or_collection&);
};

class column_definition;

int compare_atomic_cell_for_merge(const atomic_cell_view left, atomic_cell_view right);
void merge_column(const column_definition& def,
        atomic_cell_or_collection& old,
        const atomic_cell_or_collection& neww);
