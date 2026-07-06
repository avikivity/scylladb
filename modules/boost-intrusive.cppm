/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for Boost.Intrusive.

module;

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/parent_from_member.hpp>

export module boost:intrusive;

export namespace boost::intrusive {
    using boost::intrusive::list;
    using boost::intrusive::list_base_hook;
    using boost::intrusive::list_member_hook;
    using boost::intrusive::slist;
    using boost::intrusive::slist_member_hook;
    using boost::intrusive::set;
    using boost::intrusive::multiset;
    using boost::intrusive::set_member_hook;
    using boost::intrusive::unordered_set;
    using boost::intrusive::unordered_set_base_hook;
    using boost::intrusive::store_hash;
    using boost::intrusive::power_2_buckets;
    using boost::intrusive::compare_hash;
    using boost::intrusive::bucket_traits;
    using boost::intrusive::equal;
    using boost::intrusive::hash;
    using boost::intrusive::member_hook;
    using boost::intrusive::base_hook;
    using boost::intrusive::constant_time_size;
    using boost::intrusive::link_mode;
    using boost::intrusive::link_mode_type;
    using boost::intrusive::auto_unlink;
    using boost::intrusive::normal_link;
    using boost::intrusive::cache_last;
    using boost::intrusive::compare;
    using boost::intrusive::get_parent_from_member;
}

export namespace boost::intrusive::detail {
    using boost::intrusive::detail::destructor_impl;
}
