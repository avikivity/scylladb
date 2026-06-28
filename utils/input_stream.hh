/*
 * Copyright (C) 2016-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

#pragma once

#include "bytes_ostream.hh"

import seastar;

namespace utils {

using input_stream = seastar::memory_input_stream<bytes_ostream::fragment_iterator>;

}
