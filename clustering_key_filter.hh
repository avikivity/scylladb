/*
 * Copyright (C) 2016 ScyllaDB
 *
 * Modified by ScyllaDB
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

#include "schema_fwd.hh"
#include "query-request.hh"

namespace query {

class clustering_key_filter_ranges {
public:
    clustering_key_filter_ranges(const clustering_row_ranges& ranges);
    struct reversed { };
    clustering_key_filter_ranges(reversed, const clustering_row_ranges& ranges);
    clustering_key_filter_ranges(clustering_key_filter_ranges&& other) noexcept;
    clustering_key_filter_ranges& operator=(clustering_key_filter_ranges&& other) noexcept;
    static clustering_key_filter_ranges get_ranges(const schema& schema, const query::partition_slice& slice, const partition_key& key);
};

}
