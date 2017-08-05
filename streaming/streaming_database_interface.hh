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


#include <seastar/core/future.hh>
#include "gms/inet_address.hh"
#include "dht/i_partitioner.hh"
#include "utils/UUID.hh"
#include "seastarx.hh"

class mutation_reader;
class frozen_mutation;


namespace streaming {

using UUID = utils::UUID;

// Interface between database and streaming, side that sends data
class data_source {
public:
    virtual ~data_source() = default;
    virtual mutation_reader make_reader(UUID table, const dht::partition_range_vector& range) = 0;
};

// Interface between database and streaming, side that receives data
class data_sink {
public:
    virtual ~data_sink() = default;
    virtual future<> put_mutation(UUID plan_id, const frozen_mutation& fm, bool fragmented) = 0;
    // After put_mutation() is called with a plan_id (perhaps multiple times), must call commit_plan or abort_plan with same plan_id
    virtual future<> commit_plan(UUID plan_id, UUID table, const dht::token_range_vector& range) = 0;
    virtual future<> abort_plan(UUID plan_id, UUID table, const dht::token_range_vector& range) = 0;
};

}
