/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for Boost.Signals2.

module;

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal_type.hpp>
#include <boost/signals2/dummy_mutex.hpp>

export module boost:signals2;

export namespace boost::signals2 {
    using boost::signals2::signal_type;
    using boost::signals2::scoped_connection;
    using boost::signals2::dummy_mutex;

    namespace keywords {
        using boost::signals2::keywords::mutex_type;
    }
}
