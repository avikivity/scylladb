/*
 * Copyright (C) 2024-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */


#include "on_internal_error.hh"
#include "seastarx.hh"
import fmt;

static seastar::logger on_internal_error_logger("on_internal_error");

namespace utils {

[[noreturn]] void on_internal_error(std::string_view reason) {
    seastar::on_internal_error(on_internal_error_logger, reason);
}

[[noreturn]] void on_fatal_internal_error(std::string_view reason) noexcept {
    seastar::on_fatal_internal_error(on_internal_error_logger, reason);
}

[[noreturn]] void __assert_fail_on_internal_error(const char* expr, const char* file, int line, const char* function) {
    on_internal_error(fmt::format("Assertion '{}' failed at {}:{} in {}",
        expr, file, line, function));
}

} // namespace utils
