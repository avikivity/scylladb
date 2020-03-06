#include <atomic>
#include <cstring>
#include <filesystem>
#include <optional>
namespace seastar {}
namespace seastar {
namespace compat {
template <typename T> using optional = std::optional<T>;
}
} 
#include <algorithm>
namespace seastar {
template <typename char_type, typename Size, Size max_size,
          bool NulTerminate = true>
class basic_sstring;
using sstring = basic_sstring<char, uint32_t, 15>;
template <typename char_type, typename Size, Size max_size, bool NulTerminate>
class basic_sstring {};
} 
using namespace seastar;
#define GCC6_CONCEPT(x...)
    namespace seastar {
    struct lw_shared_ptr_counter_base {};
    template <typename T> struct lw_shared_ptr_deleter;
    namespace internal {
    template <typename... T> using void_t = void;
    }
    template <typename T> class lw_shared_ptr {};
    template <typename T> class shared_ptr {};
    }
    using column_count_type = uint32_t;
    using column_id = column_count_type;
    class schema;
    using schema_ptr = seastar::lw_shared_ptr<const schema>;
    namespace seastar {
   GCC6_CONCEPT( template<typename T> concept bool OptimizableOptional() {
   ) template<typename T> class optimized_optional {
     T _object;
     optimized_optional(compat::optional<T> &&obj) noexcept {
       if (obj) {
         _object = std::move(*obj);
       }
     }
     explicit operator bool() const noexcept;
     T &operator*() noexcept { return _object; }
   };
   }
    using cql_protocol_version_type = uint8_t;
    class cql_serialization_format {
    cql_protocol_version_type _version;
  public:
    static constexpr cql_protocol_version_type latest_version = 4;
    explicit cql_serialization_format(cql_protocol_version_type version)
        : _version(version) {}
    static cql_serialization_format latest() {
      return cql_serialization_format{latest_version};
    }
    static cql_serialization_format internal() { return latest(); }
   };
    namespace utils {
    template <typename T, size_t N> class small_vector {
    private:
      T *_begin;
      T *_end;
      T *_capacity_end;
      union internal {
        internal();
        ~internal();
        T storage[N];
      };
      internal _internal;
    private:
      bool uses_internal_storage() const noexcept {
        return _begin == _internal.storage;
      }
      [[gnu::cold]] [[gnu::noinline]] void expand(size_t new_capacity) {
        auto ptr = static_cast<T *>(
            ::aligned_alloc(alignof(T), new_capacity * sizeof(T)));
        if (!ptr) {
          throw std::bad_alloc();
        }
        if (!uses_internal_storage()) {
          std::free(_begin);
        }
        _begin = ptr;
      }
      [[gnu::cold]] [[gnu::noinline]] void
      slow_copy_assignment(const small_vector &other) {
        auto ptr = static_cast<T *>(
            ::aligned_alloc(alignof(T), other.size() * sizeof(T)));
        if (!ptr) {
          throw std::bad_alloc();
        }
        auto n_end = ptr;
        try {
          n_end = std::uninitialized_copy(other.begin(), other.end(), n_end);
        } catch (...) {
        }
        std::destroy(begin(), end());
        _capacity_end = n_end;
      }
      void reserve_at_least(size_t n) {
        if (__builtin_expect(_begin + n > _capacity_end, false)) {
          expand(std::max(n, capacity() * 2));
        }
      }
      [[noreturn]] [[gnu::cold]] [[gnu::noinline]] void throw_out_of_range() {
        throw std::out_of_range("out of range small vector access");
      }
    public:
      using value_type = T;
      using iterator = T *;
      using const_iterator = const T *;
      using reverse_iterator = std::reverse_iterator<iterator>;
      using const_reverse_iterator = std::reverse_iterator<const_iterator>;
      small_vector() noexcept
          : _begin(_internal.storage), _end(_begin), _capacity_end(_begin + N) {
      }
      template <typename InputIterator>
      small_vector(InputIterator first, InputIterator last) : small_vector() {
        if constexpr (std::is_base_of_v<
                          std::forward_iterator_tag,
                          typename std::iterator_traits<
                              InputIterator>::iterator_category>) {
          reserve(std::distance(first, last));
          std::copy(first, last, std::back_inserter(*this));
        }
      }
      small_vector(std::initializer_list<T> list)
          : small_vector(list.begin(), list.end()) {}
      small_vector(small_vector &&other) noexcept {
        if (other.uses_internal_storage()) {
          _begin = _internal.storage;
          _capacity_end = _begin + N;
          if constexpr (std::is_trivially_copyable_v<T>) {
            _end = _begin;
            for (auto &e : other) {
            }
          }
        }
      }
      small_vector(const small_vector &other) noexcept : small_vector() {
        reserve(other.size());
        _end = std::uninitialized_copy(other.begin(), other.end(), _end);
      }
      small_vector &operator=(small_vector &&other) noexcept {
        clear();
        if (other.uses_internal_storage()) {
          if (__builtin_expect(!uses_internal_storage(), false)) {
            _end = _begin;
            for (auto &e : other) {
            }
          }
        }
        return *this;
        if constexpr (std::is_nothrow_copy_constructible_v<T>) {
          if (capacity() >= other.size()) {
            return *this;
          }
        }
        slow_copy_assignment(other);
        return *this;
      }
      ~small_vector() {
        clear();
        if (__builtin_expect(!uses_internal_storage(), false)) {
          std::free(_begin);
        }
      }
      void clear() noexcept {
        std::destroy(_begin, _end);
        _end = _begin;
      }
      iterator begin() noexcept { return _begin; }
      const_iterator begin() const noexcept { return _begin; }
      const_iterator cbegin() const noexcept { return _begin; }
      iterator end() noexcept { return _end; }
      reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
      const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
      }
      const_reverse_iterator crbegin() const noexcept {
        return const_reverse_iterator(end());
      }
      reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
      const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
        return const_reverse_iterator(begin());
      }
      T *data() noexcept { return _begin; }
      const T *data() const noexcept { return _begin; }
      T &operator[](size_t idx) noexcept { return data()[idx]; }
      const T &operator[](size_t idx) const noexcept { return data()[idx]; }
      T &at(size_t idx) {
        if (__builtin_expect(idx >= size(), false)) {
          throw_out_of_range();
        }
        return operator[](idx);
      }
      const T &at(size_t idx) const {
        if (__builtin_expect(idx >= size(), false)) {
          throw_out_of_range();
        }
        return operator[](idx);
      }
      bool empty() const noexcept { return _begin == _end; }
      size_t size() const noexcept { return _end - _begin; }
      size_t capacity() const noexcept { return _capacity_end - _begin; }
      template <typename... Args> T &emplace_back(Args &&... args) {
        if (__builtin_expect(_end == _capacity_end, false)) {
          expand(std::max<size_t>(capacity() * 2, 1));
        }
      }
      template <typename InputIterator>
      iterator insert(const_iterator cpos, InputIterator first,
                      InputIterator last) {
        if constexpr (std::is_base_of_v<
                          typename std::iterator_traits<
                              InputIterator>::iterator_category>) {
          if (first == last) {
          }
          auto idx = cpos - _begin;
          auto pos = _begin + idx;
          if (__builtin_expect(pos == end(), true)) {
            try {
            } catch (...) {
            }
            try {
              try {
              } catch (...) {
              }
            } catch (...) {
            }
          }
          while (first != last) {
            try {
            } catch (...) {
            }
          }
        }
      }
      template <typename... Args>
      iterator emplace(const_iterator cpos, Args &&... args) {
        auto idx = cpos - _begin;
        auto pos = _begin + idx;
        if (pos != _end) {
        }
      }
      iterator insert(const_iterator cpos, const T &obj) {
      }
      void resize(size_t n) {
        if (n < size()) {
        }
      }
      void resize(size_t n, const T &value) {
        if (n < size()) {
        }
      }
      iterator erase(const_iterator cit) noexcept {
      }
      iterator erase(const_iterator cfirst, const_iterator clast) noexcept {
      }
      bool operator==(const small_vector &other) const noexcept {
        return !(*this == other);
      }
    };
   }
 namespace seastar {
    template <typename Func, typename Args, typename IndexList>
    struct apply_helper;
    template <typename Func, typename Tuple, size_t... I>
    struct apply_helper<Func, Tuple, std::index_sequence<I...>> {
      static auto apply(Func &&func, Tuple args) {
        return func(std::get<I>(std::forward<Tuple>(args))...);
      }
    };
    template <typename Func, typename... T>
    inline auto apply(Func && func, const std::tuple<T...> &args) {
      using helper = apply_helper<Func, const std::tuple<T...> &,
                                  std::index_sequence_for<T...>>;
      return helper::apply(std::forward<Func>(func), args);
    }
    template <typename T> struct function_traits;
    template <typename... T> class future;
    namespace internal {
    }
    class scheduling_group_key {
    };
    class scheduling_group {
    };
    namespace internal {
    }
    class task {
      virtual void run_and_dispose() noexcept = 0;
    };
    namespace internal {
    struct preemption_monitor {
    };
    }
    bool need_preempt() noexcept;
   }
