/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for Boost.Regex (including ICU support).

module;

#include <boost/regex.hpp>
#include <boost/regex/icu.hpp>

export module boost:regex;

export namespace boost {
    using boost::regex;
    using boost::smatch;
    using boost::cmatch;
    using boost::match_results;
    using boost::regex_match;
    using boost::regex_search;
    using boost::regex_replace;
    using boost::regex_error;
    using boost::sregex_iterator;
    using boost::sub_match;
    using boost::operator<<;

    // ICU support
    using boost::u32regex;
    using boost::make_u32regex;
    using boost::u32regex_match;

    namespace regex_constants {
        using boost::regex_constants::match_continuous;
        using boost::regex_constants::icase;
    }
}
