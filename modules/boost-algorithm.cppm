/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for Boost.Algorithm (string utilities).

module;

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/cxx11/is_sorted.hpp>
#include <boost/algorithm/cxx11/iota.hpp>
#include <boost/algorithm/string/join.hpp>

export module boost:algorithm;

export namespace boost {
    // boost/algorithm/string — brought into boost:: by the library
    using boost::split;
    using boost::is_any_of;
    using boost::iequals;
    using boost::equals;
    using boost::trim;
    using boost::trim_copy;
    using boost::trim_all;
    using boost::to_lower;
    using boost::to_lower_copy;
    using boost::to_upper;
    using boost::to_upper_copy;
    using boost::erase_all;
    using boost::replace_all;
    using boost::replace_all_copy;
    using boost::token_compress_on;

    namespace algorithm {
        using boost::algorithm::split;
        using boost::algorithm::is_any_of;
        using boost::algorithm::iequals;
        using boost::algorithm::replace_all;
        using boost::algorithm::replace_all_copy;
        using boost::algorithm::erase_all;
        using boost::algorithm::to_lower;
        using boost::algorithm::trim;
        using boost::algorithm::iota;
        using boost::algorithm::join;
    }
}
