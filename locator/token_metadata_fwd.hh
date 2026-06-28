/*
 * Copyright (C) 2023-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: (LicenseRef-ScyllaDB-Source-Available-1.1 and Apache-2.0)
 */

#pragma once

#include "seastarx.hh"

namespace locator {

class token_metadata;

using token_metadata_ptr = lw_shared_ptr<const token_metadata>;
using mutable_token_metadata_ptr = lw_shared_ptr<token_metadata>;

} // namespace locator
