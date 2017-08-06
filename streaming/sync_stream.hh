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
#include <seastar/core/sstring.hh>
#include <seastar/core/sharded.hh>
#include <seastar/core/gate.hh>
#include <seastar/core/semaphore.hh>
#include "gms/inet_address.hh"
#include "dht/i_partitioner.hh"
#include "utils/UUID.hh"
#include "message/messaging_service_fwd.hh"
#include "seastarx.hh"
#include <unordered_map>

class mutation_reader;
class frozen_mutation;

namespace streaming {

using UUID = utils::UUID;

class data_source;
class data_sink;

class sync_stream_client;

class sync_stream_server : public peering_sharded_service<sync_stream_server> {
    netw::messaging_service& _ms;
    data_source& _source;
    sync_stream_client& _client;
    gate _gate;
    static size_t window_size() { return 1 << 22; };
    semaphore _inflight_bytes{window_size()};
public:
    sync_stream_server(netw::messaging_service& ms, data_source& source, sync_stream_client& client);
    future<> stop();
private:
    // server side:
    future<> do_stream_range_sync(gms::inet_address to, UUID plan, sstring description, UUID table, dht::token_range_vector ranges);
    future<> do_stream_range_sync_on_shard(gms::inet_address to, UUID plan, sstring description, UUID table, dht::token_range_vector ranges);
    future<> do_stream_receive_range_sync(gms::inet_address to, UUID plan, sstring description, UUID table, dht::token_range_vector ranges);
};

class sync_stream_client {
    netw::messaging_service& _ms;
    data_sink& _sink;
    gate _gate;
    std::unordered_map<gms::inet_address, semaphore> _per_server_limit;
    std::unordered_map<gms::inet_address, semaphore> _per_server_limit_receive;
public:
    sync_stream_client(netw::messaging_service& ms, data_sink& sink);
    future<> stop();
public:
    // client side:
    future<> stream_ranges_sync(gms::inet_address from, UUID plan, sstring description, UUID table, dht::token_range_vector ranges);
    future<> stream_receive_ranges_sync(gms::inet_address to, UUID plan, sstring description, UUID table, dht::token_range_vector ranges);
};

}
