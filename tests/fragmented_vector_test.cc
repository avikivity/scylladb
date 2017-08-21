/*
 * Copyright (C) 2017 ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */

#define BOOST_TEST_MODULE core

#include <boost/test/included/unit_test.hpp>
#include <deque>
#include <random>
#include "utils/fragmented_vector.hh"

#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/irange.hpp>

#include "disk-error-handler.hh"

thread_local disk_error_signal_type commit_error;
thread_local disk_error_signal_type general_disk_error;

using disk_array = utils::fragmented_vector<uint64_t, 1024>;


using deque = std::deque<int>;

BOOST_AUTO_TEST_CASE(test_random_walk) {
    auto rand = std::default_random_engine();
    auto op_gen = std::uniform_int_distribution<unsigned>(0, 9);
    auto nr_dist = std::geometric_distribution<size_t>(0.7);
    deque d;
    disk_array c;
    for (auto i = 0; i != 1000000; ++i) {
        auto op = op_gen(rand);
        switch (op) {
        case 0: {
            auto n = rand();
            c.push_back(n);
            d.push_back(n);
            break;
        }
        case 1: {
            auto nr_pushes = nr_dist(rand);
            for (auto i : boost::irange(size_t(0), nr_pushes)) {
                (void)i;
                auto n = rand();
                c.push_back(n);
                d.push_back(n);
            }
            break;
        }
        case 2: {
            if (!d.empty()) {
                auto n = d.back();
                auto m = c.back();
                BOOST_REQUIRE_EQUAL(n, m);
                c.pop_back();
                d.pop_back();
            }
            break;
        }
        case 3: {
            c.reserve(nr_dist(rand));
            break;
        }
        case 4: {
            boost::sort(c);
            boost::sort(d);
            break;
        }
        case 5: {
            if (!d.empty()) {
                auto u = std::uniform_int_distribution<size_t>(0, d.size() - 1);
                auto idx = u(rand);
                auto m = c[idx];
                auto n = c[idx];
                BOOST_REQUIRE_EQUAL(m, n);
            }
            break;
        }
        case 6: {
            c.clear();
            d.clear();
            break;
        }
        case 7: {
            boost::reverse(c);
            boost::reverse(d);
            break;
        }
        case 8: {
            c.clear();
            d.clear();
            break;
        }
        case 9: {
            auto nr = nr_dist(rand);
            c.resize(nr);
            d.resize(nr);
            break;
        }
        default:
            abort();
        }
        BOOST_REQUIRE_EQUAL(c.size(), d.size());
        BOOST_REQUIRE(boost::equal(c, d));
    }
}
