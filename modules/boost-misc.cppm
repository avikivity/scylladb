/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for miscellaneous Boost libraries:
// lexical_cast, functional/hash, date_time, dynamic_bitset,
// io/ios_state, iterator, locale, make_shared, circular_buffer,
// heap, implicit_cast, mp11, units, variant, any, accumulators.

module;

#include <boost/lexical_cast.hpp>
#include <boost/functional/hash.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <boost/date_time/gregorian/greg_date.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/locale/encoding_utf.hpp>
#include <boost/locale/encoding.hpp>
#include <boost/make_shared.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/heap/binomial_heap.hpp>
#include <boost/implicit_cast.hpp>
#include <boost/mp11/algorithm.hpp>
#include <boost/units/detail/utility.hpp>
#include <boost/variant/variant.hpp>
#include <boost/any.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/max.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/p_square_quantile.hpp>
#include <boost/accumulators/statistics/extended_p_square.hpp>
#include <boost/accumulators/statistics/extended_p_square_quantile.hpp>
#include <boost/accumulators/framework/accumulator_set.hpp>
#include <boost/accumulators/framework/features.hpp>
#include <boost/accumulators/statistics/error_of_mean.hpp>
#include <boost/version.hpp>

export module boost:misc;

export namespace boost {
    // lexical_cast
    using boost::lexical_cast;
    using boost::bad_lexical_cast;

    // functional/hash
    using boost::hash;
    using boost::hash_combine;
    using boost::hash_value;

    // dynamic_bitset
    using boost::dynamic_bitset;

    // iterator
    using boost::iterator_facade;
    using boost::iterator_core_access;
    using boost::transform_iterator;
    using boost::counting_iterator;
    using boost::make_transform_iterator;
    using boost::make_counting_iterator;

    // boost::iterator_facade defines its comparison/arithmetic operators
    // as hidden friends in namespace boost::iterators.  Re-export them so
    // ADL on a transform_iterator (etc.) value finds them in importer TUs.
    namespace iterators {
        using boost::iterators::operator==;
        using boost::iterators::operator!=;
        using boost::iterators::operator<;
        using boost::iterators::operator<=;
        using boost::iterators::operator>;
        using boost::iterators::operator>=;
        using boost::iterators::operator-;
        using boost::iterators::operator+;
    }

    // make_shared
    using boost::make_shared;

    // circular_buffer
    using boost::circular_buffer;

    // implicit_cast
    using boost::implicit_cast;

    // variant
    using boost::variant;
    using boost::apply_visitor;
    using boost::static_visitor;

    // any
    using boost::any;
    using boost::any_cast;
    using boost::unsafe_any_cast;

    // typeindex (used by boost::any internally; the operators on
    // type_index_facade<> live in boost::typeindex, so they must be
    // re-exported for unqualified-name lookup in importers).
    namespace typeindex {
        using boost::typeindex::type_info;
        using boost::typeindex::type_id;
        using boost::typeindex::type_index;
        using boost::typeindex::stl_type_index;
        using boost::typeindex::operator==;
        using boost::typeindex::operator!=;
        using boost::typeindex::operator<;
        using boost::typeindex::operator<=;
        using boost::typeindex::operator>;
        using boost::typeindex::operator>=;
    }

    namespace posix_time {
        using boost::posix_time::ptime;
        using boost::posix_time::time_duration;
        using boost::posix_time::milliseconds;
        using boost::posix_time::from_time_t;
        using boost::posix_time::time_input_facet;
        using boost::posix_time::time_facet;
        using boost::posix_time::second_clock;
        using boost::posix_time::operator<<;
        using boost::posix_time::operator>>;
        using boost::posix_time::operator-;
        using boost::posix_time::operator+=;
        // boost::date_time::time_facet<ptime>::put() ends up calling
        // `to_tm(time_arg)` *unqualified*, where `time_arg` is a ptime, so
        // ADL searches boost::posix_time.  Same clang/modules reachability
        // limitation as boost::multiprecision::backends::eval_*: the
        // declaration in the GMF isn't found by ADL from an importer TU
        // unless we re-export it.
        using boost::posix_time::to_tm;
    }

    namespace gregorian {
        using boost::gregorian::date;
        using boost::gregorian::bad_year;
        // Counterpart to boost::posix_time::to_tm above: gregorian::to_tm is
        // called from inside posix_time::to_tm, again via ADL on a
        // gregorian::date argument.
        using boost::gregorian::to_tm;
    }

    namespace date_time {
        using boost::date_time::c_local_adjustor;
    }

    namespace io {
        using boost::io::ios_flags_saver;
    }

    namespace locale::conv {
        using boost::locale::conv::utf_to_utf;
        using boost::locale::conv::stop;
    }

    namespace heap {
        using boost::heap::binomial_heap;
        using boost::heap::compare;
        using boost::heap::allocator;
        using boost::heap::constant_time_size;
    }

    namespace mp11 {
        using boost::mp11::mp_with_index;
        using boost::mp11::mp_contains;
    }

    namespace units::detail {
        using boost::units::detail::demangle;
    }

    namespace accumulators {
        using boost::accumulators::accumulator_set;
        using boost::accumulators::features;
        using boost::accumulators::extractor;

        // boost::accumulators::extract::{mean,error_of} are const variables
        // with internal linkage and so cannot be re-exported through a
        // `using`-declaration.  Provide externally-linked replicas.
        inline constexpr extractor<tag::mean> mean_v{};
        template <class Feature>
        inline constexpr extractor<tag::error_of<Feature>> error_of_v{};

        namespace tag {
            using boost::accumulators::tag::mean;
            using boost::accumulators::tag::max;
            using boost::accumulators::tag::extended_p_square_quantile;
            using boost::accumulators::tag::error_of;
        }
    }
}
