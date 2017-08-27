/*
 * Copyright (C) 2017 ScyllaDB
 *
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

#include <utility>
#include <functional>
#include <unordered_set>

namespace sstables {

class sstable;
class shared_sstable;

class sstable_reference_count {
    unsigned _count = 0;
private:
    sstable_reference_count() = default;
    sstable_reference_count(const sstable_reference_count&) = delete;
    void operator=(const sstable_reference_count&) = delete;
    sstable& as_sstable();
    const sstable& as_sstable() const;
    friend class sstable;
    friend class shared_sstable;
};

class shared_sstable {
    sstable_reference_count* _sst = nullptr;
private:
    void destroyed();
    explicit shared_sstable(sstable_reference_count* sst) : _sst(sst) {
    }
public:
    shared_sstable() = default;
    shared_sstable(const shared_sstable& x) noexcept : _sst(x._sst) {
        if (_sst) {
            ++_sst->_count;
        }
    }
    shared_sstable(shared_sstable&& x) noexcept : _sst(std::exchange(x._sst, nullptr)) {
    }
    ~shared_sstable() {
        if (_sst && !--_sst->_count) {
            destroyed();
        }
    }
    shared_sstable& operator=(const shared_sstable& x) noexcept {
        return operator=(shared_sstable(x));
    }
    shared_sstable& operator=(shared_sstable&& x) noexcept {
        if (this != &x) {
            this->~shared_sstable();
            new (this) shared_sstable(std::move(x));
        }
        return *this;
    }
    explicit operator bool() const {
        return _sst;
    }
    bool operator==(const shared_sstable& x) const {
        return _sst == x._sst;
    }
    bool operator!=(const shared_sstable& x) const {
        return !operator==(x);
    }
    sstable* get() const;
    sstable& operator*() const { return *get(); }
    sstable* operator->() const { return get(); }
    size_t hash_value() const {
        return std::hash<sstable_reference_count*>()(_sst);
    }
    friend class sstable;
};

using sstable_list = std::unordered_set<shared_sstable>;

}

namespace std {

template <>
struct hash<sstables::shared_sstable> {
    size_t operator()(const sstables::shared_sstable& x) const {
        return x.hash_value();
    }
};

}
