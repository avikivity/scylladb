#include <atomic>
#include <filesystem>
#include <optional>
namespace compat {
template < typename T > using optional = std::optional< T >;
}
#include <algorithm>
#define GCC6_CONCEPT()
using column_count_type = uint32_t;
using column_id = column_count_type;
using schema_ptr = int;
GCC6_CONCEPT() template < typename T > class optimized_optional {
  optimized_optional(compat::optional< T > &&obj) noexcept;
};
using cql_protocol_version_type = uint8_t;
class cql_serialization_format {
public:
  static constexpr cql_protocol_version_type latest_version = 4;
  static cql_serialization_format internal();
};
namespace utils {
template < typename T, size_t N > class small_vector {};
} // namespace utils
template < typename... T > class future;
class task {
  virtual void run_and_dispose() noexcept = 0;
};
bool need_preempt() noexcept;
#define SEASTAR_NODISCARD
namespace internal {
template < typename T, bool is_trivial_class >
struct uninitialized_wrapper_base;
template < typename T > struct uninitialized_wrapper_base< T, false > {};
template < typename T > struct uninitialized_wrapper_base< T, true > {};
template < typename T >
constexpr bool can_inherit = std::is_same< std::tuple<>, T >::value ||
                             (std::is_trivially_destructible< T >::value &&
                              std::is_trivially_constructible< T >::value &&
                              !std::is_final< T >::value);
template < typename T >
struct uninitialized_wrapper
    : public uninitialized_wrapper_base< T, can_inherit< T > > {};
} // namespace internal
struct future_state_base {};
template < typename... T >
struct future_state
    : public future_state_base,
      private internal::uninitialized_wrapper< std::tuple< T... > > {
  static_assert(std::is_nothrow_move_constructible< std::tuple< T... > >::value,
                "Types must be no-throw destructible");
};
template < typename T > struct futurize {
  using type = future< T >;
  template < typename Arg > static type make_exception_future(Arg &&arg);
};
template < typename... Args > struct futurize< future< Args... > > {
  using type = future< Args... >;
  template < typename Arg > static type make_exception_future(Arg &&arg);
};
template < typename T > using futurize_t = typename futurize< T >::type;
GCC6_CONCEPT() namespace internal {
  class future_base {};
}
template < typename... T >
class SEASTAR_NODISCARD future : private internal::future_base {
  future_state< T... > _state;
  future_state<> &&get_available_state_ref() noexcept;

public:
  bool available() const noexcept;
  bool failed() const noexcept;
  template < typename Func,
             typename Result = futurize_t< std::result_of_t< Func(T &&...) > > >
  Result then(Func &&func) noexcept {
    return then_impl(std::move(func));
  }
  template < typename Func,
             typename Result = futurize_t< std::result_of_t< Func(T &&...) > > >
  Result then_impl(Func &&func) noexcept {
    using futurator = futurize< std::result_of_t< Func(T && ...) > >;
    if (available() && !need_preempt())
      if (failed())
        return futurator::make_exception_future(
            static_cast< future_state_base && >(get_available_state_ref()));
  }
};
namespace internal {
template < typename Future > struct continuation_base_from_future;
template < typename... T >
struct continuation_base_from_future< future< T... > > {
  using type = task;
};
template < typename HeldState, typename Future >
class do_with_state final
    : public continuation_base_from_future< Future >::type {
  HeldState _held;

public:
  explicit do_with_state(HeldState &&held) : _held(std::move(held)) {}
  virtual void run_and_dispose() noexcept override;
  Future get_future();
};
} // namespace internal
template < typename Tuple, size_t... Idx >
inline auto cherry_pick_tuple(std::index_sequence< Idx... >, Tuple &&tuple) {
  return std::make_tuple(std::get< Idx >(std::forward< Tuple >(tuple))...);
}
template < typename T1, typename T2, typename T3_or_F, typename... More >
inline auto do_with(T1 &&rv1, T2 &&rv2, T3_or_F &&rv3, More &&... more) {
  auto all = std::forward_as_tuple(
      std::forward< T1 >(rv1), std::forward< T2 >(rv2),
      std::forward< T3_or_F >(rv3), std::forward< More >(more)...);
  constexpr size_t nr = std::tuple_size< decltype(all) >::value - 1;
  using idx = std::make_index_sequence< nr >;
  auto &&just_values = cherry_pick_tuple(idx(), std::move(all));
  auto &&just_func = std::move(std::get< nr >(std::move(all)));
  using value_tuple = std::remove_reference_t< decltype(just_values) >;
  using ret_type = decltype(apply(just_func, just_values));
  auto task =
      std::make_unique< internal::do_with_state< value_tuple, ret_type > >(
          std::move(just_values));
  auto ret = task->get_future();
  return ret;
}
class lowres_clock;
class lowres_clock_impl final {
public:
  using base_steady_clock = std::chrono::steady_clock;
  using period = std::ratio< 1, 1000 >;
  using steady_rep = base_steady_clock::rep;
  using steady_duration = std::chrono::duration< steady_rep, period >;
  using steady_time_point =
      std::chrono::time_point< lowres_clock, steady_duration >;
};
class lowres_clock final {
public:
  using time_point = lowres_clock_impl::steady_time_point;
};
class table;
using column_family = table;
class trace_state_ptr;
namespace query {
using column_id_vector = utils::small_vector< column_id, 8 >;
using clustering_range = int;
typedef std::vector< clustering_range > clustering_row_ranges;
class specific_ranges {};
constexpr auto max_rows = std::numeric_limits< uint32_t >::max();
class partition_slice {
public:
  enum class option {
    send_clustering_key,
    send_partition_key,
    always_return_static_content,
  };
  using option_set = int;
  partition_slice(
      clustering_row_ranges row_ranges, column_id_vector static_columns,
      column_id_vector regular_columns, option_set options,
      std::unique_ptr< specific_ranges > specific_ranges = nullptr,
      cql_serialization_format = cql_serialization_format::internal(),
      uint32_t partition_row_limit = max_rows);
};
} // namespace query
namespace db {
using timeout_clock = lowres_clock;
}
GCC6_CONCEPT() class mutation final {
  mutation() = default;

public:
  const int &decorated_key() const;
};
using mutation_opt = optimized_optional< mutation >;
future< mutation_opt >
read_mutation_from_flat_mutation_reader(int &reader,
                                        db::timeout_clock::time_point timeout);
