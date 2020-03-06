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
GCC6_CONCEPT(template <typename H> concept bool Hasher() {
    ) template <typename T, typename Enable = void> struct appending_hash;
    namespace seastar {
    struct lw_shared_ptr_counter_base {};
    namespace internal {
    template <class T, class U> struct lw_shared_ptr_accessors;
    }
    template <typename T>
    class enable_lw_shared_from_this : private lw_shared_ptr_counter_base {};
    template <typename T>
    struct shared_ptr_no_esft : private lw_shared_ptr_counter_base {};
    template <typename T> struct lw_shared_ptr_deleter;
    namespace internal {
    template <typename... T> using void_t = void;
    template <typename T>
    struct lw_shared_ptr_accessors<
        T, void_t<decltype(lw_shared_ptr_deleter<T>{})>> {
      using concrete_type = T;
    };
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
    namespace utils {
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
      static_assert(N > 0);
      static_assert(std::is_nothrow_move_constructible_v<T>);
      static_assert(std::is_nothrow_move_assignable_v<T>);
      static_assert(std::is_nothrow_destructible_v<T>);
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
        auto n_end = std::uninitialized_move(begin(), end(), ptr);
        std::destroy(begin(), end());
        if (!uses_internal_storage()) {
          std::free(_begin);
        }
        _begin = ptr;
        _end = n_end;
        _capacity_end = ptr + new_capacity;
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
          std::free(ptr);
          throw;
        }
        std::destroy(begin(), end());
        if (!uses_internal_storage()) {
          std::free(_begin);
        }
        _begin = ptr;
        _end = n_end;
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
      using pointer = T *;
      using const_pointer = const T *;
      using reference = T &;
      using const_reference = const T &;
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
          _end = std::uninitialized_copy(first, last, _end);
        } else {
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
            std::memcpy(_internal.storage, other._internal.storage,
                        N * sizeof(T));
            _end = _begin + other.size();
          } else {
            _end = _begin;
            for (auto &e : other) {
              new (_end++) T(std::move(e));
              e.~T();
            }
          }
          other._end = other._internal.storage;
        } else {
          _begin = std::exchange(other._begin, other._internal.storage);
          _end = std::exchange(other._end, other._internal.storage);
          _capacity_end =
              std::exchange(other._capacity_end, other._internal.storage + N);
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
            std::free(_begin);
            _begin = _internal.storage;
          }
          _capacity_end = _begin + N;
          if constexpr (std::is_trivially_copyable_v<T>) {
            std::memcpy(_internal.storage, other._internal.storage,
                        N * sizeof(T));
            _end = _begin + other.size();
          } else {
            _end = _begin;
            for (auto &e : other) {
              new (_end++) T(std::move(e));
              e.~T();
            }
          }
          other._end = other._internal.storage;
        } else {
          if (__builtin_expect(!uses_internal_storage(), false)) {
            std::free(_begin);
          }
          _begin = std::exchange(other._begin, other._internal.storage);
          _end = std::exchange(other._end, other._internal.storage);
          _capacity_end =
              std::exchange(other._capacity_end, other._internal.storage + N);
        }
        return *this;
      }
      small_vector &operator=(const small_vector &other) {
        if constexpr (std::is_nothrow_copy_constructible_v<T>) {
          if (capacity() >= other.size()) {
            clear();
            _end = std::uninitialized_copy(other.begin(), other.end(), _end);
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
      void reserve(size_t n) {
        if (__builtin_expect(_begin + n > _capacity_end, false)) {
          expand(n);
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
      const_iterator end() const noexcept { return _end; }
      const_iterator cend() const noexcept { return _end; }
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
      }
      const_reverse_iterator crend() const noexcept {
        return const_reverse_iterator(begin());
      }
      T *data() noexcept { return _begin; }
      const T *data() const noexcept { return _begin; }
      T &front() noexcept { return *begin(); }
      const T &front() const noexcept { return *begin(); }
      T &back() noexcept { return end()[-1]; }
      const T &back() const noexcept { return end()[-1]; }
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
        auto &ref = *new (_end) T(std::forward<Args>(args)...);
        ++_end;
        return ref;
      }
      T &push_back(const T &value) { return emplace_back(value); }
      T &push_back(T &&value) { return emplace_back(std::move(value)); }
      template <typename InputIterator>
      iterator insert(const_iterator cpos, InputIterator first,
                      InputIterator last) {
        if constexpr (std::is_base_of_v<
                          std::forward_iterator_tag,
                          typename std::iterator_traits<
                              InputIterator>::iterator_category>) {
          if (first == last) {
            return const_cast<iterator>(cpos);
          }
          auto idx = cpos - _begin;
          auto new_count = std::distance(first, last);
          reserve_at_least(size() + new_count);
          auto pos = _begin + idx;
          auto after = std::distance(pos, end());
          if (__builtin_expect(pos == end(), true)) {
            _end = std::uninitialized_copy(first, last, end());
            return pos;
          } else if (after > new_count) {
            std::uninitialized_move(end() - new_count, end(), end());
            std::move_backward(pos, end() - new_count, end());
            try {
              std::copy(first, last, pos);
            } catch (...) {
              std::move(pos + new_count, end() + new_count, pos);
              std::destroy(end(), end() + new_count);
              throw;
            }
          } else {
            std::uninitialized_move(pos, end(), pos + new_count);
            auto mid = std::next(first, after);
            try {
              std::uninitialized_copy(mid, last, end());
              try {
                std::copy(first, mid, pos);
              } catch (...) {
                std::destroy(end(), pos + new_count);
                throw;
              }
            } catch (...) {
              std::move(pos + new_count, end() + new_count, pos);
              std::destroy(pos + new_count, end() + new_count);
              throw;
            }
          }
          _end += new_count;
          return pos;
        } else {
          auto start = cpos - _begin;
          auto idx = start;
          while (first != last) {
            try {
              insert(begin() + idx, *first);
              ++first;
              ++idx;
            } catch (...) {
              erase(begin() + start, begin() + idx);
              throw;
            }
          }
          return begin() + idx;
        }
      }
      template <typename... Args>
      iterator emplace(const_iterator cpos, Args &&... args) {
        auto idx = cpos - _begin;
        reserve_at_least(size() + 1);
        auto pos = _begin + idx;
        if (pos != _end) {
          new (_end) T(std::move(_end[-1]));
          std::move_backward(pos, _end - 1, _end);
          pos->~T();
        }
        try {
          new (pos) T(std::forward<Args>(args)...);
        } catch (...) {
          if (pos != _end) {
            new (pos) T(std::move(pos[1]));
            std::move(pos + 2, _end + 1, pos + 1);
            _end->~T();
          }
          throw;
        }
        _end++;
        return pos;
      }
      iterator insert(const_iterator cpos, const T &obj) {
        return emplace(cpos, obj);
      }
      iterator insert(const_iterator cpos, T &&obj) {
        return emplace(cpos, std::move(obj));
      }
      void resize(size_t n) {
        if (n < size()) {
          erase(end() - (size() - n), end());
        } else if (n > size()) {
          reserve_at_least(n);
          _end = std::uninitialized_value_construct_n(_end, n - size());
        }
      }
      void resize(size_t n, const T &value) {
        if (n < size()) {
          erase(end() - (size() - n), end());
        } else if (n > size()) {
          reserve_at_least(n);
          auto nend = _begin + n;
          std::uninitialized_fill(_end, nend, value);
          _end = nend;
        }
      }
      void pop_back() noexcept { (--_end)->~T(); }
      iterator erase(const_iterator cit) noexcept {
        return erase(cit, cit + 1);
      }
      iterator erase(const_iterator cfirst, const_iterator clast) noexcept {
        auto first = const_cast<iterator>(cfirst);
        auto last = const_cast<iterator>(clast);
        std::move(last, end(), first);
        auto nend = _end - (clast - cfirst);
        std::destroy(nend, _end);
        _end = nend;
        return first;
      }
      void swap(small_vector &other) noexcept { std::swap(*this, other); }
      bool operator==(const small_vector &other) const noexcept {
        return size() == other.size() &&
               std::equal(_begin, _end, other.begin());
      }
      bool operator!=(const small_vector &other) const noexcept {
        return !(*this == other);
      }
    };
   }
#include <arpa/inet.h>  
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
    class scheduling_group;
    namespace internal {
    unsigned scheduling_group_index(scheduling_group sg);
    }
    class scheduling_group_key {
      ;
    };
    class scheduling_group {
      unsigned _id;
    private:
      friend class reactor;
      friend unsigned internal::scheduling_group_index(scheduling_group sg);
      ;
      ;
    };
    namespace internal {
    unsigned scheduling_group_index(scheduling_group sg);
    scheduling_group *current_scheduling_group_ptr();
    }
    scheduling_group current_scheduling_group();
    class task {
      scheduling_group _sg;
    protected:
      ~task() = default;
    public:
      explicit task(scheduling_group sg = current_scheduling_group());
      virtual void run_and_dispose() noexcept = 0;
      scheduling_group group() const;
    };
    void schedule(task * t) noexcept;
    namespace internal {
    struct preemption_monitor {
      std::atomic<uint32_t> head;
      std::atomic<uint32_t> tail;
    };
    }
    extern __thread const internal::preemption_monitor *g_need_preempt;
    bool need_preempt() noexcept;
   }
