/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for Boost.ICL (Interval Container Library).

module;

#include <boost/icl/interval.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/interval_set.hpp>

export module boost:icl;

export namespace boost::icl {
    using boost::icl::interval_map;
    using boost::icl::interval_set;
    using boost::icl::interval;
    using boost::icl::left_open_interval;

    // Combiners / policies
    using boost::icl::partial_absorber;
    using boost::icl::inplace_max;
    using boost::icl::inter_section;

    // Free functions
    using boost::icl::is_left_closed;
    using boost::icl::is_right_closed;
    using boost::icl::contains;
    using boost::icl::within;

    // Bounds
    using boost::icl::interval_bounds;

    // Operators on interval_map / interval_set (must be re-exported for
    // unqualified-name lookup in importers).
    using boost::icl::operator+=;
    using boost::icl::operator-=;
    using boost::icl::operator&=;
    using boost::icl::operator|=;
    using boost::icl::operator==;
    using boost::icl::operator!=;
    using boost::icl::operator<<;
}
