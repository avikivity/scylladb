/*
 * Copyright (C) 2017-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <seastar/core/coroutine.hh>
#include "auth/common.hh"

#include <seastar/core/shared_ptr.hh>

#include "cql3/query_processor.hh"
#include "cql3/statements/create_table_statement.hh"
#include "replica/database.hh"
#include "schema/schema_builder.hh"
#include "service/migration_manager.hh"
#include "timeout_config.hh"

namespace auth {

namespace meta {

constinit const std::string_view AUTH_KS("system_auth");
constinit const std::string_view USERS_CF("users");
constinit const std::string_view AUTH_PACKAGE_NAME("org.apache.cassandra.auth.");

}

static logging::logger auth_log("auth");

// Func must support being invoked more than once.
future<> do_after_system_ready(seastar::abort_source& as, seastar::noncopyable_function<future<>()> func) {
    struct empty_state { };
    return exponential_backoff_retry::do_until_value(1s, 1min, as, [func = std::move(func)] {
        return func().then_wrapped([] (auto&& f) -> std::optional<empty_state> {
            if (f.failed()) {
                auth_log.debug("Auth task failed with error, rescheduling: {}", f.get_exception());
                return { };
            }
            return { empty_state() };
        });
    }).discard_result();
}

static future<> create_metadata_table_if_missing_impl(
        std::string_view table_name,
        cql3::query_processor& qp,
        std::string_view cql,
        ::service::migration_manager& mm) {
    return make_ready_future<>();
}

future<> create_metadata_table_if_missing(
        std::string_view table_name,
        cql3::query_processor& qp,
        std::string_view cql,
        ::service::migration_manager& mm) noexcept {
    return futurize_invoke(create_metadata_table_if_missing_impl, table_name, qp, cql, mm);
}

future<> wait_for_schema_agreement(::service::migration_manager& mm, const replica::database& db, seastar::abort_source& as) {
    static const auto pause = [] { return sleep(std::chrono::milliseconds(500)); };

    return do_until([&db, &as] {
        as.check();
        return db.get_version() != replica::database::empty_version;
    }, pause).then([&mm, &as] {
        return do_until([&mm, &as] {
            as.check();
            return mm.have_schema_agreement();
        }, pause);
    });
}

::service::query_state& internal_distributed_query_state() noexcept {
#ifdef DEBUG
    // Give the much slower debug tests more headroom for completing auth queries.
    static const auto t = 30s;
#else
    static const auto t = 5s;
#endif
    static const timeout_config tc{t, t, t, t, t, t, t};
    static thread_local ::service::client_state cs(::service::client_state::internal_tag{}, tc);
    static thread_local ::service::query_state qs(cs, empty_service_permit());
    return qs;
}

}
