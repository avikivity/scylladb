/*
 * Copyright (C) 2017-present ScyllaDB
 *
 * Modified by ScyllaDB
 */

/*
 * SPDX-License-Identifier: (AGPL-3.0-or-later and Apache-2.0)
 */

#include <seastar/core/shared_ptr.hh>

#include "index/secondary_index_manager.hh"

#include "cql3/statements/index_target.hh"
#include "cql3/util.hh"
#include "cql3/expr/expression.hh"
#include "index/target_parser.hh"
#include "db/query_context.hh"
#include "schema/schema_builder.hh"
#include "replica/database.hh"
#include "db/view/view.hh"
#include "concrete_types.hh"

#include <boost/range/adaptor/map.hpp>
#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace secondary_index {

index::index(const sstring& target_column, const index_metadata& im)
    : _im{im}
    , _target_type{cql3::statements::index_target::from_target_string(target_column)}
    , _target_column{cql3::statements::index_target::column_name_from_target_string(target_column)}
{}

bool index::depends_on(const column_definition& cdef) const {
    return cdef.name_as_text() == _target_column;
}

index::supports_expression_v index::supports_expression(const column_definition& cdef, const cql3::expr::oper_t op) const {
    using target_type = cql3::statements::index_target::target_type;
    auto collection_yes = supports_expression_v::from_bool_collection(true);
    if (cdef.name_as_text() != _target_column) {
        return supports_expression_v::from_bool(false);
    }

    switch (op) {
        case cql3::expr::oper_t::EQ:
            return supports_expression_v::from_bool(_target_type == target_type::regular_values); 
        case cql3::expr::oper_t::CONTAINS:
            if (cdef.type->is_set() && _target_type == target_type::keys) {
                return collection_yes;
            }
            if (cdef.type->is_list() && _target_type == target_type::collection_values) {
                return collection_yes;
            }
            if (cdef.type->is_map() && _target_type == target_type::collection_values) {
                return collection_yes;
            }
            return supports_expression_v::from_bool(false);
        case cql3::expr::oper_t::CONTAINS_KEY:
            if (cdef.type->is_map() && _target_type == target_type::keys) {
                return collection_yes;
            }
            return supports_expression_v::from_bool(false);
        default:
            return supports_expression_v::from_bool(false);
    }
}

index::supports_expression_v index::supports_subscript_expression(const column_definition& cdef, const cql3::expr::oper_t op) const {
    using target_type = cql3::statements::index_target::target_type;
    if (cdef.name_as_text() != _target_column) {
        return supports_expression_v::from_bool(false);
    }

    return supports_expression_v::from_bool_collection(op == cql3::expr::oper_t::EQ && _target_type == target_type::keys_and_values);
}

const index_metadata& index::metadata() const {
    return _im;
}

secondary_index_manager::secondary_index_manager(data_dictionary::table cf)
    : _cf{cf}
{}

void secondary_index_manager::reload() {
    const auto& table_indices = _cf.schema()->all_indices();
    auto it = _indices.begin();
    while (it != _indices.end()) {
        auto index_name = it->first;
        if (!table_indices.contains(index_name)) {
            it = _indices.erase(it);
        } else {
            ++it;
        }
    }
    for (const auto& index : _cf.schema()->all_indices()) {
        add_index(index.second);
    }
}

void secondary_index_manager::add_index(const index_metadata& im) {
}

static const data_type collection_keys_type(const abstract_type& t) {
    struct visitor {
        const data_type operator()(const abstract_type& t) {
            throw std::logic_error(format("collection_keys_type: only collections (maps, lists and sets) supported, but received {}", t.cql3_type_name()));
        }
        const data_type operator()(const list_type_impl& l) {
            return timeuuid_type;
        }
        const data_type operator()(const map_type_impl& m) {
            return m.get_keys_type();
        }
        const data_type operator()(const set_type_impl& s) {
            return s.get_elements_type();
        }
    };
    return visit(t, visitor{});
}

static const data_type collection_values_type(const abstract_type& t) {
    struct visitor {
        const data_type operator()(const abstract_type& t) {
            throw std::logic_error(format("collection_values_type: only maps and lists supported, but received {}", t.cql3_type_name()));
        }
        const data_type operator()(const map_type_impl& m) {
            return m.get_values_type();
        }
        const data_type operator()(const list_type_impl& l) {
            return l.get_elements_type();
        }
    };
    return visit(t, visitor{});
}

static const data_type collection_entries_type(const abstract_type& t) {
    struct visitor {
        const data_type operator()(const abstract_type& t) {
            throw std::logic_error(format("collection_entries_type: only maps supported, but received {}", t.cql3_type_name()));
        }
        const data_type operator()(const map_type_impl& m) {
            return tuple_type_impl::get_instance({m.get_keys_type(), m.get_values_type()});
        }
    };
    return visit(t, visitor{});
}


sstring index_table_name(const sstring& index_name) {
    return format("{}_index", index_name);
}

sstring index_name_from_table_name(const sstring& table_name) {
    if (table_name.size() < 7 || !boost::algorithm::ends_with(table_name, "_index")) {
        throw std::runtime_error(format("Table {} does not have _index suffix", table_name));
    }
    return table_name.substr(0, table_name.size() - 6); // remove the _index suffix from an index name;
}

static bytes get_available_column_name(const schema& schema, const bytes& root) {
    bytes accepted_name = root;
    int i = 0;
    while (schema.get_column_definition(accepted_name)) {
        accepted_name = root + to_bytes("_") + to_bytes(std::to_string(++i));
    }
    return accepted_name;
}

static bytes get_available_token_column_name(const schema& schema) {
    return get_available_column_name(schema, "idx_token");
}

static bytes get_available_computed_collection_column_name(const schema& schema) {
    return get_available_column_name(schema, "coll_value");
}

static data_type type_for_computed_column(cql3::statements::index_target::target_type target, const abstract_type& collection_type) {
    using namespace cql3::statements;
    switch (target) {
        case index_target::target_type::keys:               return collection_keys_type(collection_type);
        case index_target::target_type::keys_and_values:    return collection_entries_type(collection_type);
        case index_target::target_type::collection_values:  return collection_values_type(collection_type);
        default: throw std::logic_error("reached regular values or full when only collection index target types were expected");
    }
}

view_ptr secondary_index_manager::create_view_for_index(const index_metadata& im, bool new_token_column_computation) const {
    return {};
}

std::vector<index_metadata> secondary_index_manager::get_dependent_indices(const column_definition& cdef) const {
    return boost::copy_range<std::vector<index_metadata>>(_indices
           | boost::adaptors::map_values
           | boost::adaptors::filtered([&] (auto& index) { return index.depends_on(cdef); })
           | boost::adaptors::transformed([&] (auto& index) { return index.metadata(); }));
}

std::vector<index> secondary_index_manager::list_indexes() const {
    return boost::copy_range<std::vector<index>>(_indices | boost::adaptors::map_values);
}

bool secondary_index_manager::is_index(view_ptr view) const {
    return is_index(*view);
}

bool secondary_index_manager::is_index(const schema& s) const {
    return boost::algorithm::any_of(_indices | boost::adaptors::map_values, [&s] (const index& i) {
        return s.cf_name() == index_table_name(i.metadata().name());
    });
}

bool secondary_index_manager::is_global_index(const schema& s) const {
    return boost::algorithm::any_of(_indices | boost::adaptors::map_values, [&s] (const index& i) {
        return !i.metadata().local() && s.cf_name() == index_table_name(i.metadata().name());
    });
}

}