namespace seastar {
    class thread_context;
    struct jmp_buf_link {
    };
    namespace thread_impl {
    thread_context *get();
    }
   }
#include <assert.h>
namespace seastar {
    namespace internal {
    template <typename T> struct used_size {
      static constexpr size_t value = std::is_empty<T>::value ? 0 : sizeof(T);
    };
    }
   }
    namespace seastar {
    template <typename Signature> class noncopyable_function;
    namespace internal {
    class noncopyable_function_base {
      static constexpr size_t nr_direct = 32;
      union [[gnu::may_alias]] storage {
      };
      using move_type = void (*)(noncopyable_function_base *from,
                                 noncopyable_function_base *to);
      static void empty_move(noncopyable_function_base *from,
                             noncopyable_function_base *to);
      static void empty_destroy(noncopyable_function_base *func);
      static void indirect_move(noncopyable_function_base *from,
                                noncopyable_function_base *to);
    };
    }
    template <typename Ret, typename... Args>
    class noncopyable_function<Ret(Args...)>
        : private internal::noncopyable_function_base {
      struct vtable {
      };
      static Ret empty_call(const noncopyable_function *func, Args... args);
      static constexpr vtable _s_empty_vtable = {empty_call, empty_move,
                                                 empty_destroy};
      template <typename Func> struct direct_vtable_for {
        static void move(noncopyable_function_base *from,
                         noncopyable_function_base *to);
      };
      template <typename Func> struct indirect_vtable_for {
        static constexpr vtable make_vtable() {
        }
      };
      template <typename Func, bool Direct = true>
      struct select_vtable_for : direct_vtable_for<Func> {};
      template <typename Func> static constexpr bool is_direct() {
        return sizeof(Func) <= nr_direct && alignof(Func) <= alignof(storage) &&
               std::is_nothrow_move_constructible<Func>::value;
      }
      template <typename Func>
      struct vtable_for : select_vtable_for<Func, is_direct<Func>()> {};
    public:
      template <typename Func> noncopyable_function(Func func) {
      }
      template <typename Object, typename... AllButFirstArg>
      noncopyable_function(Ret (Object::*member)(AllButFirstArg...))
          : noncopyable_function(std::mem_fn(member)) {}
    };
    namespace memory {
    class alloc_failure_injector {
      noncopyable_function<void()> _on_alloc_failure = [] {
      };
    };
    struct disable_failure_guard {
    };
    }
   }
