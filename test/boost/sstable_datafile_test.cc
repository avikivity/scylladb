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


namespace tests {

void decorate_with_timestamps(const schema& schema, std::mt19937& engine, timestamp_generator& ts_gen, expiry_generator exp_gen,
        data_model::mutation_description::value& value);

}

void tests::random_schema::add_row(std::mt19937& engine, data_model::mutation_description& md, data_model::mutation_description::key ckey,
        timestamp_generator ts_gen, expiry_generator exp_gen) {
    value_generator gen;
    for (const auto& cdef : _schema->regular_columns()) {
        auto value = gen.generate_value(engine, *cdef.type);
        decorate_with_timestamps(*_schema, engine, ts_gen, exp_gen, value);
        md.add_clustered_cell(ckey, cdef.name_as_text(), std::move(value));
    }
    if (auto ts = ts_gen(engine, timestamp_destination::row_marker, api::min_timestamp); ts != api::missing_timestamp) {
        if (auto expiry_opt = exp_gen(engine, timestamp_destination::row_marker)) {
            md.add_clustered_row_marker(ckey, tests::data_model::mutation_description::row_marker(ts, expiry_opt->ttl, expiry_opt->expiry_point));
        } else {
            md.add_clustered_row_marker(ckey, ts);
        }
    }
}

future<std::vector<mutation>> my_coroutine(
        uint32_t seed,
        tests::random_schema& random_schema,
        tests::timestamp_generator ts_gen,
        tests::expiry_generator exp_gen) {
    auto engine = std::mt19937(seed);
    const auto partition_count = 2;
    std::vector<mutation> muts;
    muts.reserve(partition_count);
    for (size_t pk = 0; pk != partition_count; ++pk) {
        auto mut = random_schema.new_mutation(pk);

        const auto clustering_row_count = 1;
        const auto range_tombstone_count = 1;
        auto ckeys = random_schema.make_ckeys(std::max(clustering_row_count, range_tombstone_count));

        random_schema.add_row(engine, mut, ckeys[0], ts_gen, exp_gen);
        co_await coroutine::maybe_yield();

        muts.emplace_back(mut.build(random_schema.schema()));
    }
    co_return std::move(muts);
}


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

        const auto muts = my_coroutine(7,
            random_schema,
            tests::default_timestamp_generator(),
            tests::no_expiry_expiry_generator()
        ).get();
    });
}
