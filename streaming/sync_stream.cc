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


#include <seastar/rpc/rpc_types.hh>
#include "sync_stream.hh"
#include "streaming_database_interface.hh"
#include "frozen_mutation.hh"
#include "seastarx.hh"
#include "message/messaging_service.hh"

namespace streaming {

sync_stream_server::sync_stream_server(netw::messaging_service& ms, data_source& source)
        : _ms(ms)
        , _source(source) {
    // FIXME: unregister
    ms.register_stream_range_sync([this] (const rpc::client_info& cinfo, UUID plan_id, sstring description, UUID table, dht::token_range_vector ranges) {
        return do_stream_range_sync(cinfo.addr, plan_id, description, table, std::move(ranges));
    });
}

future<>
sync_stream_server::stop() {
    _ms.unregister_stream_range_sync();
    return _gate.close();
}

future<>
sync_stream_server::do_stream_range_sync(gms::inet_address to, UUID plan_id, sstring description, UUID table, dht::token_range_vector ranges) {
    // FIXME: sharded_from_this()!
    return with_gate(_gate, [=, ranges = std::move(ranges)] {
        return container().invoke_on_all(&sync_stream_server::do_stream_range_sync_on_shard, to, plan_id, description, table, ranges);
    });
}


future<>
sync_stream_server::do_stream_range_sync_on_shard(gms::inet_address to, UUID plan_id, sstring description, UUID table, dht::token_range_vector ranges) {
  return do_with(boost::copy_range<dht::partition_range_vector>(ranges | boost::adaptors::transformed(dht::to_partition_range)), [=] (dht::partition_range_vector& prs) {
    return with_gate(_gate, [=, &prs] {
        auto reader = _source.make_reader(table, prs);
        return repeat([this, to, plan_id, reader = std::move(reader)] () mutable {
            return reader().then([this, to, plan_id] (streamed_mutation_opt smo) {
                _gate.check();
                if (!smo) {
                    return make_ready_future<stop_iteration>(true);
                }
                return fragment_and_freeze(std::move(*smo), [this, to, plan_id] (frozen_mutation fm, bool fragmented ){
                    return _ms.send_stream_mutation({to, 0}, plan_id, fm, 0, fragmented);
                }).then([] {
                    return stop_iteration::no;
                });
            });
        });
    });
  });
}

sync_stream_client::sync_stream_client(netw::messaging_service& ms, data_sink& sink)
        : _ms(ms)
        , _sink(sink) {
}

future<>
sync_stream_client::stop() {
    return _gate.close();
}

future<>
sync_stream_client::stream_ranges_sync(gms::inet_address from, UUID plan, sstring description, UUID table, dht::token_range_vector ranges) {
    return with_gate(_gate, [=, ranges = std::move(ranges)] {
        return _ms.send_stream_range_sync({from, 0}, plan, description, table, std::move(ranges)).then_wrapped([this, plan, table, ranges] (future<> result) {
            if (result.failed()) {
                _sink.abort_plan(plan, table, ranges);
            } else {
                _sink.commit_plan(plan, table, ranges);
            }
            return result;
        });
    });
}

}