#define SEASTAR_NODISCARD [[nodiscard]]
namespace seastar {
    template <typename... T, typename... A>
    future<T...> make_ready_future(A && ... value);
    namespace internal {
    template <typename... T>
    future<T...> current_exception_as_future() noexcept;
    template <typename... T> struct get0_return_type {
      using type = void;
    };
    template <typename T0, typename... T> struct get0_return_type<T0, T...> {
      using type = T0;
    };
    template <typename T, bool is_trivial_class>
    struct uninitialized_wrapper_base;
    template <typename T> struct uninitialized_wrapper_base<T, false> {
      union any {
      } _v;
      T &uninitialized_get();
    };
    template <typename T>
    struct uninitialized_wrapper_base<T, true> : private T {
    };
    template <typename T>
    constexpr bool can_inherit = std::is_same<std::tuple<>, T>::value ||
                                 (std::is_trivially_destructible<T>::value &&
                                  std::is_trivially_constructible<T>::value &&
                                  !std::is_final<T>::value);
    template <typename T>
    struct uninitialized_wrapper
        : public uninitialized_wrapper_base<T, can_inherit<T>> {};
    static_assert(std::is_empty<uninitialized_wrapper<std::tuple<>>>::value,
                  "This should still be empty");
    template <typename T>
    struct is_trivially_move_constructible_and_destructible {
      static constexpr bool value =
          std::is_trivially_destructible<T>::value;
    };
    template <bool... v> struct all_true : std::false_type {};
    }
    struct future_state_base {
      static_assert(
          "std::exception_ptr's copy constructor must not throw");
      static_assert(
          "std::exception_ptr's move constructor must not throw");
      enum class state : uintptr_t {
        result = 3,
        exception_min = 4,
      };
      union any {
        state st;
      } _u;
      future_state_base() noexcept;
      future_state_base(future_state_base &&x) noexcept : _u(std::move(x._u)) {}
      ~future_state_base() noexcept {
        if (failed()) {
        }
      }
    public:
      bool available() const noexcept {
        return _u.st == state::result || _u.st >= state::exception_min;
      }
      bool failed() const noexcept;
    };
    struct ready_future_marker {};
    struct future_for_get_promise_marker {};
    template <typename... T>
    struct future_state
        : public future_state_base,
          private internal::uninitialized_wrapper<std::tuple<T...>> {
      static constexpr bool has_trivial_move_and_destroy = internal::all_true<
          internal::is_trivially_move_constructible_and_destructible<
              T>::value...>::value;
      static_assert(std::is_nothrow_move_constructible<std::tuple<T...>>::value,
                    "Types must be no-throw destructible");
      future_state() noexcept {}
      [[gnu::always_inline]] future_state(future_state &&x) noexcept
          : future_state_base(std::move(x)) {
        if (has_trivial_move_and_destroy) {
          memcpy(reinterpret_cast<char *>(&this->uninitialized_get()),
                 &x.uninitialized_get(),
                 internal::used_size<std::tuple<T...>>::value);
        }
      }
      future_state &operator=(future_state &&x) noexcept {
      }
      template <typename... A>
      future_state(ready_future_marker, A &&... a)
          : future_state_base(state::result) {
      }
      std::tuple<T...> &&get_value() && noexcept {
        assert(_u.st == state::result);
      }
      std::tuple<T...> &&take_value() && noexcept {
      }
      std::tuple<T...> &&take() && {
        if (_u.st >= state::exception_min) {
        }
      }
      const std::tuple<T...> &get() const & {
        if (_u.st >= state::exception_min) {
        }
      }
      using get0_return_type = typename internal::get0_return_type<T...>::type;
      static get0_return_type get0(std::tuple<T...> &&x) {
      }
    };
    template <typename... T> class continuation_base : public task {
    protected:
      future_state<T...> _state;
      continuation_base() = default;
      explicit continuation_base(future_state<T...> &&state)
          : _state(std::move(state)) {}
    };
    template <typename Func, typename... T>
    struct continuation final : continuation_base<T...> {
      virtual void run_and_dispose() noexcept override {
      }
      Func _func;
    };
    namespace internal {
    template <typename... T>
    future<T...> make_exception_future(future_state_base &&state) noexcept;
    template <typename... T, typename U>
    void set_callback(future<T...> &fut, U *callback) noexcept;
    class promise_base {
      future_state_base *_state;
      template <typename Exception>
      std::enable_if_t<!std::is_same<std::remove_reference_t<Exception>,
                                     std::exception_ptr>::value,
                       void>
      set_exception(Exception &&e) noexcept {
      }
    };
    template <typename... T>
    class promise_base_with_type : protected internal::promise_base {
      promise_base_with_type(future<T...> *future);
      void set_urgent_state(future_state<T...> &&state) noexcept {
        if (_state) {
        }
      }
      template <typename Func> void schedule(Func &&func) noexcept {
      }
      template <typename... U> friend class seastar::future;
    };
    }
    template <typename... T>
    class promise : private internal::promise_base_with_type<T...> {
    public:
      promise();
      future<T...> get_future() noexcept;
      template <typename Exception>
      std::enable_if_t<!std::is_same<std::remove_reference_t<Exception>,
                                     std::exception_ptr>::value,
                       void>
      set_exception(Exception &&e) noexcept {
      }
    };
    template <typename... T> struct is_future : std::false_type {};
    template <typename T> struct futurize {
      using type = future<T>;
      template <typename Func, typename... FuncArgs>
      static inline type apply(Func &&func,
                               std::tuple<FuncArgs...> &&args) noexcept;
      static type convert(T &&value);
      template <typename Arg> static type make_exception_future(Arg &&arg);
    };
    template <typename... Args> struct futurize<future<Args...>> {
      using type = future<Args...>;
      template <typename Func, typename... FuncArgs>
      static inline type apply(Func &&func,
                               std::tuple<FuncArgs...> &&args) noexcept;
      template <typename Func, typename... FuncArgs>
      static inline type apply(Func &&func, FuncArgs &&... args) noexcept;
      template <typename Arg> static type make_exception_future(Arg &&arg);
    };
    template <typename T> using futurize_t = typename futurize<T>::type;
    GCC6_CONCEPT(template <typename T> concept bool Future =
                     is_future<T>::value;)
    namespace internal {
      class future_base {
      protected:
        promise_base *_promise;
        future_base() noexcept : _promise(nullptr) {}
        future_base(future_base &&x, future_state_base *state);
        promise_base *detach_promise() noexcept;
      };
      template <bool IsVariadic> struct warn_variadic_future {
      };
      template <> struct warn_variadic_future<true> {
        [[deprecated(
            "deprecated, replace with future<std::tuple<...>>")]] void
        check_deprecation() {}
      };
    }
    template <typename... T>
    class SEASTAR_NODISCARD future
        : private internal::future_base,
          internal::warn_variadic_future<(sizeof...(T) > 1)> {
      future_state<T...> _state;
      future(future_for_get_promise_marker m) {}
      [[gnu::always_inline]] explicit future(
          future_state<T...> &&state) noexcept
          : _state(std::move(state)) {
      }
      internal::promise_base_with_type<T...> get_promise() noexcept {
        return internal::promise_base_with_type<T...>(this);
      }
      internal::promise_base_with_type<T...> *detach_promise() {
        return static_cast<internal::promise_base_with_type<T...> *>(
            future_base::detach_promise());
      }
      template <typename Func> void schedule(Func &&func) noexcept {
        if (_state.available() || !_promise) {
          if (__builtin_expect(!_state.available() && !_promise, false)) {
          }
        }
      }
      [[gnu::always_inline]] future_state<T...> &&
      get_available_state_ref() noexcept {
        if (_promise) {
        }
        return std::move(_state);
      }
      [[gnu::noinline]] future<T...> rethrow_with_nested() {
        if (!failed()) {
          try {
          } catch (...) {
          }
        }
      }
    public:
      using promise_type = promise<T...>;
      [[gnu::always_inline]] future(future &&x) noexcept
          : future_base(std::move(x), &_state), _state(std::move(x._state)) {}
      future &operator=(future &&x) noexcept {
      }
      [[gnu::always_inline]] std::tuple<T...> &&get() {
        if (!_state.available()) {
        }
      }
      typename future_state<T...>::get0_return_type get0() {
      }
      class thread_wake_task final : public continuation_base<T...> {
        virtual void run_and_dispose() noexcept override {
        }
      };
      void do_wait() noexcept {
        if (__builtin_expect(!_promise, false)) {
        }
        auto thread = thread_impl::get();
        thread_wake_task wake_task{thread, this};
        detach_promise()->schedule(
            static_cast<continuation_base<T...> *>(&wake_task));
      }
    public:
      [[gnu::always_inline]] bool available() const noexcept {
        return _state.available();
      }
      [[gnu::always_inline]] bool failed() const noexcept {
        return _state.failed();
      }
      template <typename Func,
                typename Result = futurize_t<std::result_of_t<Func(T &&...)>>>
      Result then(Func &&func) noexcept {
        return then_impl(std::move(func));
      }
      template <typename Func,
                typename Result = futurize_t<std::result_of_t<Func(T &&...)>>>
      Result then_impl(Func &&func) noexcept {
        using futurator = futurize<std::result_of_t<Func(T && ...)>>;
        if (available() && !need_preempt()) {
          if (failed()) {
            return futurator::make_exception_future(
                static_cast<future_state_base &&>(get_available_state_ref()));
          }
        }
        typename futurator::type fut(future_for_get_promise_marker{});
        [&]() noexcept {
          schedule([pr = fut.get_promise(), func = std::forward<Func>(func)](
                       future_state<T...> &&state) mutable {
            if (state.failed()) {
              futurator::apply(std::forward<Func>(func),
                               std::move(state).get_value())
                  .forward_to(std::move(pr));
            }
          });
        }();
        return fut;
      }
      template <typename Func,
                typename FuncResult = std::result_of_t<Func(future)>>
          futurize_t<FuncResult> then_wrapped(Func &&func) & noexcept {
      }
      template <bool AsSelf, typename FuncResult, typename Func>
      futurize_t<FuncResult> then_wrapped_maybe_erase(Func &&func) noexcept {
      }
      template <bool AsSelf, typename FuncResult, typename Func>
      futurize_t<FuncResult> then_wrapped_common(Func &&func) noexcept {
        using futurator = futurize<FuncResult>;
        if (available() && !need_preempt()) {
          if (AsSelf) {
            if (_promise) {
            }
            return futurator::apply(std::forward<Func>(func),
                                    future(get_available_state_ref()));
          }
        }
        typename futurator::type fut(future_for_get_promise_marker{});
        [&]() noexcept {
          schedule([pr = fut.get_promise(), func = std::forward<Func>(func)](
                       future_state<T...> &&state) mutable {
          });
        }();
      }
      void forward_to(internal::promise_base_with_type<T...> &&pr) noexcept {
      }
      void forward_to(promise<T...> &&pr) noexcept {
        if (_state.available()) {
        }
      }
      template <typename Func>
      future<T...> finally(Func &&func) noexcept {
        return then_wrapped(
            finally_body<Func, is_future<std::result_of_t<Func()>>::value>(
                std::forward<Func>(func)));
      }
      template <typename Func, bool FuncReturnsFuture> struct finally_body;
      template <typename Func> struct finally_body<Func, true> {
        Func _func;
        future<T...> operator()(future<T...> &&result) {
          using futurator = futurize<std::result_of_t<Func()>>;
          return futurator::apply(_func).then_wrapped(
              [result = std::move(result)](auto f_res) mutable {
                if (!f_res.failed()) {
                  try {
                  } catch (...) {
                  }
                }
              });
        }
      };
      template <typename Func> struct finally_body<Func, false> {
        future<T...> operator()(future<T...> &&result) {
          try {
          } catch (...) {
          }
        };
      };
      future<> or_terminate() noexcept {
        return then_wrapped([](auto &&f) {
          try {
          } catch (...) {
          }
        });
      }
      template <typename Func>
      future<T...> handle_exception(Func &&func) noexcept {
        return then_wrapped([func = std::forward<Func>(func)](
                                auto &&fut) mutable -> future<T...> {
          if (!fut.failed()) {
          }
        });
      }
      template <typename Func>
      future<T...> handle_exception_type(Func &&func) noexcept {
        using trait = function_traits<Func>;
        using ex_type = typename trait::template arg<0>::type;
        return then_wrapped([func = std::forward<Func>(func)](
                                auto &&fut) mutable -> future<T...> {
          try {
          } catch (ex_type &ex) {
          }
        });
      }
      void set_callback(continuation_base<T...> *callback) noexcept {
        if (_state.available()) {
        }
      }
      template <typename... U> friend class future;
      template <typename... U, typename... A>
      friend future<U...>
      internal::make_exception_future(future_state_base &&state) noexcept;
    };
    template <typename T>
    template <typename Func, typename... FuncArgs>
    typename futurize<T>::type futurize<T>::apply(
        Func && func, std::tuple<FuncArgs...> && args) noexcept {
      try {
        return convert(
            ::seastar::apply(std::forward<Func>(func), std::move(args)));
      } catch (...) {
      }
    }
    template <typename... Args>
    template <typename Func, typename... FuncArgs>
    typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(
        Func && func, std::tuple<FuncArgs...> && args) noexcept {
      try {
        return ::seastar::apply(std::forward<Func>(func), std::move(args));
      } catch (...) {
      }
    }
    template <typename Tag> class bool_class {
    };
    namespace internal {
    template <typename Future> struct continuation_base_from_future;
    template <typename... T>
    struct continuation_base_from_future<future<T...>> {
      using type = continuation_base<T...>;
    };
    template <typename HeldState, typename Future>
    class do_with_state final
        : public continuation_base_from_future<Future>::type {
      HeldState _held;
      typename Future::promise_type _pr;
    public:
      explicit do_with_state(HeldState &&held) : _held(std::move(held)) {}
      virtual void run_and_dispose() noexcept override {
      }
      HeldState &data() { return _held; }
      Future get_future() { return _pr.get_future(); }
    };
    }
    template <typename Tuple, size_t... Idx>
    inline auto cherry_pick_tuple(std::index_sequence<Idx...>, Tuple && tuple) {
      return std::make_tuple(std::get<Idx>(std::forward<Tuple>(tuple))...);
    }
    template <typename T1, typename T2, typename T3_or_F, typename... More>
    inline auto do_with(T1 && rv1, T2 && rv2, T3_or_F && rv3,
                        More && ... more) {
      auto all = std::forward_as_tuple(
          std::forward<T1>(rv1), std::forward<T2>(rv2),
          std::forward<T3_or_F>(rv3), std::forward<More>(more)...);
      constexpr size_t nr = std::tuple_size<decltype(all)>::value - 1;
      using idx = std::make_index_sequence<nr>;
      auto &&just_values = cherry_pick_tuple(idx(), std::move(all));
      auto &&just_func = std::move(std::get<nr>(std::move(all)));
      using value_tuple = std::remove_reference_t<decltype(just_values)>;
      using ret_type = decltype(apply(just_func, just_values));
      auto task =
          std::make_unique<internal::do_with_state<value_tuple, ret_type>>(
              std::move(just_values));
      auto fut = apply(just_func, task->data());
      if (fut.available()) {
      }
      auto ret = task->get_future();
      return ret;
    }
   }
 namespace seastar {
    class timer_set {
    };
   };
    namespace seastar {
    using steady_clock_type = std::chrono::steady_clock;
    template <typename Clock = steady_clock_type> class timer {
    };
    struct stop_iteration_tag {};
    using stop_iteration = bool_class<stop_iteration_tag>;
 }
   namespace seastar {
    class lowres_clock;
    class lowres_clock_impl final {
    public:
      using base_steady_clock = std::chrono::steady_clock;
      using period = std::ratio<1, 1000>;
      using steady_rep = base_steady_clock::rep;
      using steady_duration = std::chrono::duration<steady_rep, period>;
      using steady_time_point =
          std::chrono::time_point<lowres_clock, steady_duration>;
    };
    class lowres_clock final {
    public:
      using time_point = lowres_clock_impl::steady_time_point;
    };
    class manual_clock {
    };
    namespace metrics {
    namespace impl {
    };
    };
}
namespace seastar {
    namespace alien {
    }
}
namespace std {
}
namespace seastar {
    class reactor {
    };  }
    struct blob_storage {  }
    __attribute__((packed));
    class table;
    using column_family = table;
    class clustering_key_prefix;
    template <typename T> class nonwrapping_range { };
    GCC6_CONCEPT(template <template <typename> typename T, typename U>              concept bool Range =                  std::is_same<T<U>, wrapping_range<U>>::value ||                  std::is_same<T<U>, nonwrapping_range<U>>::value;
   ) namespace std { }
    namespace dht {
    class decorated_key;  }
    template <typename EnumType, EnumType... Items> struct super_enum { };
    template <typename Enum> class enum_set { };
    namespace tracing {
    class trace_state_ptr;  }
    namespace query {
    using column_id_vector = utils::small_vector<column_id, 8>;
    using clustering_range = nonwrapping_range<clustering_key_prefix>;
    typedef std::vector<clustering_range> clustering_row_ranges;
    class specific_ranges {};
    constexpr auto max_rows = std::numeric_limits<uint32_t>::max();
    class partition_slice {
    public:
      enum class option {
        send_clustering_key,
        send_partition_key,
        always_return_static_content,
      };
      using option_set = enum_set<super_enum<
          option, option::send_clustering_key, option::send_partition_key,
          option::always_return_static_content>>;
      partition_slice(
          clustering_row_ranges row_ranges, column_id_vector static_columns,
          column_id_vector regular_columns, option_set options,
          std::unique_ptr<specific_ranges> specific_ranges = nullptr,
          cql_serialization_format = cql_serialization_format::internal(),
          uint32_t partition_row_limit = max_rows);
    };  }
    namespace db {
    using timeout_clock = seastar::lowres_clock;  }
    GCC6_CONCEPT(template <typename T, typename ReturnType>              concept bool MutationFragmentConsumer() {  ) class mutation final {
      mutation() = default;
    public:
      const dht::decorated_key &decorated_key() const;
    };
    using mutation_opt = optimized_optional<mutation>;
    class flat_mutation_reader;
    future<mutation_opt> read_mutation_from_flat_mutation_reader(
        flat_mutation_reader & reader, db::timeout_clock::time_point timeout);
    class locked_cell;
    class frozen_mutation;
    class table {
    public:
      future<std::vector<locked_cell>>
      lock_counter_cells(const mutation &m,
                         db::timeout_clock::time_point timeout);
    };
    class database {
      future<mutation>
      do_apply_counter_update(column_family &cf, const frozen_mutation &fm,
                              schema_ptr m_schema,
                              db::timeout_clock::time_point timeout,
                              tracing::trace_state_ptr trace_state);
    };
    namespace tracing {
    class trace_state_ptr final {
    public:
      trace_state_ptr(nullptr_t);
    };
    }
    template <typename Consumer>
    inline future<> consume_partitions(flat_mutation_reader & reader,
                                       Consumer consumer,
                                       db::timeout_clock::time_point timeout) {
      using futurator = futurize<std::result_of_t<Consumer(mutation &&)>>;
      return do_with(
          std::move(consumer), [&reader, timeout](Consumer &c) -> future<> {
            return repeat([&reader, &c, timeout]() {
              return read_mutation_from_flat_mutation_reader(reader, timeout)
                  .then([&c](mutation_opt &&mo) -> future<stop_iteration> {
                    if (!mo) {
                    }
                    return futurator::apply(c, std::move(*mo));
                  });
            });
          });
    }
    class frozen_mutation final {
    public:
      mutation unfreeze(schema_ptr s) const;
    };
    class mutation_source {};
    future<mutation_opt> counter_write_query(
        schema_ptr, const mutation_source &, const dht::decorated_key &dk,
        const query::partition_slice &slice,
        tracing::trace_state_ptr trace_ptr);
    class locked_cell {};
    future<mutation> database::do_apply_counter_update(
        column_family & cf, const frozen_mutation &fm, schema_ptr m_schema,
        db::timeout_clock::time_point timeout,
        tracing::trace_state_ptr trace_state) {
      auto m = fm.unfreeze(m_schema);
      query::column_id_vector static_columns;
      query::clustering_row_ranges cr_ranges;
      query::column_id_vector regular_columns;
      auto slice = query::partition_slice(
          std::move(cr_ranges), std::move(static_columns),
          std::move(regular_columns), {}, {},
          cql_serialization_format::internal(), query::max_rows);
      return do_with(
          std::move(slice), std::move(m), std::vector<locked_cell>(),
          [this, &cf, timeout](const query::partition_slice &slice, mutation &m,
                               std::vector<locked_cell> &locks) mutable {
            return cf.lock_counter_cells(m, timeout)
                .then([&, timeout, this](std::vector<locked_cell> lcs) mutable {
                  return counter_write_query(schema_ptr(), mutation_source(),
                                             m.decorated_key(), slice, nullptr)
                      .then([this, &cf, &m, timeout](auto mopt) {
                        return std::move(m);
                      });
                });
          });
    }