class locked_cell;
class frozen_mutation;
class table {
public:
  future< std::vector< locked_cell > >
  lock_counter_cells(const mutation &m, db::timeout_clock::time_point timeout);
};
class database {
  future< mutation > do_apply_counter_update(
      column_family &cf, const frozen_mutation &fm, schema_ptr m_schema,
      db::timeout_clock::time_point timeout, trace_state_ptr trace_state);
};
class trace_state_ptr final {
public:
  trace_state_ptr(nullptr_t);
};
template < typename Consumer >
void consume_partitions(db::timeout_clock::time_point timeout) {
  int &reader;
  return do_with(std::move, [&reader, timeout](Consumer &c) -> future<> {
    return repeat([&reader, &c, timeout] {
      return read_mutation_from_flat_mutation_reader(reader, timeout).then;
    });
  });
}
class frozen_mutation final {
public:
  mutation unfreeze(void) const;
};
class mutation_source {};
future< int > counter_write_query(schema_ptr, const mutation_source &,
                                  const int &dk,
                                  const query::partition_slice &slice,
                                  trace_state_ptr trace_ptr);
class locked_cell {};
future< mutation > database::do_apply_counter_update(
    column_family &cf, const frozen_mutation &fm, schema_ptr m_schema,
    db::timeout_clock::time_point timeout, trace_state_ptr trace_state) {
  auto m = fm.unfreeze();
  query::column_id_vector static_columns;
  query::clustering_row_ranges cr_ranges;
  query::column_id_vector regular_columns;
  auto slice = query::partition_slice(
      std::move(cr_ranges), std::move(static_columns),
      std::move(regular_columns), {}, {}, cql_serialization_format::internal(),
      query::max_rows);
  return do_with(
      std::move(slice), std::move(m), std::vector< locked_cell >(),
      [this, &cf, timeout](const query::partition_slice &slice, mutation &m,
                           std::vector< locked_cell > &locks) mutable {
        return cf.lock_counter_cells(m, timeout)
            .then([&, timeout, this](std::vector< locked_cell > lcs) mutable {
              return counter_write_query(schema_ptr(), mutation_source(),
                                         m.decorated_key(), slice, nullptr)
                  .then([this, &cf, &m, timeout](auto mopt) {
                    return std::move(m);
                  });
            });
      });
}
