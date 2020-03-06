#include <seastar/core/sstring.hh>
  using namespace seastar;
#include <seastar/util/gcc6-concepts.hh>
GCC6_CONCEPT(template <typename H> concept bool Hasher() {
    return requires(H & h, const char *ptr, size_t size) {     { h.update(ptr, size) }     ->void;   };
  }
   ) template <typename T, typename Enable = void> struct appending_hash;
#include <seastar/core/shared_ptr.hh>
using column_count_type = uint32_t;
    using column_id = column_count_type;
    class schema;
    using schema_ptr = seastar::lw_shared_ptr<const schema>;
     GCC6_CONCEPT(template <typename T> concept bool HasTriCompare =                  requires(const T &t) {
                     { t.compare(t) }
                     ->int;
                   }
    &&                  std::is_same<std::result_of_t<decltype (&T::compare)(T, T)>,                               int>::value;
   ) template <typename T> class with_relational_operators {
  private:   template <typename U>   GCC6_CONCEPT(requires HasTriCompare<U>)   int do_compare(const U &t) const;
  };
#include <seastar/util/optimized_optional.hh>
   using bytes = basic_sstring<int8_t, uint32_t, 31, false>;
 class UTFDataFormatException {
 };
    template <typename CharOutputIterator> GCC6_CONCEPT(requires requires(CharOutputIterator it) {
  *it++ = 'a';
  *it++ = 'a';
  }
   ) inline void serialize_string(CharOutputIterator &out, const char *s) {
    auto len = strlen(s);
    out = std::copy_n(s, len, out);
  }
    namespace utils {
  class UUID { private:   int64_t most_sig_bits;   int64_t least_sig_bits; public:                           ; };
  }
     class abstract_type;
    using cql_protocol_version_type = uint8_t;
    class cql_serialization_format {
    cql_protocol_version_type _version;
  public:   static constexpr cql_protocol_version_type latest_version = 4;
    explicit cql_serialization_format(cql_protocol_version_type version)       : _version(version) {}
    static cql_serialization_format latest() {     return cql_serialization_format{latest_version};   }
    static cql_serialization_format internal() { return latest(); }
  };
    namespace utils {
  template <typename T, size_t N> class small_vector {   static_assert(N > 0);   static_assert(std::is_nothrow_move_constructible_v<T>);   static_assert(std::is_nothrow_move_assignable_v<T>);   static_assert(std::is_nothrow_destructible_v<T>); private:   T *_begin;   T *_end;   T *_capacity_end;   union internal {     internal();     ~internal();     T storage[N];   };   internal _internal; private:   bool uses_internal_storage() const noexcept {     return _begin == _internal.storage;   }   [[gnu::cold]] [[gnu::noinline]] void expand(size_t new_capacity) {     auto ptr =         static_cast<T *>(::aligned_alloc(alignof(T), new_capacity * sizeof(T)));     if (!ptr) {       throw std::bad_alloc();     }     auto n_end = std::uninitialized_move(begin(), end(), ptr);     std::destroy(begin(), end());     if (!uses_internal_storage()) {       std::free(_begin);     }     _begin = ptr;     _end = n_end;     _capacity_end = ptr + new_capacity;   }   [[gnu::cold]] [[gnu::noinline]] void   slow_copy_assignment(const small_vector &other) {     auto ptr =         static_cast<T *>(::aligned_alloc(alignof(T), other.size() * sizeof(T)));     if (!ptr) {       throw std::bad_alloc();     }     auto n_end = ptr;     try {       n_end = std::uninitialized_copy(other.begin(), other.end(), n_end);     } catch (...) {       std::free(ptr);       throw;     }     std::destroy(begin(), end());     if (!uses_internal_storage()) {       std::free(_begin);     }     _begin = ptr;     _end = n_end;     _capacity_end = n_end;   }   void reserve_at_least(size_t n) {     if (__builtin_expect(_begin + n > _capacity_end, false)) {       expand(std::max(n, capacity() * 2));     }   }   [[noreturn]] [[gnu::cold]] [[gnu::noinline]] void throw_out_of_range() {     throw std::out_of_range("out of range small vector access");   } public:   using value_type = T;   using pointer = T *;   using const_pointer = const T *;   using reference = T &;   using const_reference = const T &;   using iterator = T *;   using const_iterator = const T *;   using reverse_iterator = std::reverse_iterator<iterator>;   using const_reverse_iterator = std::reverse_iterator<const_iterator>;   small_vector() noexcept       : _begin(_internal.storage), _end(_begin), _capacity_end(_begin + N) {}   template <typename InputIterator>   small_vector(InputIterator first, InputIterator last) : small_vector() {     if constexpr (std::is_base_of_v<std::forward_iterator_tag,                                     typename std::iterator_traits<                                         InputIterator>::iterator_category>) {       reserve(std::distance(first, last));       _end = std::uninitialized_copy(first, last, _end);     } else {       std::copy(first, last, std::back_inserter(*this));     }   }   small_vector(std::initializer_list<T> list)       : small_vector(list.begin(), list.end()) {}   small_vector(small_vector &&other) noexcept {     if (other.uses_internal_storage()) {       _begin = _internal.storage;       _capacity_end = _begin + N;       if constexpr (std::is_trivially_copyable_v<T>) {         std::memcpy(_internal.storage, other._internal.storage, N * sizeof(T));         _end = _begin + other.size();       } else {         _end = _begin;         for (auto &e : other) {           new (_end++) T(std::move(e));           e.~T();         }       }       other._end = other._internal.storage;     } else {       _begin = std::exchange(other._begin, other._internal.storage);       _end = std::exchange(other._end, other._internal.storage);       _capacity_end =           std::exchange(other._capacity_end, other._internal.storage + N);     }   }   small_vector(const small_vector &other) noexcept : small_vector() {     reserve(other.size());     _end = std::uninitialized_copy(other.begin(), other.end(), _end);   }   small_vector &operator=(small_vector &&other) noexcept {     clear();     if (other.uses_internal_storage()) {       if (__builtin_expect(!uses_internal_storage(), false)) {         std::free(_begin);         _begin = _internal.storage;       }       _capacity_end = _begin + N;       if constexpr (std::is_trivially_copyable_v<T>) {         std::memcpy(_internal.storage, other._internal.storage, N * sizeof(T));         _end = _begin + other.size();       } else {         _end = _begin;         for (auto &e : other) {           new (_end++) T(std::move(e));           e.~T();         }       }       other._end = other._internal.storage;     } else {       if (__builtin_expect(!uses_internal_storage(), false)) {         std::free(_begin);       }       _begin = std::exchange(other._begin, other._internal.storage);       _end = std::exchange(other._end, other._internal.storage);       _capacity_end =           std::exchange(other._capacity_end, other._internal.storage + N);     }     return *this;   }   small_vector &operator=(const small_vector &other) {     if constexpr (std::is_nothrow_copy_constructible_v<T>) {       if (capacity() >= other.size()) {         clear();         _end = std::uninitialized_copy(other.begin(), other.end(), _end);         return *this;       }     }     slow_copy_assignment(other);     return *this;   }   ~small_vector() {     clear();     if (__builtin_expect(!uses_internal_storage(), false)) {       std::free(_begin);     }   }   void reserve(size_t n) {     if (__builtin_expect(_begin + n > _capacity_end, false)) {       expand(n);     }   }   void clear() noexcept {     std::destroy(_begin, _end);     _end = _begin;   }   iterator begin() noexcept { return _begin; }   const_iterator begin() const noexcept { return _begin; }   const_iterator cbegin() const noexcept { return _begin; }   iterator end() noexcept { return _end; }   const_iterator end() const noexcept { return _end; }   const_iterator cend() const noexcept { return _end; }   reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }   const_reverse_iterator rbegin() const noexcept {     return const_reverse_iterator(end());   }   const_reverse_iterator crbegin() const noexcept {     return const_reverse_iterator(end());   }   reverse_iterator rend() noexcept { return reverse_iterator(begin()); }   const_reverse_iterator rend() const noexcept {     return const_reverse_iterator(begin());   }   const_reverse_iterator crend() const noexcept {     return const_reverse_iterator(begin());   }   T *data() noexcept { return _begin; }   const T *data() const noexcept { return _begin; }   T &front() noexcept { return *begin(); }   const T &front() const noexcept { return *begin(); }   T &back() noexcept { return end()[-1]; }   const T &back() const noexcept { return end()[-1]; }   T &operator[](size_t idx) noexcept { return data()[idx]; }   const T &operator[](size_t idx) const noexcept { return data()[idx]; }   T &at(size_t idx) {     if (__builtin_expect(idx >= size(), false)) {       throw_out_of_range();     }     return operator[](idx);   }   const T &at(size_t idx) const {     if (__builtin_expect(idx >= size(), false)) {       throw_out_of_range();     }     return operator[](idx);   }   bool empty() const noexcept { return _begin == _end; }   size_t size() const noexcept { return _end - _begin; }   size_t capacity() const noexcept { return _capacity_end - _begin; }   template <typename... Args> T &emplace_back(Args &&... args) {     if (__builtin_expect(_end == _capacity_end, false)) {       expand(std::max<size_t>(capacity() * 2, 1));     }     auto &ref = *new (_end) T(std::forward<Args>(args)...);     ++_end;     return ref;   }   T &push_back(const T &value) { return emplace_back(value); }   T &push_back(T &&value) { return emplace_back(std::move(value)); }   template <typename InputIterator>   iterator insert(const_iterator cpos, InputIterator first,                   InputIterator last) {     if constexpr (std::is_base_of_v<std::forward_iterator_tag,                                     typename std::iterator_traits<                                         InputIterator>::iterator_category>) {       if (first == last) {         return const_cast<iterator>(cpos);       }       auto idx = cpos - _begin;       auto new_count = std::distance(first, last);       reserve_at_least(size() + new_count);       auto pos = _begin + idx;       auto after = std::distance(pos, end());       if (__builtin_expect(pos == end(), true)) {         _end = std::uninitialized_copy(first, last, end());         return pos;       } else if (after > new_count) {         std::uninitialized_move(end() - new_count, end(), end());         std::move_backward(pos, end() - new_count, end());         try {           std::copy(first, last, pos);         } catch (...) {           std::move(pos + new_count, end() + new_count, pos);           std::destroy(end(), end() + new_count);           throw;         }       } else {         std::uninitialized_move(pos, end(), pos + new_count);         auto mid = std::next(first, after);         try {           std::uninitialized_copy(mid, last, end());           try {             std::copy(first, mid, pos);           } catch (...) {             std::destroy(end(), pos + new_count);             throw;           }         } catch (...) {           std::move(pos + new_count, end() + new_count, pos);           std::destroy(pos + new_count, end() + new_count);           throw;         }       }       _end += new_count;       return pos;     } else {       auto start = cpos - _begin;       auto idx = start;       while (first != last) {         try {           insert(begin() + idx, *first);           ++first;           ++idx;         } catch (...) {           erase(begin() + start, begin() + idx);           throw;         }       }       return begin() + idx;     }   }   template <typename... Args>   iterator emplace(const_iterator cpos, Args &&... args) {     auto idx = cpos - _begin;     reserve_at_least(size() + 1);     auto pos = _begin + idx;     if (pos != _end) {       new (_end) T(std::move(_end[-1]));       std::move_backward(pos, _end - 1, _end);       pos->~T();     }     try {       new (pos) T(std::forward<Args>(args)...);     } catch (...) {       if (pos != _end) {         new (pos) T(std::move(pos[1]));         std::move(pos + 2, _end + 1, pos + 1);         _end->~T();       }       throw;     }     _end++;     return pos;   }   iterator insert(const_iterator cpos, const T &obj) {     return emplace(cpos, obj);   }   iterator insert(const_iterator cpos, T &&obj) {     return emplace(cpos, std::move(obj));   }   void resize(size_t n) {     if (n < size()) {       erase(end() - (size() - n), end());     } else if (n > size()) {       reserve_at_least(n);       _end = std::uninitialized_value_construct_n(_end, n - size());     }   }   void resize(size_t n, const T &value) {     if (n < size()) {       erase(end() - (size() - n), end());     } else if (n > size()) {       reserve_at_least(n);       auto nend = _begin + n;       std::uninitialized_fill(_end, nend, value);       _end = nend;     }   }   void pop_back() noexcept { (--_end)->~T(); }   iterator erase(const_iterator cit) noexcept { return erase(cit, cit + 1); }   iterator erase(const_iterator cfirst, const_iterator clast) noexcept {     auto first = const_cast<iterator>(cfirst);     auto last = const_cast<iterator>(clast);     std::move(last, end(), first);     auto nend = _end - (clast - cfirst);     std::destroy(nend, _end);     _end = nend;     return first;   }   void swap(small_vector &other) noexcept { std::swap(*this, other); }   bool operator==(const small_vector &other) const noexcept {     return size() == other.size() && std::equal(_begin, _end, other.begin());   }   bool operator!=(const small_vector &other) const noexcept {     return !(*this == other);   } };
  }
