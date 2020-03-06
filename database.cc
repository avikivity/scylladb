#include <atomic>
#include <cstring>
#include <filesystem>
#include <optional>
namespace seastar {
   namespace compat {
  template <typename T> using optional = std::optional<T>;
  }
   }
#include <algorithm>
namespace seastar {
   }
    using namespace seastar;
#define GCC6_CONCEPT(x...)
    namespace seastar {
       template <typename T> class lw_shared_ptr {
 };
       }
       using column_count_type = uint32_t;
       using column_id = column_count_type;
       class schema;
       using schema_ptr = seastar::lw_shared_ptr<const schema>;
       namespace seastar {
      GCC6_CONCEPT( template<typename T> concept bool OptimizableOptional() {
     ) template<typename T> class optimized_optional {
      T _object;
      optimized_optional(compat::optional<T> &&obj) noexcept {        if (obj) {        }      }
      explicit operator bool() const noexcept;
      T &operator*() noexcept { return _object; }
    };
     }
       using cql_protocol_version_type = uint8_t;
       class cql_serialization_format {
      cql_protocol_version_type _version;
    public:     static constexpr cql_protocol_version_type latest_version = 4;
      explicit cql_serialization_format(cql_protocol_version_type version)         : _version(version) {
}
      static cql_serialization_format latest() {
       return cql_serialization_format{latest_version};
     }
      static cql_serialization_format internal() {
 return latest();
 }
     };
       namespace utils {
      template <typename T, size_t N> class small_vector {
       T *_begin;
       T *_end;
       T *_capacity_end;
       union internal {         T storage[N];       };
       internal _internal;
       bool uses_internal_storage() const noexcept {         return _begin == _internal.storage;       }
       [[gnu::cold]] [[gnu::noinline]] void expand(size_t new_capacity) {         auto ptr = static_cast<T *>(             ::aligned_alloc(alignof(T), new_capacity * sizeof(T)));         if (!ptr) {         }         if (!uses_internal_storage()) {         }       }
       [[gnu::cold]] [[gnu::noinline]] void       slow_copy_assignment(const small_vector &other) {         auto ptr = static_cast<T *>(             ::aligned_alloc(alignof(T), other.size() * sizeof(T)));         if (!ptr) {         }         try {         } catch (...) {         }       }
     };
     }
    namespace seastar {
      template <typename Func, typename Args, typename IndexList>     struct apply_helper;
      template <typename Func, typename Tuple, size_t... I>     struct apply_helper<Func, Tuple, std::index_sequence<I...>> {
       static auto apply(Func &&func, Tuple args) {         return func(std::get<I>(std::forward<Tuple>(args))...);       }
     };
      template <typename Func, typename... T>     inline auto apply(Func && func, const std::tuple<T...> &args) {
       using helper = apply_helper<Func, const std::tuple<T...> &,                                   std::index_sequence_for<T...>>;
       return helper::apply(std::forward<Func>(func), args);
     }
      template <typename T> struct function_traits;
      template <typename... T> class future;
      class task {
       virtual void run_and_dispose() noexcept = 0;
     };
      bool need_preempt() noexcept;
     }
   namespace seastar {
      class thread_context;
      namespace thread_impl {
     thread_context *get();
     }
     }
#include <assert.h>
namespace seastar {
      namespace internal {
     template <typename T> struct used_size {       static constexpr size_t value = std::is_empty<T>::value ? 0 : sizeof(T);     };
     }
     }
#define SEASTAR_NODISCARD [[nodiscard]]
namespace seastar {
      namespace internal {
     template <typename... T>     future<T...> current_exception_as_future() noexcept;
     template <typename T, bool is_trivial_class>     struct uninitialized_wrapper_base;
     template <typename T> struct uninitialized_wrapper_base<T, false> {       union any {       } _v;       T &uninitialized_get();     };
     template <typename T>     struct uninitialized_wrapper_base<T, true> : private T {     };
     template <typename T>     constexpr bool can_inherit = std::is_same<std::tuple<>, T>::value ||                                  (std::is_trivially_destructible<T>::value &&                                   std::is_trivially_constructible<T>::value &&                                   !std::is_final<T>::value);
     template <typename T>     struct uninitialized_wrapper         : public uninitialized_wrapper_base<T, can_inherit<T>> {};
     static_assert(std::is_empty<uninitialized_wrapper<std::tuple<>>>::value,                   "This should still be empty");
     template <typename T>     struct is_trivially_move_constructible_and_destructible {       static constexpr bool value =           std::is_trivially_destructible<T>::value;     };
     template <bool... v> struct all_true : std::false_type {};
     }
      struct future_state_base {
       enum class state : uintptr_t {         result = 3,         exception_min = 4,       };
       union any {         state st;       }
 _u;
       future_state_base() noexcept;
     public:       bool available() const noexcept {         return _u.st == state::result || _u.st >= state::exception_min;       }
       bool failed() const noexcept;
};
      struct future_for_get_promise_marker {
};
      template <typename... T>     struct future_state         : public future_state_base,           private internal::uninitialized_wrapper<std::tuple<T...>> {
       static constexpr bool has_trivial_move_and_destroy = internal::all_true<           internal::is_trivially_move_constructible_and_destructible<               T>::value...>::value;
       static_assert(std::is_nothrow_move_constructible<std::tuple<T...>>::value,                     "Types must be no-throw destructible");
       std::tuple<T...> &&get_value() && noexcept {         assert(_u.st == state::result);       }
       std::tuple<T...> &&take_value() && noexcept {       }
     };
      template <typename... T> class continuation_base : public task {
     protected:       future_state<T...> _state;
       continuation_base() = default;
       explicit continuation_base(future_state<T...> &&state)           : _state(std::move(state)) {}
     };
      namespace internal {
     template <typename... T>     future<T...> make_exception_future(future_state_base &&state) noexcept;
     template <typename... T, typename U>     void set_callback(future<T...> &fut, U *callback) noexcept;
     class promise_base {       future_state_base *_state;       template <typename Exception>       std::enable_if_t<!std::is_same<std::remove_reference_t<Exception>,                                      std::exception_ptr>::value,                        void>       set_exception(Exception &&e) noexcept {       }     };
     template <typename... T>     class promise_base_with_type : protected internal::promise_base {       promise_base_with_type(future<T...> *future);       void set_urgent_state(future_state<T...> &&state) noexcept {         if (_state) {         }       }       template <typename Func> void schedule(Func &&func) noexcept {       }       template <typename... U> friend class seastar::future;     };
     }
      template <typename... T>     class promise : private internal::promise_base_with_type<T...> {
     public:       promise();
       future<T...> get_future() noexcept;
       template <typename Exception>       std::enable_if_t<!std::is_same<std::remove_reference_t<Exception>,                                      std::exception_ptr>::value,                        void>       set_exception(Exception &&e) noexcept {       }
};
      template <typename T> struct futurize {
       using type = future<T>;
       template <typename Func, typename... FuncArgs>       static inline type apply(Func &&func,                                std::tuple<FuncArgs...> &&args) noexcept;
       static type convert(T &&value);
       template <typename Arg> static type make_exception_future(Arg &&arg);
     };
      template <typename... Args> struct futurize<future<Args...>> {
       using type = future<Args...>;
       template <typename Func, typename... FuncArgs>       static inline type apply(Func &&func,                                std::tuple<FuncArgs...> &&args) noexcept;
       template <typename Func, typename... FuncArgs>       static inline type apply(Func &&func, FuncArgs &&... args) noexcept;
       template <typename Arg> static type make_exception_future(Arg &&arg);
     };
      template <typename T> using futurize_t = typename futurize<T>::type;
      GCC6_CONCEPT(template <typename T> concept bool Future =                      is_future<T>::value;
 )     namespace internal {
       class future_base {       protected:         promise_base *_promise;         future_base() noexcept : _promise(nullptr) {}         future_base(future_base &&x, future_state_base *state);         promise_base *detach_promise() noexcept;       };
       template <bool IsVariadic> struct warn_variadic_future {       };
       template <> struct warn_variadic_future<true> {         [[deprecated(             "deprecated, replace with future<std::tuple<...>>")]] void         check_deprecation() {}       };
     }
      template <typename... T>     class SEASTAR_NODISCARD future         : private internal::future_base,           internal::warn_variadic_future<(sizeof...(T) > 1)> {
       future_state<T...> _state;
       future(future_for_get_promise_marker m) {}
       [[gnu::always_inline]] explicit future(           future_state<T...> &&state) noexcept           : _state(std::move(state)) {       }
       internal::promise_base_with_type<T...> get_promise() noexcept {         return internal::promise_base_with_type<T...>(this);       }
       internal::promise_base_with_type<T...> *detach_promise() {         return static_cast<internal::promise_base_with_type<T...> *>(             future_base::detach_promise());       }
       template <typename Func> void schedule(Func &&func) noexcept {         if (_state.available() || !_promise) {           if (__builtin_expect(!_state.available() && !_promise, false)) {           }         }       }
       [[gnu::always_inline]] future_state<T...> &&       get_available_state_ref() noexcept {         if (_promise) {         }         return std::move(_state);       }
       [[gnu::noinline]] future<T...> rethrow_with_nested() {         if (!failed()) {           try {           } catch (...) {           }         }       }
     public:       using promise_type = promise<T...>;
     public:       [[gnu::always_inline]] bool available() const noexcept {         return _state.available();       }
       [[gnu::always_inline]] bool failed() const noexcept {         return _state.failed();       }
       template <typename Func,                 typename Result = futurize_t<std::result_of_t<Func(T &&...)>>>       Result then(Func &&func) noexcept {         return then_impl(std::move(func));       }
       template <typename Func,                 typename Result = futurize_t<std::result_of_t<Func(T &&...)>>>       Result then_impl(Func &&func) noexcept {         using futurator = futurize<std::result_of_t<Func(T && ...)>>;         if (available() && !need_preempt()) {           if (failed()) {             return futurator::make_exception_future(                 static_cast<future_state_base &&>(get_available_state_ref()));           }         }         typename futurator::type fut(future_for_get_promise_marker{});         [&]() noexcept {           schedule([pr = fut.get_promise(), func = std::forward<Func>(func)](                        future_state<T...> &&state) mutable {             if (state.failed()) {               futurator::apply(std::forward<Func>(func),                                std::move(state).get_value())                   .forward_to(std::move(pr));             }           });         }();         return fut;       }
       template <bool AsSelf, typename FuncResult, typename Func>       futurize_t<FuncResult> then_wrapped_common(Func &&func) noexcept {         using futurator = futurize<FuncResult>;         if (available() && !need_preempt()) {           if (AsSelf) {             if (_promise) {             }             return futurator::apply(std::forward<Func>(func),                                     future(get_available_state_ref()));           }         }         typename futurator::type fut(future_for_get_promise_marker{});         [&]() noexcept {           schedule([pr = fut.get_promise(), func = std::forward<Func>(func)](                        future_state<T...> &&state) mutable {           });         }();       }
       void forward_to(internal::promise_base_with_type<T...> &&pr) noexcept {       }
       template <typename... U> friend class future;
       template <typename... U, typename... A>       friend future<U...>       internal::make_exception_future(future_state_base &&state) noexcept;
     };
      template <typename T>     template <typename Func, typename... FuncArgs>     typename futurize<T>::type futurize<T>::apply(         Func && func, std::tuple<FuncArgs...> && args) noexcept {
       try {         return convert(             ::seastar::apply(std::forward<Func>(func), std::move(args)));       }
 catch (...) {       }
     }
      template <typename... Args>     template <typename Func, typename... FuncArgs>     typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(         Func && func, std::tuple<FuncArgs...> && args) noexcept {
       try {         return ::seastar::apply(std::forward<Func>(func), std::move(args));       }
 catch (...) {       }
     }
      template <typename Tag> class bool_class {
     };
      namespace internal {
     template <typename Future> struct continuation_base_from_future;
     template <typename... T>     struct continuation_base_from_future<future<T...>> {       using type = continuation_base<T...>;     };
     template <typename HeldState, typename Future>     class do_with_state final         : public continuation_base_from_future<Future>::type {       HeldState _held;       typename Future::promise_type _pr;     public:       explicit do_with_state(HeldState &&held) : _held(std::move(held)) {}       virtual void run_and_dispose() noexcept override {       }       HeldState &data() { return _held; }       Future get_future() { return _pr.get_future(); }     };
     }
      template <typename Tuple, size_t... Idx>     inline auto cherry_pick_tuple(std::index_sequence<Idx...>, Tuple && tuple) {
       return std::make_tuple(std::get<Idx>(std::forward<Tuple>(tuple))...);
     }
      template <typename T1, typename T2, typename T3_or_F, typename... More>     inline auto do_with(T1 && rv1, T2 && rv2, T3_or_F && rv3,                         More && ... more) {
       auto all = std::forward_as_tuple(           std::forward<T1>(rv1), std::forward<T2>(rv2),           std::forward<T3_or_F>(rv3), std::forward<More>(more)...);
       constexpr size_t nr = std::tuple_size<decltype(all)>::value - 1;
       using idx = std::make_index_sequence<nr>;
       auto &&just_values = cherry_pick_tuple(idx(), std::move(all));
       auto &&just_func = std::move(std::get<nr>(std::move(all)));
       using value_tuple = std::remove_reference_t<decltype(just_values)>;
       using ret_type = decltype(apply(just_func, just_values));
       auto task =           std::make_unique<internal::do_with_state<value_tuple, ret_type>>(               std::move(just_values));
       auto ret = task->get_future();
       return ret;
     }
      struct stop_iteration_tag {
};
      using stop_iteration = bool_class<stop_iteration_tag>;
      class lowres_clock;
      class lowres_clock_impl final {
     public:       using base_steady_clock = std::chrono::steady_clock;
       using period = std::ratio<1, 1000>;
       using steady_rep = base_steady_clock::rep;
       using steady_duration = std::chrono::duration<steady_rep, period>;
       using steady_time_point =           std::chrono::time_point<lowres_clock, steady_duration>;
     };
      class lowres_clock final {
     public:       using time_point = lowres_clock_impl::steady_time_point;
     };
  }
       struct blob_storage {
   }
       __attribute__((packed));
       class table;
       using column_family = table;
       class clustering_key_prefix;
       template <typename T> class nonwrapping_range {
  };
       GCC6_CONCEPT(template <template <typename> typename T, typename U>              concept bool Range =                  std::is_same<T<U>, wrapping_range<U>>::value ||                  std::is_same<T<U>, nonwrapping_range<U>>::value;
      ) namespace std {
  }
       namespace dht {
      class decorated_key;
   }
       template <typename EnumType, EnumType... Items> struct super_enum {
  };
       template <typename Enum> class enum_set {
  };
       namespace tracing {
      class trace_state_ptr;
   }
       namespace query {
      using column_id_vector = utils::small_vector<column_id, 8>;
      using clustering_range = nonwrapping_range<clustering_key_prefix>;
      typedef std::vector<clustering_range> clustering_row_ranges;
      class specific_ranges {
};
      constexpr auto max_rows = std::numeric_limits<uint32_t>::max();
      class partition_slice {
     public:       enum class option {         send_clustering_key,         send_partition_key,         always_return_static_content,       };
       using option_set = enum_set<super_enum<           option, option::send_clustering_key, option::send_partition_key,           option::always_return_static_content>>;
       partition_slice(           clustering_row_ranges row_ranges, column_id_vector static_columns,           column_id_vector regular_columns, option_set options,           std::unique_ptr<specific_ranges> specific_ranges = nullptr,           cql_serialization_format = cql_serialization_format::internal(),           uint32_t partition_row_limit = max_rows);
     };
   }
       namespace db {
      using timeout_clock = seastar::lowres_clock;
   }
       GCC6_CONCEPT(template <typename T, typename ReturnType>              concept bool MutationFragmentConsumer() {
   ) class mutation final {
       mutation() = default;
     public:       const dht::decorated_key &decorated_key() const;
     };
      using mutation_opt = optimized_optional<mutation>;
      class flat_mutation_reader;
      future<mutation_opt> read_mutation_from_flat_mutation_reader(         flat_mutation_reader & reader, db::timeout_clock::time_point timeout);
      class locked_cell;
      class frozen_mutation;
      class table {
     public:       future<std::vector<locked_cell>>       lock_counter_cells(const mutation &m,                          db::timeout_clock::time_point timeout);
     };
      class database {
       future<mutation>       do_apply_counter_update(column_family &cf, const frozen_mutation &fm,                               schema_ptr m_schema,                               db::timeout_clock::time_point timeout,                               tracing::trace_state_ptr trace_state);
     };
      namespace tracing {
     class trace_state_ptr final {     public:       trace_state_ptr(nullptr_t);     };
     }
      template <typename Consumer>     inline future<> consume_partitions(flat_mutation_reader & reader,                                        Consumer consumer,                                        db::timeout_clock::time_point timeout) {
       using futurator = futurize<std::result_of_t<Consumer(mutation &&)>>;
       return do_with(           std::move(consumer), [&reader, timeout](Consumer &c) -> future<> {             return repeat([&reader, &c, timeout]() {               return read_mutation_from_flat_mutation_reader(reader, timeout)                   .then([&c](mutation_opt &&mo) -> future<stop_iteration> {                     if (!mo) {                     }                     return futurator::apply(c, std::move(*mo));                   });             });           }
);
     }
      class frozen_mutation final {
     public:       mutation unfreeze(schema_ptr s) const;
     };
      class mutation_source {
};
      future<mutation_opt> counter_write_query(         schema_ptr, const mutation_source &, const dht::decorated_key &dk,         const query::partition_slice &slice,         tracing::trace_state_ptr trace_ptr);
      class locked_cell {
};
      future<mutation> database::do_apply_counter_update(         column_family & cf, const frozen_mutation &fm, schema_ptr m_schema,         db::timeout_clock::time_point timeout,         tracing::trace_state_ptr trace_state) {
       auto m = fm.unfreeze(m_schema);
       query::column_id_vector static_columns;
       query::clustering_row_ranges cr_ranges;
       query::column_id_vector regular_columns;
       auto slice = query::partition_slice(           std::move(cr_ranges), std::move(static_columns),           std::move(regular_columns), {}
, {}
,           cql_serialization_format::internal(), query::max_rows);
       return do_with(           std::move(slice), std::move(m), std::vector<locked_cell>(),           [this, &cf, timeout](const query::partition_slice &slice, mutation &m,                                std::vector<locked_cell> &locks) mutable {             return cf.lock_counter_cells(m, timeout)                 .then([&, timeout, this](std::vector<locked_cell> lcs) mutable {                   return counter_write_query(schema_ptr(), mutation_source(),                                              m.decorated_key(), slice, nullptr)                       .then([this, &cf, &m, timeout](auto mopt) {                         return std::move(m);                       });                 });           }
);
     }
