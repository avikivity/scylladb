/*
 * Copyright (C) 2016-present ScyllaDB
 *
 * Modified by ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

#pragma once

// Direct access to the generated CQL parser. This brings in the ANTLR3 C++
// runtime, which includes standard library headers textually, so include it
// only where a parser rule must be invoked directly (cql3/util.cc and tests).
// Everything else should use the type-erased parse_*() functions in
// cql3/util.hh.

import std.compat;

#include "cql3/CqlParser.hpp"
#include "cql3/error_collector.hh"
#include "cql3/dialect.hh"

namespace utils {
template <mutable_view> class chunked_string_basic_view;
using chunked_string_view = chunked_string_basic_view<mutable_view::no>;
}

namespace cql3::util {

void do_with_parser_impl(utils::chunked_string_view cql, dialect d, noncopyable_function<void (cql3_parser::CqlParser& p)> func);

template <typename Func, typename Result = cql3_parser::unwrap_uninitialized_t<std::invoke_result_t<Func, cql3_parser::CqlParser&>>>
Result do_with_parser(utils::chunked_string_view cql, dialect d, Func&& f) {
    std::optional<Result> ret;
    do_with_parser_impl(cql, d, [&] (cql3_parser::CqlParser& parser) {
        ret.emplace(f(parser));
    });
    return std::move(*ret);
}

}
