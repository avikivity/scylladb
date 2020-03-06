#include <seastar/core/sstring.hh>
  using namespace seastar;
#include <seastar/util/gcc6-concepts.hh>
GCC6_CONCEPT(template <typename H> concept bool Hasher() {
   return requires(H & h, const char *ptr, size_t size) {     { h.update(ptr, size) }     ->void;   };
 }
 ) class hasher {
 public:   virtual ~hasher() = default;
   virtual void update(const char *ptr, size_t size) = 0;
 };
  GCC6_CONCEPT(static_assert(Hasher<hasher>());
 ) template <typename T, typename Enable = void> struct appending_hash;
#include <seastar/core/lowres_clock.hh>
class gc_clock final {
 public:   using base = seastar::lowres_system_clock;
   using rep = int64_t;
   using period = std::ratio<1, 1>;
   using duration = std::chrono::duration<rep, period>;
 };
#include <seastar/core/shared_ptr.hh>
using column_count_type = uint32_t;
  using column_id = column_count_type;
  class schema;
  class schema_extension;
  using schema_ptr = seastar::lw_shared_ptr<const schema>;
  namespace api {
 using timestamp_type = int64_t;
 class timestamp_clock final {   using base = std::chrono::system_clock; public:   using rep = timestamp_type;   using duration = std::chrono::microseconds;   using period = typename duration::period;   using time_point = std::chrono::time_point<timestamp_clock, duration>;   static constexpr bool is_steady = base::is_steady; };
 }
   GCC6_CONCEPT(template <typename T> concept bool HasTriCompare =                  requires(const T &t) {
                    { t.compare(t) }
                    ->int;
                  }
  &&                  std::is_same<std::result_of_t<decltype (&T::compare)(T, T)>,                               int>::value;
 ) template <typename T> class with_relational_operators {
 private:   template <typename U>   GCC6_CONCEPT(requires HasTriCompare<U>)   int do_compare(const U &t) const;
 };
  struct tombstone final : public with_relational_operators<tombstone> {
   api::timestamp_type timestamp;
 };
#include <seastar/util/optimized_optional.hh>
template <typename CharT> class basic_mutable_view {
 };
  using bytes = basic_sstring<int8_t, uint32_t, 31, false>;
  using bytes_view = std::basic_string_view<int8_t>;
  using bytes_mutable_view = basic_mutable_view<bytes_view::value_type>;
  using sstring_view = std::string_view;
  namespace std {
 std::ostream &operator<<(std::ostream &os, const bytes_view &b);
 }
#include <seastar/net/byteorder.hh>
class UTFDataFormatException {
};
  template <typename CharOutputIterator> GCC6_CONCEPT(requires requires(CharOutputIterator it) {
 *it++ = 'a';
 }
 ) inline void serialize_string(CharOutputIterator &out, const sstring &s) {
   out = std::copy(s.begin(), s.end(), out);
 }
  template <typename CharOutputIterator> GCC6_CONCEPT(requires requires(CharOutputIterator it) {
 *it++ = 'a';
 }
 ) inline void serialize_string(CharOutputIterator &out, const char *s) {
   auto len = strlen(s);
   if (len > std::numeric_limits<uint16_t>::max()) {     throw UTFDataFormatException();   }
   serialize_int16(out, len);
   out = std::copy_n(s, len, out);
 }
  template <typename T, typename CharOutputIterator> static inline void write(CharOutputIterator &out, const T &val) {
   auto v = net::ntoh(val);
   out = std::copy_n(reinterpret_cast<char *>(&v), sizeof(v), out);
 }
  namespace utils {
 class UUID { private:   int64_t most_sig_bits;   int64_t least_sig_bits; public:   UUID() : most_sig_bits(0), least_sig_bits(0) {}   UUID(int64_t most_sig_bits, int64_t least_sig_bits)       : most_sig_bits(most_sig_bits), least_sig_bits(least_sig_bits) {}   explicit UUID(const sstring &uuid_string) : UUID(sstring_view(uuid_string)) {}   explicit UUID(const char *s) : UUID(sstring_view(s)) {}   explicit UUID(sstring_view uuid_string);   int64_t get_most_significant_bits() const { return most_sig_bits; }   int64_t get_least_significant_bits() const { return least_sig_bits; }   int version() const;   ; };
 }