#include <setjmp.h>
#include <ucontext.h>
namespace seastar {
    class thread_context;
    struct jmp_buf_link {
      jmp_buf jmpbuf;
      void switch_in();
      void switch_out();
      void initial_switch_in_completed();
      void final_switch_out();
    };
    extern thread_local jmp_buf_link *g_current_context;
    namespace thread_impl {
    thread_context *get();
    bool should_yield();
    scheduling_group sched_group(const thread_context *);
    void yield();
    void switch_in(thread_context *to);
    void switch_out(thread_context *from);
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
    private:
      static constexpr size_t nr_direct = 32;
      union [[gnu::may_alias]] storage {
        char direct[nr_direct];
        void *indirect;
      };
      using move_type = void (*)(noncopyable_function_base *from,
                                 noncopyable_function_base *to);
      using destroy_type = void (*)(noncopyable_function_base *func);
      static void empty_move(noncopyable_function_base *from,
                             noncopyable_function_base *to);
      static void empty_destroy(noncopyable_function_base *func);
      static void indirect_move(noncopyable_function_base *from,
                                noncopyable_function_base *to);
      ;
    private:
      storage _storage;
      template <typename Signature> friend class seastar::noncopyable_function;
    };
    }
    template <typename Ret, typename... Args>
    class noncopyable_function<Ret(Args...)>
        : private internal::noncopyable_function_base {
      using call_type = Ret (*)(const noncopyable_function *func, Args...);
      struct vtable {
        const call_type call;
        const move_type move;
        const destroy_type destroy;
      };
    private:
      const vtable *_vtable;
    private:
      static Ret empty_call(const noncopyable_function *func, Args... args);
      static constexpr vtable _s_empty_vtable = {empty_call, empty_move,
                                                 empty_destroy};
      template <typename Func> struct direct_vtable_for {
        static Ret call(const noncopyable_function *func, Args... args);
        static void move(noncopyable_function_base *from,
                         noncopyable_function_base *to);
        static constexpr move_type select_move_thunk();
        static void destroy(noncopyable_function_base *func);
        static constexpr destroy_type select_destroy_thunk();
        static void initialize(Func &&from, noncopyable_function *to);
        static constexpr vtable make_vtable();
        static const vtable s_vtable;
      };
      template <typename Func> struct indirect_vtable_for {
        static Func *access(noncopyable_function *func);
        static const Func *access(const noncopyable_function *func);
        static Func *access(noncopyable_function_base *func);
        static Ret call(const noncopyable_function *func, Args... args);
        static void destroy(noncopyable_function_base *func);
        static void initialize(Func &&from, noncopyable_function *to);
        static constexpr vtable make_vtable() {
          return {call, indirect_move, destroy};
        }
        static const vtable s_vtable;
      };
      template <typename Func, bool Direct = true>
      struct select_vtable_for : direct_vtable_for<Func> {};
      template <typename Func>
      struct select_vtable_for<Func, false> : indirect_vtable_for<Func> {};
      template <typename Func> static constexpr bool is_direct() {
        return sizeof(Func) <= nr_direct && alignof(Func) <= alignof(storage) &&
               std::is_nothrow_move_constructible<Func>::value;
      }
      template <typename Func>
      struct vtable_for : select_vtable_for<Func, is_direct<Func>()> {};
    public:
      noncopyable_function() noexcept : _vtable(&_s_empty_vtable) {}
      template <typename Func> noncopyable_function(Func func) {
        vtable_for<Func>::initialize(std::move(func), this);
        _vtable = &vtable_for<Func>::s_vtable;
      }
      template <typename Object, typename... AllButFirstArg>
      noncopyable_function(Ret (Object::*member)(AllButFirstArg...))
          : noncopyable_function(std::mem_fn(member)) {}
    };
    namespace memory {
    class alloc_failure_injector {
      uint64_t _alloc_count;
      uint64_t _fail_at = std::numeric_limits<uint64_t>::max();
      noncopyable_function<void()> _on_alloc_failure = [] {
        throw std::bad_alloc();
      };
      bool _failed;
      uint64_t _suppressed = 0;
      friend struct disable_failure_guard;
    private:
    public:
    };
    extern thread_local alloc_failure_injector the_alloc_failure_injector;
    struct disable_failure_guard {
      ~disable_failure_guard();
    };
    }
   }