#include <seastar/net/ip.hh>
  using data_type = shared_ptr<const abstract_type>;
#include <boost/range/adaptor/transformed.hpp>
   class column_set {
  };
    class schema final : public enable_lw_shared_from_this<schema> {
    struct column {     bytes name;     data_type type;   };
  public:   schema(std::optional<utils::UUID> id, std::string_view ks_name,          std::string_view cf_name, std::vector<column> partition_key,          std::vector<column> clustering_key,          std::vector<column> regular_columns,          std::vector<column> static_columns, data_type regular_column_name_type,          std::string_view comment = {}
 );
    ~schema();
  public: };
    struct blob_storage {
    struct [[gnu::packed]] ref_type {     blob_storage *ptr;     ref_type() {}     ref_type(blob_storage * ptr) : ptr(ptr) {}     operator blob_storage *() const { return ptr; }     blob_storage *operator->() const { return ptr; }     blob_storage &operator*() const { return *ptr; }   };
  }
    __attribute__((packed));
    class table;
    using column_family = table;
    class clustering_key_prefix;
    template <typename T> class wrapping_range {
  };
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
  class specific_ranges {};
  constexpr auto max_rows = std::numeric_limits<uint32_t>::max();
  class partition_slice { public:   enum class option {     send_clustering_key,     send_partition_key,     send_timestamp,     send_expiry,     reversed,     distinct,     collections_as_maps,     send_ttl,     allow_short_read,     with_digest,     bypass_cache,     always_return_static_content,   };   using option_set = enum_set<super_enum<       option, option::send_clustering_key, option::send_partition_key,       option::send_timestamp, option::send_expiry, option::reversed,       option::distinct, option::collections_as_maps, option::send_ttl,       option::allow_short_read, option::with_digest, option::bypass_cache,       option::always_return_static_content>>; public:   partition_slice(       clustering_row_ranges row_ranges, column_id_vector static_columns,       column_id_vector regular_columns, option_set options,       std::unique_ptr<specific_ranges> specific_ranges = nullptr,       cql_serialization_format = cql_serialization_format::internal(),       uint32_t partition_row_limit = max_rows);   partition_slice(clustering_row_ranges ranges, const schema &schema,                   const column_set &mask, option_set options);   partition_slice(const partition_slice &); };
  }
    class range_tombstone final {
  };
    namespace db {
  using timeout_clock = seastar::lowres_clock;
  }
     class clustering_row {
  };
    class static_row {
  };
    class partition_start final {
  public: };
    class partition_end final {
  public: };
    GCC6_CONCEPT(template <typename T, typename ReturnType>              concept bool MutationFragmentConsumer() {
                 return requires(T t, static_row sr, clustering_row cr,                                range_tombstone rt, partition_start ph,                                partition_end pe) {                  { t.consume(std::move(sr)) }                  ->ReturnType;                  { t.consume(std::move(cr)) }                  ->ReturnType;                  { t.consume(std::move(rt)) }                  ->ReturnType;                  { t.consume(std::move(ph)) }                  ->ReturnType;                  { t.consume(std::move(pe)) }                  ->ReturnType;                };
               }
   ) class mutation_fragment {
  };
    GCC6_CONCEPT(template <typename F> concept bool StreamedMutationTranformer() {
    return requires(F f, mutation_fragment mf, schema_ptr s) {     { f(std::move(mf)) }     ->mutation_fragment;     { f(s) }     ->schema_ptr;   };
  }
   ) class mutation final {
    mutation() = default;
  public:   const dht::decorated_key &decorated_key() const;
  };
    using mutation_opt = optimized_optional<mutation>;
    class flat_mutation_reader;
    future<mutation_opt> read_mutation_from_flat_mutation_reader(flat_mutation_reader &reader,                                         db::timeout_clock::time_point timeout);
    class locked_cell;
    class frozen_mutation;
    class table {
  public:   future<std::vector<locked_cell>>   lock_counter_cells(const mutation &m, db::timeout_clock::time_point timeout);
  };
    class database {
  private:   future<mutation>   do_apply_counter_update(column_family &cf, const frozen_mutation &fm,                           schema_ptr m_schema,                           db::timeout_clock::time_point timeout,                           tracing::trace_state_ptr trace_state);
  public: };
     namespace tracing {
  class trace_state_ptr final { public:   trace_state_ptr();   trace_state_ptr(nullptr_t); };
  }
    template <typename Consumer> inline future<> consume_partitions(flat_mutation_reader &reader,                                    Consumer consumer,                                    db::timeout_clock::time_point timeout) {
    using futurator = futurize<std::result_of_t<Consumer(mutation &&)>>;
    return do_with(       std::move(consumer), [&reader, timeout](Consumer &c) -> future<> {         return repeat([&reader, &c, timeout]() {           return read_mutation_from_flat_mutation_reader(reader, timeout)               .then([&c](mutation_opt &&mo) -> future<stop_iteration> {                 if (!mo) {                   return make_ready_future<stop_iteration>(stop_iteration::yes);                 }                 return futurator::apply(c, std::move(*mo));               });         });       }
 );
  }
    class frozen_mutation final {
  public:   frozen_mutation(const mutation &m);
    mutation unfreeze(schema_ptr s) const;
  };
    class mutation_source {
  };
    future<mutation_opt> counter_write_query(schema_ptr, const mutation_source &,                                          const dht::decorated_key &dk,                                          const query::partition_slice &slice,                                          tracing::trace_state_ptr trace_ptr);
    class locked_cell {
 };
    future<mutation> database::do_apply_counter_update(column_family &cf, const frozen_mutation &fm,                                   schema_ptr m_schema,                                   db::timeout_clock::time_point timeout,                                   tracing::trace_state_ptr trace_state) {
    auto m = fm.unfreeze(m_schema);
    query::column_id_vector static_columns;
    query::clustering_row_ranges cr_ranges;
    query::column_id_vector regular_columns;
    auto slice = query::partition_slice(       std::move(cr_ranges), std::move(static_columns),       std::move(regular_columns), {}
 , {}
 , cql_serialization_format::internal(),       query::max_rows);
    return do_with(       std::move(slice), std::move(m), std::vector<locked_cell>(),       [this, &cf, timeout](const query::partition_slice &slice, mutation &m,                            std::vector<locked_cell> &locks) mutable {         return cf.lock_counter_cells(m, timeout)             .then([&, timeout, this](std::vector<locked_cell> lcs) mutable {               locks = std::move(lcs);               return counter_write_query(schema_ptr(), mutation_source(),                                          m.decorated_key(), slice, nullptr)                   .then([this, &cf, &m, timeout](auto mopt) {                     return std::move(m);                   });             });       }
 );
  }
