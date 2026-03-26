/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for Boost.Container.

module;

#include <boost/container/static_vector.hpp>
#include <boost/container/deque.hpp>

export module boost:container;

export namespace boost::container {
    using boost::container::static_vector;
    using boost::container::deque;
}
