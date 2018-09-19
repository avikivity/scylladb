/*
 * Copyright 2018 ScyllaDB
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

#include "i_partitioner.hh"
#include "serializer_impl.hh"

namespace ser {

template <>
struct serializer<dht::token_data> {
    using original = serializer<bytes>;

    template <typename Input>
    static dht::token_data read(Input& v) {
        auto tmp = bytes(original::read(v));
        if (tmp.size() != 8) {
            throw std::runtime_error("deserializing badly sized token_data");
        }
        dht::token_data ret;
        std::copy_n(tmp.begin(), 8, ret.begin());
        return ret;
    }
    template <typename Output>
    static void write(Output& out, dht::token_data v) {
        auto tmp = bytes(bytes::initialized_later{}, 8);
        std::copy_n(v.begin(), 8, tmp.begin());
        original::write(out, tmp);
    }
    template <typename Input>
    static void skip(Input& v) {
        read(v);
    }

};

}
