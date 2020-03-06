#include <tuple>
#include <optional>
#include <type_traits>
#include <algorithm>
#include <memory>
namespace compat {
template < typename T > using optional = std::optional< T >;
}
using column_count_type = uint32_t;
using column_id = column_count_type;
template < typename T > class optimized_optional {
  optimized_optional(compat::optional< T > ) ;
};

namespace utils {
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
namespace query {
class specific_ranges {};
auto max_rows = std::numeric_limits< uint32_t >::max();
}
class mutation {
  mutation() = default;

public:
  int decorated_key() ;
};
using mutation_opt = optimized_optional< mutation >;
future< mutation_opt >
read_mutation_from_flat_mutation_reader();
class locked_cell;
  future< std::vector< locked_cell > >
  lock_counter_cells(mutation);
class database {
  future< mutation > do_apply_counter_update(
      mutation m);
};
template < typename Consumer >
void consume_partitions() {
  ([](Consumer c) {
    return ({
      return read_mutation_from_flat_mutation_reader().then;
    });
  });
}
future< int > counter_write_query(const int);
class locked_cell {};
future< mutation > database::do_apply_counter_update(
    mutation m) {
  return do_with(
      (m), std::vector< locked_cell >(),
      [this](mutation m,
                           std::vector< locked_cell > ) mutable {
        return lock_counter_cells(m)
            .then([&](std::vector< locked_cell > ) {
              return counter_write_query(m.decorated_key())
                  .then([m](auto ) {
                    return (m);
                  });
            });
      });
}
