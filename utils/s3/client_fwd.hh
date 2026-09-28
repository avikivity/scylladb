/*
 * Copyright (C) 2024-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

#pragma once

#include "utils/upload_progress.hh"
import std.compat;

namespace s3 {
class client;
using upload_progress = utils::upload_progress;
}