#define SEASTAR_NODISCARD [[nodiscard]]
namespace seastar {
    template <class... T> class promise;
    template <typename... T, typename... A>
    future<T...> make_ready_future(A && ... value);
    void engine_exit(std::exception_ptr eptr = {});
    void report_failed_future(const std::exception_ptr &ex) noexcept;
    namespace internal {
    template <class... T> class promise_base_with_type;
    template <typename... T>
    future<T...> current_exception_as_future() noexcept;
    extern template future<> current_exception_as_future() noexcept;
    template <typename... T> struct get0_return_type {
      using type = void;
      static type get0(std::tuple<T...> v);
    };
    template <typename T0, typename... T> struct get0_return_type<T0, T...> {
      using type = T0;
      static type get0(std::tuple<T0, T...> v);
    };
    template <typename T, bool is_trivial_class>
    struct uninitialized_wrapper_base;
    template <typename T> struct uninitialized_wrapper_base<T, false> {
      union any {
        any();
        ~any();
        T value;
      } _v;
    public:
      void uninitialized_set(T &&v);
      T &uninitialized_get();
      const T &uninitialized_get() const;
    };
    template <typename T>
    struct uninitialized_wrapper_base<T, true> : private T {
      void uninitialized_set(T &&v);
      T &uninitialized_get();
      const T &uninitialized_get() const;
    };
    template <typename T>
    constexpr bool can_inherit = std::is_same<std::tuple<>, T>::value ||
                                 (std::is_trivially_destructible<T>::value &&
                                  std::is_trivially_constructible<T>::value &&
                                  std::is_class<T>::value &&
                                  !std::is_final<T>::value);
    template <typename T>
    struct uninitialized_wrapper
        : public uninitialized_wrapper_base<T, can_inherit<T>> {};
    static_assert(std::is_empty<uninitialized_wrapper<std::tuple<>>>::value,
                  "This should still be empty");
    template <typename T>
    struct is_trivially_move_constructible_and_destructible {
      static constexpr bool value =
          std::is_trivially_move_constructible<T>::value &&
          std::is_trivially_destructible<T>::value;
    };
    template <bool... v> struct all_true : std::false_type {};
    template <> struct all_true<> : std::true_type {};
    template <bool... v> struct all_true<true, v...> : public all_true<v...> {};
    }
    struct future_state_base {
      static_assert(
          std::is_nothrow_copy_constructible<std::exception_ptr>::value,
          "std::exception_ptr's copy constructor must not throw");
      static_assert(
          std::is_nothrow_move_constructible<std::exception_ptr>::value,
          "std::exception_ptr's move constructor must not throw");
      static_assert(sizeof(std::exception_ptr) == sizeof(void *),
                    "exception_ptr not a pointer");
      enum class state : uintptr_t {
        invalid = 0,
        future = 1,
        result_unavailable = 2,
        result = 3,
        exception_min = 4,
      };
      union any {
        any();
        any(state s);
        void set_exception(std::exception_ptr &&e);
        any(std::exception_ptr &&e);
        ~any();
        std::exception_ptr take_exception();
        any(any &&x);
        bool has_result() const;
        state st;
        std::exception_ptr ex;
      } _u;
      future_state_base() noexcept;
      future_state_base(state st) noexcept : _u(st) {}
      future_state_base(std::exception_ptr &&ex) noexcept : _u(std::move(ex)) {}
      future_state_base(future_state_base &&x) noexcept : _u(std::move(x._u)) {}
    protected:
      ~future_state_base() noexcept {
        if (failed()) {
          report_failed_future(_u.take_exception());
        }
      }
    public:
      bool valid() const noexcept { return _u.st != state::invalid; }
      bool available() const noexcept {
        return _u.st == state::result || _u.st >= state::exception_min;
      }
      bool failed() const noexcept;
      void set_to_broken_promise() noexcept;
      void ignore() noexcept;
      static future_state_base current_exception();
      template <typename... U>
      friend future<U...> internal::current_exception_as_future() noexcept;
    };
    struct ready_future_marker {};
    struct exception_future_marker {};
    struct future_for_get_promise_marker {};
    template <typename... T>
    struct future_state
        : public future_state_base,
          private internal::uninitialized_wrapper<std::tuple<T...>> {
      static constexpr bool copy_noexcept =
          std::is_nothrow_copy_constructible<std::tuple<T...>>::value;
      static constexpr bool has_trivial_move_and_destroy = internal::all_true<
          internal::is_trivially_move_constructible_and_destructible<
              T>::value...>::value;
      static_assert(std::is_nothrow_move_constructible<std::tuple<T...>>::value,
                    "Types must be no-throw move constructible");
      static_assert(std::is_nothrow_destructible<std::tuple<T...>>::value,
                    "Types must be no-throw destructible");
      future_state() noexcept {}
      [[gnu::always_inline]] future_state(future_state &&x) noexcept
          : future_state_base(std::move(x)) {
        if (has_trivial_move_and_destroy) {
          memcpy(reinterpret_cast<char *>(&this->uninitialized_get()),
                 &x.uninitialized_get(),
                 internal::used_size<std::tuple<T...>>::value);
        } else if (_u.has_result()) {
          this->uninitialized_set(std::move(x.uninitialized_get()));
          x.uninitialized_get().~tuple();
        }
      }
      future_state &operator=(future_state &&x) noexcept {
        this->~future_state();
        new (this) future_state(std::move(x));
        return *this;
      }
      template <typename... A>
      future_state(ready_future_marker, A &&... a)
          : future_state_base(state::result) {
      }
      future_state(exception_future_marker m, std::exception_ptr &&ex)
          : future_state_base(std::move(ex)) {}
      future_state(exception_future_marker m, future_state_base &&state)
          : future_state_base(std::move(state)) {}
      std::tuple<T...> &&get_value() && noexcept {
        assert(_u.st == state::result);
        return std::move(this->uninitialized_get());
      }
      std::tuple<T...> &&take_value() && noexcept {
        assert(_u.st == state::result);
        _u.st = state::result_unavailable;
        return std::move(this->uninitialized_get());
      }
      template <typename U = std::tuple<T...>>
      std::tuple<T...> &&take() && {
        assert(available());
        if (_u.st >= state::exception_min) {
          std::rethrow_exception(std::move(*this).get_exception());
        }
        if (_u.st >= state::exception_min) {
          std::rethrow_exception(std::move(*this).get_exception());
        }
        return std::move(this->uninitialized_get());
      }
      const std::tuple<T...> &get() const & {
        assert(available());
        if (_u.st >= state::exception_min) {
          std::rethrow_exception(_u.ex);
        }
        return this->uninitialized_get();
      }
      using get0_return_type = typename internal::get0_return_type<T...>::type;
      static get0_return_type get0(std::tuple<T...> &&x) {
        return internal::get0_return_type<T...>::get0(std::move(x));
      }
    };
    template <typename... T> class continuation_base : public task {
    protected:
      future_state<T...> _state;
      using future_type = future<T...>;
      using promise_type = promise<T...>;
    public:
      continuation_base() = default;
      explicit continuation_base(future_state<T...> &&state)
          : _state(std::move(state)) {}
      void set_state(future_state<T...> &&state) { _state = std::move(state); }
      friend class internal::promise_base_with_type<T...>;
      friend class promise<T...>;
      friend class future<T...>;
    };
    template <typename Func, typename... T>
    struct continuation final : continuation_base<T...> {
      continuation(Func &&func, future_state<T...> &&state)
          : continuation_base<T...>(std::move(state)), _func(std::move(func)) {}
      continuation(Func &&func) : _func(std::move(func)) {}
      virtual void run_and_dispose() noexcept override {
        _func(std::move(this->_state));
        delete this;
      }
      Func _func;
    };
    namespace internal {
    template <typename... T>
    future<T...> make_exception_future(future_state_base &&state) noexcept;
    template <typename... T, typename U>
    void set_callback(future<T...> &fut, U *callback) noexcept;
    class future_base;
    class promise_base {
    protected:
      enum class urgent { no, yes };
      future_base *_future = nullptr;
      future_state_base *_state;
      task *_task = nullptr;
      template <urgent Urgent> void make_ready() noexcept;
      template <typename T> void set_exception_impl(T &&val) noexcept;
      void set_exception(future_state_base &&state) noexcept;
      void set_exception(std::exception_ptr &&ex) noexcept;
      void set_exception(const std::exception_ptr &ex) noexcept;
      template <typename Exception>
      std::enable_if_t<!std::is_same<std::remove_reference_t<Exception>,
                                     std::exception_ptr>::value,
                       void>
      set_exception(Exception &&e) noexcept {
        set_exception(make_exception_ptr(std::forward<Exception>(e)));
      }
      friend class future_base;
      template <typename... U> friend class seastar::future;
    };
    template <typename... T>
    class promise_base_with_type : protected internal::promise_base {
    protected:
      future_state<T...> *get_state();
      static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
    public:
      promise_base_with_type(future_state_base *state);
      promise_base_with_type(future<T...> *future);
      promise_base_with_type(promise_base_with_type &&x) noexcept
          : promise_base(std::move(x)) {}
      promise_base_with_type(const promise_base_with_type &) = delete;
      void operator=(const promise_base_with_type &) = delete;
      void set_urgent_state(future_state<T...> &&state) noexcept {
        if (_state) {
          *get_state() = std::move(state);
          make_ready<urgent::yes>();
          make_ready<urgent::no>();
        }
      }
    private:
      template <typename Func> void schedule(Func &&func) noexcept {
        auto tws = new continuation<Func, T...>(std::move(func));
        _state = &tws->_state;
        _task = tws;
      }
      void schedule(continuation_base<T...> *callback) noexcept {
        _state = &callback->_state;
        _task = callback;
      }
      template <typename... U> friend class seastar::future;
      friend struct seastar::future_state<T...>;
    };
    }
    template <typename... T>
    class promise : private internal::promise_base_with_type<T...> {
      future_state<T...> _local_state;
    public:
      promise();
      promise(promise &&x) noexcept;
      promise(const promise &) = delete;
      promise &operator=(promise &&x) noexcept;
      void operator=(const promise &) = delete;
      future<T...> get_future() noexcept;
      template <typename... A> void set_value(A &&... a);
      void set_exception(std::exception_ptr &&ex) noexcept;
      void set_exception(const std::exception_ptr &ex) noexcept;
      template <typename Exception>
      std::enable_if_t<!std::is_same<std::remove_reference_t<Exception>,
                                     std::exception_ptr>::value,
                       void>
      set_exception(Exception &&e) noexcept {
        internal::promise_base::set_exception(std::forward<Exception>(e));
      }
      using internal::promise_base_with_type<T...>::set_urgent_state;
      template <typename... U> friend class future;
    };
    template <typename... T> struct is_future : std::false_type {};
    template <typename T> struct futurize {
      using type = future<T>;
      using promise_type = promise<T>;
      using value_type = std::tuple<T>;
      template <typename Func, typename... FuncArgs>
      static inline type apply(Func &&func,
                               std::tuple<FuncArgs...> &&args) noexcept;
      template <typename Func, typename... FuncArgs>
      static inline type apply(Func &&func, FuncArgs &&... args) noexcept;
      static type convert(T &&value);
      static type convert(type &&value);
      static type from_tuple(value_type &&value);
      static type from_tuple(const value_type &value);
      template <typename Arg> static type make_exception_future(Arg &&arg);
    };
    template <typename... Args> struct futurize<future<Args...>> {
      using type = future<Args...>;
      using promise_type = promise<Args...>;
      using value_type = std::tuple<Args...>;
      template <typename Func, typename... FuncArgs>
      static inline type apply(Func &&func,
                               std::tuple<FuncArgs...> &&args) noexcept;
      template <typename Func, typename... FuncArgs>
      static inline type apply(Func &&func, FuncArgs &&... args) noexcept;
      static inline type convert(type &&value) { return std::move(value); }
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
        future_base(promise_base *promise, future_state_base *state);
        future_base(future_base &&x, future_state_base *state);
        ~future_base() noexcept;
        promise_base *detach_promise() noexcept;
        friend class promise_base;
      };
      template <bool IsVariadic> struct warn_variadic_future {
        void check_deprecation();
      };
      template <> struct warn_variadic_future<true> {
        [[deprecated(
            "Variadic future<> with more than one template parmeter is "
            "deprecated, replace with future<std::tuple<...>>")]] void
        check_deprecation() {}
      };
    }
    template <typename... T>
    class SEASTAR_NODISCARD future
        : private internal::future_base,
          internal::warn_variadic_future<(sizeof...(T) > 1)> {
      future_state<T...> _state;
      static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
    private:
      future(future_for_get_promise_marker m) {}
      future(exception_future_marker m, std::exception_ptr &&ex) noexcept
          : _state(m, std::move(ex)) {}
      future(exception_future_marker m, future_state_base &&state) noexcept
          : _state(m, std::move(state)) {}
      [[gnu::always_inline]] explicit future(
          future_state<T...> &&state) noexcept
          : _state(std::move(state)) {
        this->check_deprecation();
      }
      internal::promise_base_with_type<T...> get_promise() noexcept {
        assert(!_promise);
        return internal::promise_base_with_type<T...>(this);
      }
      internal::promise_base_with_type<T...> *detach_promise() {
        return static_cast<internal::promise_base_with_type<T...> *>(
            future_base::detach_promise());
      }
      template <typename Func> void schedule(Func &&func) noexcept {
        if (_state.available() || !_promise) {
          if (__builtin_expect(!_state.available() && !_promise, false)) {
            _state.set_to_broken_promise();
          }
          ::seastar::schedule(
              new continuation<Func, T...>(std::move(func), std::move(_state)));
        } else {
          assert(_promise);
          detach_promise()->schedule(std::move(func));
          _state._u.st = future_state_base::state::invalid;
        }
      }
      [[gnu::always_inline]] future_state<T...> &&
      get_available_state_ref() noexcept {
        if (_promise) {
          detach_promise();
        }
        return std::move(_state);
      }
      [[gnu::noinline]] future<T...> rethrow_with_nested() {
        if (!failed()) {
          return internal::current_exception_as_future<T...>();
        } else {
          std::nested_exception f_ex;
          try {
            get();
          } catch (...) {
            std::throw_with_nested(f_ex);
          }
          __builtin_unreachable();
        }
      }
      template <typename... U> friend class shared_future;
    public:
      using value_type = std::tuple<T...>;
      using promise_type = promise<T...>;
      [[gnu::always_inline]] future(future &&x) noexcept
          : future_base(std::move(x), &_state), _state(std::move(x._state)) {}
      future(const future &) = delete;
      future &operator=(future &&x) noexcept {
        this->~future();
        new (this) future(std::move(x));
        return *this;
      }
      void operator=(const future &) = delete;
      [[gnu::always_inline]] std::tuple<T...> &&get() {
        if (!_state.available()) {
          do_wait();
        }
        return get_available_state_ref().take();
      }
      [[gnu::always_inline]] std::exception_ptr get_exception() {
        return get_available_state_ref().get_exception();
      }
      typename future_state<T...>::get0_return_type get0() {
        return future_state<T...>::get0(get());
      }
    private:
      class thread_wake_task final : public continuation_base<T...> {
        thread_context *_thread;
        future *_waiting_for;
      public:
        thread_wake_task(thread_context *thread, future *waiting_for)
            : _thread(thread), _waiting_for(waiting_for) {}
        virtual void run_and_dispose() noexcept override {
          _waiting_for->_state = std::move(this->_state);
          thread_impl::switch_in(_thread);
        }
      };
      void do_wait() noexcept {
        if (__builtin_expect(!_promise, false)) {
          _state.set_to_broken_promise();
          return;
        }
        auto thread = thread_impl::get();
        assert(thread);
        thread_wake_task wake_task{thread, this};
        detach_promise()->schedule(
            static_cast<continuation_base<T...> *>(&wake_task));
        thread_impl::switch_out(thread);
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
      GCC6_CONCEPT(requires ::seastar::CanApply<Func, T...>)
      Result then(Func &&func) noexcept {
        return then_impl(std::move(func));
      }
    private:
      template <typename Func,
                typename Result = futurize_t<std::result_of_t<Func(T &&...)>>>
      Result then_impl(Func &&func) noexcept {
        using futurator = futurize<std::result_of_t<Func(T && ...)>>;
        if (available() && !need_preempt()) {
          if (failed()) {
            return futurator::make_exception_future(
                static_cast<future_state_base &&>(get_available_state_ref()));
          } else {
            return futurator::apply(std::forward<Func>(func),
                                    get_available_state_ref().take_value());
          }
        }
        typename futurator::type fut(future_for_get_promise_marker{});
        [&]() noexcept {
          memory::disable_failure_guard dfg;
          schedule([pr = fut.get_promise(), func = std::forward<Func>(func)](
                       future_state<T...> &&state) mutable {
            if (state.failed()) {
              pr.set_exception(
                  static_cast<future_state_base &&>(std::move(state)));
            } else {
              futurator::apply(std::forward<Func>(func),
                               std::move(state).get_value())
                  .forward_to(std::move(pr));
            }
          });
        }();
        return fut;
      }
    public:
      template <typename Func,
                typename FuncResult = std::result_of_t<Func(future)>>
          GCC6_CONCEPT(requires ::seastar::CanApply<Func, future>)
          futurize_t<FuncResult> then_wrapped(Func &&func) & noexcept {
        return then_wrapped_maybe_erase<false, FuncResult>(
            std::forward<Func>(func));
        return then_wrapped_maybe_erase<true, FuncResult>(
            std::forward<Func>(func));
      }
    private:
      template <bool AsSelf, typename FuncResult, typename Func>
      futurize_t<FuncResult> then_wrapped_maybe_erase(Func &&func) noexcept {
        return then_wrapped_common<AsSelf, FuncResult>(
            std::forward<Func>(func));
      }
      template <bool AsSelf, typename FuncResult, typename Func>
      futurize_t<FuncResult> then_wrapped_common(Func &&func) noexcept {
        using futurator = futurize<FuncResult>;
        if (available() && !need_preempt()) {
          if (AsSelf) {
            if (_promise) {
              detach_promise();
            }
            return futurator::apply(std::forward<Func>(func), std::move(*this));
          } else {
            return futurator::apply(std::forward<Func>(func),
                                    future(get_available_state_ref()));
          }
        }
        typename futurator::type fut(future_for_get_promise_marker{});
        [&]() noexcept {
          memory::disable_failure_guard dfg;
          schedule([pr = fut.get_promise(), func = std::forward<Func>(func)](
                       future_state<T...> &&state) mutable {
            futurator::apply(std::forward<Func>(func), future(std::move(state)))
                .forward_to(std::move(pr));
          });
        }();
        return fut;
      }
      void forward_to(internal::promise_base_with_type<T...> &&pr) noexcept {
      }
    public:
      void forward_to(promise<T...> &&pr) noexcept {
        if (_state.available()) {
          pr.set_urgent_state(std::move(_state));
        } else if (&pr._local_state != pr._state) {
          *detach_promise() = std::move(pr);
        }
      }
      template <typename Func>
      GCC6_CONCEPT(requires ::seastar::CanApply<Func>)
      future<T...> finally(Func &&func) noexcept {
        return then_wrapped(
            finally_body<Func, is_future<std::result_of_t<Func()>>::value>(
                std::forward<Func>(func)));
      }
      template <typename Func, bool FuncReturnsFuture> struct finally_body;
      template <typename Func> struct finally_body<Func, true> {
        Func _func;
        finally_body(Func &&func) : _func(std::forward<Func>(func)) {}
        future<T...> operator()(future<T...> &&result) {
          using futurator = futurize<std::result_of_t<Func()>>;
          return futurator::apply(_func).then_wrapped(
              [result = std::move(result)](auto f_res) mutable {
                if (!f_res.failed()) {
                  return std::move(result);
                } else {
                  try {
                    f_res.get();
                  } catch (...) {
                    return result.rethrow_with_nested();
                  }
                  __builtin_unreachable();
                }
              });
        }
      };
      template <typename Func> struct finally_body<Func, false> {
        Func _func;
        finally_body(Func &&func) : _func(std::forward<Func>(func)) {}
        future<T...> operator()(future<T...> &&result) {
          try {
            _func();
            return std::move(result);
          } catch (...) {
            return result.rethrow_with_nested();
          }
        };
      };
      future<> or_terminate() noexcept {
        return then_wrapped([](auto &&f) {
          try {
            f.get();
          } catch (...) {
            engine_exit(std::current_exception());
          }
        });
      }
      future<> discard_result() noexcept {
        return then([](T &&...) {});
      }
      template <typename Func>
      future<T...> handle_exception(Func &&func) noexcept {
        using func_ret = std::result_of_t<Func(std::exception_ptr)>;
        return then_wrapped([func = std::forward<Func>(func)](
                                auto &&fut) mutable -> future<T...> {
          if (!fut.failed()) {
            return make_ready_future<T...>(fut.get());
          } else {
            return futurize<func_ret>::apply(func, fut.get_exception());
          }
        });
      }
      template <typename Func>
      future<T...> handle_exception_type(Func &&func) noexcept {
        using trait = function_traits<Func>;
        static_assert(trait::arity == 1, "func can take only one parameter");
        using ex_type = typename trait::template arg<0>::type;
        using func_ret = typename trait::return_type;
        return then_wrapped([func = std::forward<Func>(func)](
                                auto &&fut) mutable -> future<T...> {
          try {
            return make_ready_future<T...>(fut.get());
          } catch (ex_type &ex) {
            return futurize<func_ret>::apply(func, ex);
          }
        });
      }
      void ignore_ready_future() noexcept { _state.ignore(); }
    private:
      void set_callback(continuation_base<T...> *callback) noexcept {
        if (_state.available()) {
          callback->set_state(get_available_state_ref());
          ::seastar::schedule(callback);
        } else {
          assert(_promise);
          detach_promise()->schedule(callback);
        }
      }
      template <typename... U> friend class future;
      template <typename... U> friend class promise;
      template <typename... U> friend class internal::promise_base_with_type;
      template <typename... U, typename... A>
      friend future<U...> make_ready_future(A &&... value);
      template <typename... U>
      friend future<U...>
      internal::make_exception_future(future_state_base &&state) noexcept;
      template <typename... U, typename V>
      friend void internal::set_callback(future<U...> &, V *) noexcept;
    };
    template <typename T>
    template <typename Func, typename... FuncArgs>
    typename futurize<T>::type futurize<T>::apply(
        Func && func, std::tuple<FuncArgs...> && args) noexcept {
      try {
        return convert(
            ::seastar::apply(std::forward<Func>(func), std::move(args)));
      } catch (...) {
        return internal::current_exception_as_future<T>();
      }
    }
    template <typename... Args>
    template <typename Func, typename... FuncArgs>
    typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(
        Func && func, std::tuple<FuncArgs...> && args) noexcept {
      try {
        return ::seastar::apply(std::forward<Func>(func), std::move(args));
      } catch (...) {
        return internal::current_exception_as_future<Args...>();
      }
    }
    template <typename Tag> class bool_class {
      bool _value;
    public:
      static const bool_class yes;
      static const bool_class no;
      constexpr bool_class();
      constexpr explicit bool_class(bool v) noexcept : _value(v) {}
    };
   }
    namespace seastar {
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
        _pr.set_urgent_state(std::move(this->_state));
        delete this;
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
        return fut;
      }
      auto ret = task->get_future();
      internal::set_callback(fut, task.release());
      return ret;
    }
   }
