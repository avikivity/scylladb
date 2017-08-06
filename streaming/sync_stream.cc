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

sync_stream_server::sync_stream_server(netw::messaging_service& ms, data_source& source, sync_stream_client& client)
        : _ms(ms)
        , _source(source)
        , _client(client) {
    // FIXME: unregister
    ms.register_stream_range_sync([this] (const rpc::client_info& cinfo, UUID plan_id, sstring description, UUID table, dht::token_range_vector ranges) {
        return do_stream_range_sync(cinfo.addr, plan_id, description, table, std::move(ranges));
    });
    ms.register_stream_receive_range_sync([this] (const rpc::client_info& cinfo, UUID plan_id, sstring description, UUID table, dht::token_range_vector ranges) {
        return do_stream_receive_range_sync(cinfo.addr, plan_id, description, table, std::move(ranges));
    });
}

future<>
sync_stream_server::stop() {
    _ms.unregister_stream_range_sync();
    _ms.unregister_stream_receive_range_sync();
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
sync_stream_server::do_stream_receive_range_sync(gms::inet_address from, UUID plan_id, sstring description, UUID table, dht::token_range_vector ranges) {
    // FIXME: sharded_from_this()!
    return with_gate(_gate, [=, ranges = std::move(ranges)] {
        return _client.stream_ranges_sync(from, plan_id, description, table, std::move(ranges));
    });
}

namespace {

// TODO: complete & donate to seastar
struct background_work_manager {
    unsigned nr_running = 0;
    promise<> done;
    std::exception_ptr error;
    bool loop_exited = false;

    // run func in the background, but keep track of it and don't set done until it is complete
    template <typename Func>
    future<> run_worker(Func func) {
        if (error) {
            // break out of the main loop
            return make_exception_future<>(std::runtime_error("dropping streaming mutation due to previous error"));
        }
        ++nr_running;
        func().then_wrapped([this] (future<> worker_ret) {
            if (worker_ret.failed()) {
                // We only keep track of the first error we see
                if (!error) {
                    error = worker_ret.get_exception();
                } else {
                    worker_ret.ignore_ready_future();
                }
            }
            --nr_running;
            // Are we the last worker to complete?
            if (!nr_running) {
                if (error) {
                    // yes, signal error
                    done.set_exception(std::move(error));
                } else if (loop_exited) {
                    // yes, signal all's well
                    done.set_value();
                }
                // no, maybe another worker will be spawned
            }
        });
        // Let worker run; run(), below, will wait for it.
        return make_ready_future<>();
    }

    // run main_loop and keep track of workers too. Collect errors from
    // either, and don't return until both the main loop and workers are done.
    template <typename Func>
    future<>
    run(Func main_loop) {
        return main_loop().then_wrapped([this] (future<> loop_ret) {
            loop_exited = true;
            if (!nr_running && !error) {
                // all the workers already completed, so up to us
                done.set_value();
            }
            done.get_future().then_wrapped([loop_ret = std::move(loop_ret)] (future<> worker_ret) mutable {
                // loop_ret can be spurious, so priority to worker_ret
                if (worker_ret.failed()) {
                    loop_ret.ignore_ready_future();
                    return worker_ret;
                }
                return std::move(loop_ret);
            });
        });
    }
};

}

future<>
sync_stream_server::do_stream_range_sync_on_shard(gms::inet_address to, UUID plan_id, sstring description, UUID table, dht::token_range_vector ranges) {
  return do_with(boost::copy_range<dht::partition_range_vector>(ranges | boost::adaptors::transformed(dht::to_partition_range)),
          background_work_manager{},
          [=] (dht::partition_range_vector& prs, background_work_manager& bwm) {
    return with_gate(_gate, [=, &prs, &bwm] {
      auto reader = _source.make_reader(table, prs);
      return bwm.run([this, to, plan_id, &bwm, reader = std::move(reader)] () mutable {
        return repeat([this, to, plan_id, &bwm, reader = std::move(reader)] () mutable {
            return reader().then([this, to, plan_id, &bwm] (streamed_mutation_opt smo) {
                _gate.check();
                if (!smo) {
                    return make_ready_future<stop_iteration>(true);
                }
                return fragment_and_freeze(std::move(*smo), [this, to, plan_id, &bwm] (frozen_mutation fm, bool fragmented ){
                 return bwm.run_worker([this, fm = std::move(fm), to, plan_id, fragmented] () mutable {
                  auto weight = std::min<size_t>(fm.representation().size(), window_size());
                  return with_semaphore(_inflight_bytes, weight, [this, fm = std::move(fm), to, plan_id, fragmented] () mutable {
                    return _ms.send_stream_mutation({to, 0}, plan_id, fm, 0, fragmented);
                  });
                 });
                }).then([] {
                    return stop_iteration::no;
                });
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
      auto& sem = _per_server_limit.emplace(from, semaphore{4}).first->second;
      return with_semaphore(sem, 1, [=, ranges = std::move(ranges)] {
        _gate.check();
        return _ms.send_stream_range_sync({from, 0}, plan, description, table, std::move(ranges)).then_wrapped([this, plan, table, ranges] (future<> result) {
            if (result.failed()) {
                _sink.abort_plan(plan, table, ranges);
            } else {
                _sink.commit_plan(plan, table, ranges);
            }
            return result;
        });
      });
    });
}

future<>
sync_stream_client::stream_receive_ranges_sync(gms::inet_address to, UUID plan, sstring description, UUID table, dht::token_range_vector ranges) {
    return with_gate(_gate, [=, ranges = std::move(ranges)] {
        auto& sem = _per_server_limit_receive.emplace(to, semaphore{4}).first->second;
        return with_semaphore(sem, 1, [=, ranges = std::move(ranges)] {
            _gate.check();
            return _ms.send_stream_receive_range_sync({to, 0}, plan, description, table, std::move(ranges));
        });
    });
}

}
