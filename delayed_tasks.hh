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

#include <algorithm>
#include <chrono>
#include <memory>
#include <list>
#include <stdexcept>

#include <seastar/core/future.hh>
#include <seastar/core/timer.hh>
#include <seastar/util/noncopyable_function.hh>

#include "log.hh"
#include "seastarx.hh"

//
// Delay asynchronous tasks.
//
class delayed_tasks final {
public:
    using clock = std::chrono::steady_clock;
private:
    static logging::logger _logger;

    // A waiter is destroyed before the timer has elapsed.
    class cancelled : public std::exception {};

    class waiter final {
        timer<clock> _timer;
        promise<> _done{};
    public:
        explicit waiter(clock::duration d);
        ~waiter();
        future<> get_future() noexcept;
    };

    // `std::list` because iterators are not invalidated. We assume that look-up time is not a bottleneck.
    std::list<std::unique_ptr<waiter>> _waiters{};
public:
    //
    // Schedule the task `f` after d` has elapsed. If the instance goes out of scope before
    // the duration has elapsed, then the task is cancelled.
    //
    void schedule_after(clock::duration d, noncopyable_function<future<> ()> f);

    //
    // Cancel all scheduled tasks.
    //
    void cancel_all();
};

