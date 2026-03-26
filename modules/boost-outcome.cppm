/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for Boost.Outcome.

module;

#include <boost/outcome/bad_access.hpp>
#include <boost/outcome/basic_result.hpp>
#include <boost/outcome/config.hpp>
#include <boost/outcome/policy/base.hpp>
#include <boost/outcome/result.hpp>

export module boost:outcome;

export namespace boost::outcome_v2 {
    using BOOST_OUTCOME_V2_NAMESPACE::success;
    using BOOST_OUTCOME_V2_NAMESPACE::failure;
    using BOOST_OUTCOME_V2_NAMESPACE::result;
    using BOOST_OUTCOME_V2_NAMESPACE::basic_result;
    using BOOST_OUTCOME_V2_NAMESPACE::is_basic_result;
    using BOOST_OUTCOME_V2_NAMESPACE::bad_result_access;

    namespace policy {
        using BOOST_OUTCOME_V2_NAMESPACE::policy::base;
    }
}
