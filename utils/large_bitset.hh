/*
 * Copyright (C) 2015-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

// A bitset containing a very large number of bits, so it uses fragmented
// storage in order not to stress the memory allocator.

#pragma once