#include <seastar/util/log.hh>
namespace logging {
 using log_level = seastar::log_level;
 using logger = seastar::logger;
 using registry = seastar::logger_registry;
 using settings = seastar::logging_settings;
 using seastar::level_name;
 using seastar::pretty_type_name;
 }
  GCC6_CONCEPT(     template <typename T> concept bool FragmentRange =         requires(T range) {
           typename T::fragment_type;
           requires std::is_same_v<typename T::fragment_type, bytes_view> ||               std::is_same_v<typename T::fragment_type, bytes_mutable_view>;
           { *range.begin() }
           ->typename T::fragment_type;
           { *range.end() }
           ->typename T::fragment_type;
           { range.size_bytes() }
           ->size_t;
           { range.empty() }
           ->bool;
         };
 ) template <typename T, typename = void> struct is_fragment_range : std::false_type {
};
class bytes_ostream {
 public:   using size_type = bytes::size_type;
   using value_type = bytes::value_type;
   struct chunk {     std::unique_ptr<chunk> next;     ~chunk() {       auto p = std::move(next);       while (p) {         auto p_next = std::move(p->next);         p = std::move(p_next);       }     }     size_type offset;     size_type size;     value_type data[0];     void operator delete(void *ptr) { free(ptr); }   };
 private:   std::unique_ptr<chunk> _begin;
 public:   class fragment_iterator       : public std::iterator<std::input_iterator_tag, bytes_view> {     chunk *_current = nullptr;   public:     fragment_iterator() = default;     fragment_iterator(chunk *current) : _current(current) {}     fragment_iterator(const fragment_iterator &) = default;     fragment_iterator &operator=(const fragment_iterator &) = default;     bytes_view operator*() const;     bytes_view operator->() const;     fragment_iterator &operator++();     fragment_iterator operator++(int);     bool operator==(const fragment_iterator &other) const;     bool operator!=(const fragment_iterator &other) const;   };
   fragment_iterator begin() const { return {_begin.get()}; }
   fragment_iterator end() const { return {nullptr}; }
   boost::iterator_range<fragment_iterator> fragments() const {     return {begin(), end()};   }
 };
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
   namespace utils {
 }
  class marshal_exception : public std::exception {
 };
#include <seastar/net/ip.hh>
#include <seastar/util/backtrace.hh>
  using data_type = shared_ptr<const abstract_type>;
  class abstract_type : public enable_shared_from_this<abstract_type> {
                                  public:                        bool is_reversed() const;
 };
#include <boost/range/adaptor/transformed.hpp>
  enum class allow_prefixes {
 no, yes };
  template <allow_prefixes AllowPrefixes = allow_prefixes::no> class compound_type final {
 private:   const std::vector<data_type> _types;
   const bool _byte_order_equal;
   const bool _byte_order_comparable;
   const bool _is_reversed;
   compound_type(std::vector<data_type> types)       : _types(std::move(types)),         _byte_order_equal(             std::all_of(_types.begin(), _types.end(),                         [](auto t) { return t->is_byte_order_equal(); }
)),         _byte_order_comparable(false),         _is_reversed(_types.size() == 1 && _types[0]->is_reversed()) {}
 };
  class column_set {
 };
  enum class column_kind {
   partition_key,   clustering_key,   static_column,   regular_column };
  enum class cf_type : uint8_t {
   standard,   super, };
  struct speculative_retry {
 };
  class index_metadata final {
};
  class thrift_schema {
 };
  class column_mapping_entry {
 public: };
  class raw_view_info final {
 public: };
  class v3_columns {
 public: public: };
  class schema final : public enable_lw_shared_from_this<schema> {
   struct column {     bytes name;     data_type type;   };
 public:   schema(std::optional<utils::UUID> id, std::string_view ks_name,          std::string_view cf_name, std::vector<column> partition_key,          std::vector<column> clustering_key,          std::vector<column> regular_columns,          std::vector<column> static_columns, data_type regular_column_name_type,          std::string_view comment = {}
);
   ~schema();
 public: };
