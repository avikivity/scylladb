/*
 * Copyright (C) 2015-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

#include "test/lib/scylla_test_case.hh"
#include <seastar/testing/thread_test_case.hh>
#include "schema/schema.hh"

#include "test/lib/random_utils.hh"
#include "test/lib/random_schema.hh"


namespace tests {

void decorate_with_timestamps(const schema& schema, std::mt19937& engine, timestamp_generator& ts_gen, expiry_generator exp_gen,
        data_model::mutation_description::value& value);

}

void tests::random_schema::add_row(std::mt19937& engine, data_model::mutation_description& md, data_model::mutation_description::key ckey,
        timestamp_generator ts_gen, expiry_generator exp_gen) {
    value_generator gen;
    const auto& cdef = _schema->regular_columns()[0];
    {
        auto value = gen.generate_value(engine, *cdef.type);
        md.add_clustered_cell(ckey, cdef.name_as_text(), std::move(value));
    }
}

static auto ts_gen = tests::default_timestamp_generator();
static auto exp_gen = tests::no_expiry_expiry_generator();


future<std::vector<mutation>> my_coroutine(
        uint32_t seed,
        tests::random_schema& random_schema) {
    auto engine = std::mt19937(seed);
    const auto partition_count = 2;
    std::vector<mutation> muts;
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
        static auto random_spec = tests::make_random_schema_specification(
                get_name(),
                std::uniform_int_distribution<size_t>(1, 4),
                std::uniform_int_distribution<size_t>(2, 4),
                std::uniform_int_distribution<size_t>(2, 8),
                std::uniform_int_distribution<size_t>(2, 8));
        static auto random_schema = tests::random_schema{tests::random::get_int<uint32_t>(), *random_spec};

        return my_coroutine(7,
            random_schema
        ).discard_result();
}
