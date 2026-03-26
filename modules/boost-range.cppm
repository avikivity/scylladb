/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for Boost.Range.

module;

#include <boost/range/join.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/size.hpp>
#include <boost/range/numeric.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/heap_algorithm.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/algorithm/unique.hpp>
#include <boost/range/algorithm/find_end.hpp>
#include <boost/range/algorithm/partition.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <boost/range/algorithm_ext/push_back.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/map.hpp>

export module boost:range;

export namespace boost {
    using boost::get;
    using boost::join;
    using boost::combine;
    using boost::irange;
    using boost::size;
    using boost::make_iterator_range;
    using boost::copy_range;
    using boost::unique;
    using boost::reverse;
    using boost::equal;
    using boost::for_each;
    using boost::set_intersection;
    using boost::partition;

    namespace range {
        using boost::range::join;
        using boost::range::combine;
        using boost::range::copy;
        using boost::range::find_end;
        using boost::range::partition;
    }

    // boost::adaptors::{transformed,map_values,map_keys} are const objects
    // declared in an unnamed namespace and so have internal linkage;
    // they cannot be re-exported through `using`.  Provide externally-
    // linked replicas under a `_v` suffix so importer TUs can reach them
    // without falling back to textual includes of <boost/range/adaptor/*.hpp>.
    namespace adaptors {
        inline const range_detail::forwarder<range_detail::transform_holder> transformed_v{};
        inline const range_detail::map_values_forwarder map_values_v{};
        inline const range_detail::map_keys_forwarder map_keys_v{};
    }

    // operator| for the adaptors lives in boost::range_detail; re-export
    // it so ADL through the holder types finds it in importer TUs.
    namespace range_detail {
        using boost::range_detail::operator|;
    }

    // Postfix operator++ for iterator_facade-derived iterators (such as the
    // one boost::irange returns) is a namespace-scope function template in
    // boost::iterators, so ADL in an importer TU only finds it if exported.
    namespace iterators {
        using boost::iterators::operator++;
    }
}