#include <bitset>
#include <boost/intrusive/list.hpp>
 namespace seastar {
    template <typename Timer, boost::intrusive::list_member_hook<> Timer::*link>
    class timer_set {
    };
   };
    namespace seastar {
    using steady_clock_type = std::chrono::steady_clock;
    template <typename Clock = steady_clock_type> class timer {
    public:
    };
    struct stop_iteration_tag {};
    using stop_iteration = bool_class<stop_iteration_tag>;
 }
   namespace seastar {
    static constexpr size_t cache_line_size = 64;
   }
    namespace seastar {
    class lowres_clock;
    class lowres_system_clock;
    class lowres_clock_impl final {
    public:
      using base_steady_clock = std::chrono::steady_clock;
      using base_system_clock = std::chrono::system_clock;
      using period = std::ratio<1, 1000>;
      using steady_rep = base_steady_clock::rep;
      using steady_duration = std::chrono::duration<steady_rep, period>;
      using steady_time_point =
          std::chrono::time_point<lowres_clock, steady_duration>;
      static constexpr std::chrono::milliseconds _granularity{10};
      timer<> _timer{};
    };
    class lowres_clock final {
    public:
      using rep = lowres_clock_impl::steady_rep;
      using period = lowres_clock_impl::period;
      using duration = lowres_clock_impl::steady_duration;
      using time_point = lowres_clock_impl::steady_time_point;
      static constexpr bool is_steady = true;
    };
   }
    namespace seastar {
    class manual_clock {
    public:
      using rep = int64_t;
      using period = std::chrono::nanoseconds::period;
      using duration = std::chrono::duration<rep, period>;
      using time_point = std::chrono::time_point<manual_clock, duration>;
    private:
      static std::atomic<rep> _now;
    public:
    };
   }
    namespace seastar {
    namespace metrics {
    namespace impl {
    class metric_groups_def;
    struct metric_definition_impl;
    class metric_groups_impl;
    };
    };
}
;
namespace seastar {
    namespace alien {
    class message_queue;
    }
}
namespace std {
    template <> struct hash<::sockaddr_in> {};
}
namespace seastar {
    namespace internal {}
    class reactor {
      ;
      ;
    public:
    };  }
    class column_set { };
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
        send_timestamp,
        send_expiry,
        reversed,
        distinct,
        collections_as_maps,
        send_ttl,
        allow_short_read,
        with_digest,
        bypass_cache,
        always_return_static_content,
      };
      using option_set = enum_set<super_enum<
          option, option::send_clustering_key, option::send_partition_key,
          option::send_timestamp, option::send_expiry, option::reversed,
          option::distinct, option::collections_as_maps, option::send_ttl,
          option::allow_short_read, option::with_digest, option::bypass_cache,
          option::always_return_static_content>>;
    public:
      partition_slice(
          clustering_row_ranges row_ranges, column_id_vector static_columns,
          column_id_vector regular_columns, option_set options,
          std::unique_ptr<specific_ranges> specific_ranges = nullptr,
          cql_serialization_format = cql_serialization_format::internal(),
          uint32_t partition_row_limit = max_rows);
      partition_slice(clustering_row_ranges ranges, const schema &schema,
                      const column_set &mask, option_set options);
      partition_slice(const partition_slice &);
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
    private:
      future<mutation>
      do_apply_counter_update(column_family &cf, const frozen_mutation &fm,
                              schema_ptr m_schema,
                              db::timeout_clock::time_point timeout,
                              tracing::trace_state_ptr trace_state);
    public:
    };
    namespace tracing {
    class trace_state_ptr final {
    public:
      trace_state_ptr();
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
                      return make_ready_future<stop_iteration>(
                          stop_iteration::yes);
                    }
                    return futurator::apply(c, std::move(*mo));
                  });
            });
          });
    }
    class frozen_mutation final {
    public:
      frozen_mutation(const mutation &m);
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
                  locks = std::move(lcs);
                  return counter_write_query(schema_ptr(), mutation_source(),
                                             m.decorated_key(), slice, nullptr)
                      .then([this, &cf, &m, timeout](auto mopt) {
                        return std::move(m);
                      });
                });
          });
    }