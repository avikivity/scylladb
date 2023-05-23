/*
 * Copyright (C) 2015-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include <seastar/core/sstring.hh>
#include <seastar/core/future-util.hh>
#include <seastar/core/align.hh>
#include <seastar/core/aligned_buffer.hh>
#include <seastar/util/closeable.hh>

#include "sstables/sstables.hh"
#include "sstables/key.hh"
#include "sstables/compress.hh"
#include "test/lib/scylla_test_case.hh"
#include <seastar/testing/thread_test_case.hh>
#include "schema/schema.hh"
#include "schema/schema_builder.hh"
#include "replica/database.hh"
#include "sstables/metadata_collector.hh"
#include "sstables/sstable_writer.hh"
#include "sstables/sstable_directory.hh"
#include <memory>
#include "test/boost/sstable_test.hh"
#include <seastar/core/seastar.hh>
#include <seastar/core/do_with.hh>
#include "compaction/compaction_manager.hh"
#include "test/lib/tmpdir.hh"
#include "dht/i_partitioner.hh"
#include "dht/murmur3_partitioner.hh"
#include "range.hh"
#include "partition_slice_builder.hh"
#include "test/lib/mutation_assertions.hh"
#include "counters.hh"
#include "cell_locking.hh"
#include "test/lib/simple_schema.hh"
#include "replica/memtable-sstable.hh"
#include "test/lib/index_reader_assertions.hh"
#include "test/lib/flat_mutation_reader_assertions.hh"
#include "test/lib/make_random_string.hh"
#include "compatible_ring_position.hh"
#include "mutation/mutation_compactor.hh"
#include "service/priority_manager.hh"
#include "db/config.hh"
#include "mutation_writer/partition_based_splitting_writer.hh"

#include <stdio.h>
#include <ftw.h>
#include <unistd.h>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/algorithm/cxx11/is_sorted.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/icl/interval_map.hpp>
#include "test/lib/test_services.hh"
#include "test/lib/cql_test_env.hh"
#include "test/lib/reader_concurrency_semaphore.hh"
#include "test/lib/sstable_utils.hh"
#include "test/lib/random_utils.hh"
#include "test/lib/test_utils.hh"
#include "readers/from_mutations_v2.hh"
#include "readers/from_fragments_v2.hh"
#include "test/lib/random_schema.hh"
#include "test/lib/exception_utils.hh"

SEASTAR_TEST_CASE(test_validate_checksums) {
    return test_env::do_with_async([&] (test_env& env) {
        auto random_spec = tests::make_random_schema_specification(
                get_name(),
                std::uniform_int_distribution<size_t>(1, 4),
                std::uniform_int_distribution<size_t>(2, 4),
                std::uniform_int_distribution<size_t>(2, 8),
                std::uniform_int_distribution<size_t>(2, 8));
        auto random_schema = tests::random_schema{tests::random::get_int<uint32_t>(), *random_spec};

        testlog.info("Random schema:\n{}", random_schema.cql());

        const auto muts = tests::generate_random_mutations(random_schema).get();
    });
}
