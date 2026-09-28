/*
 * Copyright (C) 2025-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

#pragma once

import seastar;

seastar::future<const char *> __crypt_sha512(const char *key, const char *setting, char *output);
