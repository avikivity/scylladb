#include <atomic>
#include <filesystem>
#include <optional>
namespace compat {
template < typename T > using optional = std::optional< T >;
}
#include <algorithm>
using column_count_type = uint32_t;
using column_id = column_count_type;
using schema_ptr = int;
template < typename T > class optimized_optional {
  optimized_optional(compat::optional< T > ) ;
};

class cql_serialization_format {
public:
  ;
};
namespace utils {
template < typename , size_t > class small_vector {};
} template < typename... > class future;
bool need_preempt ;
namespace internal {
template < typename , bool >
struct uninitialized_wrapper_base;
template < typename T > struct uninitialized_wrapper_base< T, false > {};
template < typename T > struct uninitialized_wrapper_base< T, true > {};
template < typename T >
constexpr bool can_inherit = std::is_same< std::tuple<>, T >::value ||
                             std::is_trivially_destructible< T >::value &&
                              std::is_trivially_constructible< T >::value &&
                              std::is_final< T >::value;
template < typename T >
struct uninitialized_wrapper
    : uninitialized_wrapper_base< T, can_inherit< T > > {};
} struct future_state_base {};
template < typename... T >
struct future_state
    : future_state_base,
      internal::uninitialized_wrapper< std::tuple< T... > > {
  static_assert(std::is_nothrow_move_constructible< std::tuple< T... > >::value);
};
template < typename T > struct futurize {
  using type = future< T >;
  template < typename Arg > static type make_exception_future(Arg );
};
template < typename... Args > struct futurize< future< Args... > > {
  using type = future< Args... >;
  template < typename Arg > static type make_exception_future(Arg );
};
template < typename T > using futurize_t = typename futurize< T >::type;
namespace internal {
  class future_base {};
}
template < typename... T >
class future {
  future_state< T... > _state;
  future_state<> get_available_state_ref() ;

public:
  bool failed ;
  template < typename Func,
             typename Result = futurize_t< std::result_of_t< Func(T ...) > > >
  Result then(Func func) {
    return then_impl(func);
  }
  template < typename Func,
             typename Result = futurize_t< std::result_of_t< Func(T ...) > > >
  Result then_impl(Func ) {
    using futurator = futurize< std::result_of_t< Func(T ...) > >;
    return futurator::make_exception_future(
            (get_available_state_ref()));
  }
};
namespace internal {
template < typename > struct continuation_base_from_future;
template < typename... T >
struct continuation_base_from_future< future< T... > > {
  ;
};
template < typename HeldState, typename Future >
class do_with_state {
  HeldState _held;

public:
  do_with_state(HeldState held) : _held(move(held)) {}
  ;
  Future get_future();
};
} template < typename Tuple, size_t... Idx >
auto cherry_pick_tuple(std::index_sequence< Idx... >, Tuple tuple) {
  return make_tuple(std::get< Idx >((tuple))...);
}
template < typename T1, typename T2, typename T3_or_F, typename... More >
auto do_with(T1 rv1, T2 rv2, T3_or_F rv3, More ... more) {
  auto all = forward_as_tuple(
      (rv1), (rv2),
      (rv3), (more)...);
  constexpr size_t nr = std::tuple_size< decltype(all) >::value - 1;
  using idx = std::make_index_sequence< nr >;
  auto just_values = cherry_pick_tuple(idx(), move(all));
  auto just_func = (std::get< nr >(move(all)));
  using value_tuple = std::remove_reference_t< decltype(just_values) >;
  using ret_type = decltype(apply(just_func, just_values));
  auto task =
      std::make_unique< internal::do_with_state< value_tuple, ret_type > >(
          move(just_values));
  auto ret = task->get_future();
  return ret;
}
class lowres_clock_impl {
public:
  using base_steady_clock = std::chrono::steady_clock;
  using period = std::ratio< 0 >;
  ;
  using steady_duration = std::chrono::duration< period >;
  using steady_time_point =
      std::chrono::time_point< steady_duration >;
};
class lowres_clock {
public:
  using time_point = lowres_clock_impl;
};
class table;
using column_family = table;
class trace_state_ptr;
namespace query {
using column_id_vector = utils::small_vector< column_id, 8 >;
using clustering_range = int;
typedef std::vector< clustering_range > clustering_row_ranges;
class specific_ranges {};
auto max_rows = std::numeric_limits< uint32_t >::max();
class partition_slice {
public:
  ;
  using option_set = int;
  partition_slice(
      clustering_row_ranges , column_id_vector ,
      column_id_vector , option_set ,
      std::unique_ptr< specific_ranges > ,
      cql_serialization_format ,
      uint32_t = max_rows);
};
} namespace db {
using timeout_clock = lowres_clock;
}
class mutation {
  mutation() = default;

public:
  int decorated_key() ;
};
using mutation_opt = optimized_optional< mutation >;
future< mutation_opt >
read_mutation_from_flat_mutation_reader(int ,
                                        db::timeout_clock::time_point );
class locked_cell;
class frozen_mutation;
class table {
public:
  future< std::vector< locked_cell > >
  lock_counter_cells(mutation , db::timeout_clock::time_point );
};
class database {
  future< mutation > do_apply_counter_update(
      column_family &, const frozen_mutation &, schema_ptr ,
      db::timeout_clock::time_point , trace_state_ptr );
};
class trace_state_ptr {
public:
  trace_state_ptr(nullptr_t);
};
template < typename Consumer >
void consume_partitions(db::timeout_clock::time_point timeout) {
  int reader;
  ([reader, timeout](Consumer c) {
    return ({
      return read_mutation_from_flat_mutation_reader(reader, timeout).then;
    });
  });
}
class frozen_mutation {
public:
  mutation unfreeze() const;
};
class mutation_source {};
future< int > counter_write_query(schema_ptr, const mutation_source ,
                                  const int ,
                                  const query::partition_slice ,
                                  trace_state_ptr );
class locked_cell {};
future< mutation > database::do_apply_counter_update(
    column_family &cf, const frozen_mutation &fm, schema_ptr ,
    db::timeout_clock::time_point timeout, trace_state_ptr ) {
  auto m = fm.unfreeze();
  query::column_id_vector static_columns;
  query::clustering_row_ranges cr_ranges;
  query::column_id_vector regular_columns;
  auto slice = query::partition_slice(
      move(cr_ranges), (static_columns),
      (regular_columns), {}, {}, cql_serialization_format());
  return do_with(
      (slice), (m), std::vector< locked_cell >(),
      [this, cf, timeout](query::partition_slice slice, mutation m,
                           std::vector< locked_cell > ) mutable {
        return cf.lock_counter_cells(m, timeout)
            .then([&](std::vector< locked_cell > ) {
              return counter_write_query(schema_ptr(), mutation_source(),
                                         m.decorated_key(), slice, nullptr)
                  .then([m](auto ) {
                    return (m);
                  });
            });
      });
}