class migrate_fn_type {
 };
  struct blob_storage {
   struct [[gnu::packed]] ref_type {     blob_storage *ptr;     ref_type() {}     ref_type(blob_storage * ptr) : ptr(ptr) {}     operator blob_storage *() const { return ptr; }     blob_storage *operator->() const { return ptr; }     blob_storage &operator*() const { return *ptr; }   };
 }
  __attribute__((packed));
  class managed_bytes {
 private:   static constexpr size_t max_inline_size = 15;
   struct small_blob {     bytes_view::value_type data[max_inline_size];     int8_t size;   };
   union u {     blob_storage::ref_type ptr;     small_blob small;   }
 _u;
 };
  class table;
  using column_family = table;
  class partition_key_view;
  class clustering_key_prefix;
  class clustering_key_prefix_view;
  using clustering_key = clustering_key_prefix;
  template <typename TopLevel, typename TopLevelView> class compound_wrapper {
 };
  template <typename TopLevel> class prefix_view_on_prefix_compound {
 };
  template <typename TopLevel, typename TopLevelView, typename FullTopLevel> class prefix_compound_wrapper     : public compound_wrapper<TopLevel, TopLevelView> {
 };
  class partition_key     : public compound_wrapper<partition_key, partition_key_view> {
 };
  class clustering_key_prefix     : public prefix_compound_wrapper<           clustering_key_prefix, clustering_key_prefix_view, clustering_key> {
 };
  template <typename T> class range_bound {
 };
  template <typename T> class wrapping_range {
 };
  template <typename T> class nonwrapping_range {
 };
  GCC6_CONCEPT(template <template <typename> typename T, typename U>              concept bool Range =                  std::is_same<T<U>, wrapping_range<U>>::value ||                  std::is_same<T<U>, nonwrapping_range<U>>::value;
 ) namespace std {
 }
  enum class bound_kind : uint8_t {
   excl_end = 0,   incl_start = 1,   incl_end = 6,   excl_start = 7, };
  class bound_view {
 };
 namespace dht {
 }
   namespace dht {
 class decorated_key;
 }
   template <typename EnumType, EnumType... Items> struct super_enum {
   using enum_type = EnumType;
   using sequence_type = typename std::underlying_type<enum_type>::type;
 };
  class bad_enum_set_mask : public std::invalid_argument {
 };
  template <typename Enum> class enum_set {
 public:   using mask_type = size_t;
   using enum_type = typename Enum::enum_type;
 private:   static constexpr int mask_digits = std::numeric_limits<mask_type>::digits;
   using mask_iterator = seastar::bitsets::set_iterator<mask_digits>;
   mask_type _mask;
   static auto make_iterator(mask_iterator iter) {     return boost::make_transform_iterator(         std::move(iter),         [](typename Enum::sequence_type s) { return enum_type(s); });   }
 public:   using iterator =       std::invoke_result_t<decltype(&enum_set::make_iterator), mask_iterator>;
   constexpr enum_set() : _mask(0) {}
 };
#include <seastar/net/inet_address.hh>
namespace gms {
 class inet_address { private:   net::inet_address _addr; public:   inet_address() = default;   inet_address(int32_t ip) : inet_address(uint32_t(ip)) {}   explicit inet_address(uint32_t ip) : _addr(net::ipv4_address(ip)) {}   inet_address(const net::inet_address &addr) : _addr(addr) {}   inet_address(const socket_address &sa) : inet_address(sa.addr()) {}   const net::inet_address &addr() const { return _addr; }   inet_address(const inet_address &) = default;   operator const seastar::net::inet_address &() const { return _addr; }   inet_address(const sstring &addr) {     if (addr == "localhost") {       _addr = net::ipv4_address("127.0.0.1");     } else {       _addr = net::inet_address(addr);     }   }   bytes_view bytes() const {     return bytes_view(reinterpret_cast<const int8_t *>(_addr.data()),                       _addr.size());   }   uint32_t raw_addr() const { return addr().as_ipv4_address().ip; }   sstring to_sstring() const { return format("{}", *this); }   friend inline bool operator==(const inet_address &x, const inet_address &y) {     return x._addr == y._addr;   }   friend inline bool operator!=(const inet_address &x, const inet_address &y) {     using namespace std::rel_ops;     return x._addr != y._addr;   }   friend inline bool operator<(const inet_address &x, const inet_address &y) {     return x.bytes() < y.bytes();   }   friend struct std::hash<inet_address>;   using opt_family = std::optional<net::inet_address::family>;   static future<inet_address> lookup(sstring, opt_family family = {},                                      opt_family preferred = {}); };
 std::ostream &operator<<(std::ostream &os, const inet_address &x);
 }
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
  class position_in_partition_view {
 };
  class range_tombstone final {
 };
  class row {
 };
  class rows_entry {
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
  namespace streamed_mutation {
 }
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
  GCC6_CONCEPT(template <typename Consumer>              concept bool FlatMutationReaderConsumer() {
                return requires(Consumer c, mutation_fragment mf) {                  { c(std::move(mf)) }                  ->stop_iteration;                };
              }
 ) class flat_mutation_reader final {
 };
  template <typename Consumer> inline future<> consume_partitions(flat_mutation_reader &reader,                                    Consumer consumer,                                    db::timeout_clock::time_point timeout) {
   using futurator = futurize<std::result_of_t<Consumer(mutation &&)>>;
   return do_with(       std::move(consumer), [&reader, timeout](Consumer &c) -> future<> {         return repeat([&reader, &c, timeout]() {           return read_mutation_from_flat_mutation_reader(reader, timeout)               .then([&c](mutation_opt &&mo) -> future<stop_iteration> {                 if (!mo) {                   return make_ready_future<stop_iteration>(stop_iteration::yes);                 }                 return futurator::apply(c, std::move(*mo));               });         });       }
);
 }
  namespace ser {
 }
  class frozen_mutation final {
 public:   frozen_mutation(const mutation &m);
   mutation unfreeze(schema_ptr s) const;
 };
  struct reader_resources {
 };
  class reader_permit {
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
