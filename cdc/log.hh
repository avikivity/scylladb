/*
 * Copyright (C) 2019-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

/*
 * This module manages CDC log tables. It contains facilities used to:
 * - perform schema changes to CDC log tables correspondingly when base tables are changed,
 * - perform writes to CDC log tables correspondingly when writes to base tables are made.
 */

#pragma once

