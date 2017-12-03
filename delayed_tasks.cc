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

#include "delayed_tasks.hh"
#include <seastar/core/reactor.hh>  // seastar's timer.hh is not self-contained

delayed_tasks::waiter::waiter(clock::duration d) : _timer([this] { _done.set_value(); }) {
    _timer.arm(d);
}

delayed_tasks::waiter::~waiter() {
    if (_timer.armed()) {
        _timer.cancel();
        _done.set_exception(cancelled());
    }
}

future<> delayed_tasks::waiter::get_future() noexcept {
    return _done.get_future();
}

void delayed_tasks::schedule_after(clock::duration d, noncopyable_function<future<> ()> f) {
    _logger.trace("Adding scheduled task.");

    auto iter = _waiters.insert(_waiters.end(), std::make_unique<waiter>(d));
    auto& w = *iter;

    w->get_future().then([this, f = std::move(f)] () mutable {
        _logger.trace("Running scheduled task.");
        return f();
    }).then([this, iter] {
         // We'll only get here if the instance is still alive, since otherwise the future will be resolved to
         // `cancelled`.
        _waiters.erase(iter);
    }).handle_exception_type([](const cancelled&) {
        // Nothing.
        return make_ready_future<>();
    });
}

void delayed_tasks::cancel_all() {
    _waiters.clear();
}

logging::logger delayed_tasks::_logger("delayed_tasks");
