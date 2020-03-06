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
   using time_point = std::chrono::time_point<gc_clock, duration>;
   static constexpr auto is_steady = base::is_steady;
   static time_point now();
 };
#include <seastar/core/shared_ptr.hh>
using column_count_type = uint32_t;
  using column_id = column_count_type;
  class schema;
  class schema_extension;
  using schema_ptr = seastar::lw_shared_ptr<const schema>;
  namespace api {
 using timestamp_type = int64_t;
 timestamp_type constexpr missing_timestamp =     std::numeric_limits<timestamp_type>::min();
 timestamp_type constexpr min_timestamp =     std::numeric_limits<timestamp_type>::min() + 1;
 timestamp_type constexpr max_timestamp =     std::numeric_limits<timestamp_type>::max();
 class timestamp_clock final {   using base = std::chrono::system_clock; public:   using rep = timestamp_type;   using duration = std::chrono::microseconds;   using period = typename duration::period;   using time_point = std::chrono::time_point<timestamp_clock, duration>;   static constexpr bool is_steady = base::is_steady; };
 }
   GCC6_CONCEPT(template <typename T> concept bool HasTriCompare =                  requires(const T &t) {
                    { t.compare(t) }
                    ->int;
                  }
  &&                  std::is_same<std::result_of_t<decltype (&T::compare)(T, T)>,                               int>::value;
 ) template <typename T> class with_relational_operators {
 private:   template <typename U>   GCC6_CONCEPT(requires HasTriCompare<U>)   int do_compare(const U &t) const;
 public:   bool operator<(const T &t) const;
   bool operator<=(const T &t) const;
   bool operator>(const T &t) const;
   bool operator>=(const T &t) const;
   bool operator==(const T &t) const;
   bool operator!=(const T &t) const;
 };
  struct tombstone final : public with_relational_operators<tombstone> {
   api::timestamp_type timestamp;
   gc_clock::time_point deletion_time;
   tombstone();
   friend std::ostream &operator<<(std::ostream &out, const tombstone &t);
 };
#include <seastar/util/optimized_optional.hh>
template <typename CharT> class basic_mutable_view {
   CharT *_begin = nullptr;
   CharT *_end = nullptr;
 public:   using value_type = CharT;
   using pointer = CharT *;
   using iterator = CharT *;
   using const_iterator = CharT *;
   basic_mutable_view() = default;
   template <typename U, U N>   basic_mutable_view(basic_sstring<CharT, U, N> &str)       : _begin(str.begin()), _end(str.end()) {}
   basic_mutable_view(CharT *ptr, size_t length)       : _begin(ptr), _end(ptr + length) {}
   operator std::basic_string_view<CharT>() const noexcept {     return std::basic_string_view<CharT>(begin(), size());   }
   CharT &operator[](size_t idx) const { return _begin[idx]; }
   iterator begin() const { return _begin; }
   iterator end() const { return _end; }
   CharT *data() const { return _begin; }
   size_t size() const { return _end - _begin; }
   bool empty() const { return _begin == _end; }
   void remove_prefix(size_t n) { _begin += n; }
   void remove_suffix(size_t n) { _end -= n; }
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
   for (char c : s) {     if (c == '\0') {       throw UTFDataFormatException();     }   }
   if (s.size() > std::numeric_limits<uint16_t>::max()) {     throw UTFDataFormatException();   }
   serialize_int16(out, s.size());
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
#include <seastar/core/simple-stream.hh>
class bytes_ostream {
 public:   using size_type = bytes::size_type;
   using value_type = bytes::value_type;
   using fragment_type = bytes_view;
   static constexpr size_type max_chunk_size() { return 128 * 1024; }
 private:   static_assert(sizeof(value_type) == 1,                 "value_type is assumed to be one byte long");
   struct chunk {     std::unique_ptr<chunk> next;     ~chunk() {       auto p = std::move(next);       while (p) {         auto p_next = std::move(p->next);         p = std::move(p_next);       }     }     size_type offset;     size_type size;     value_type data[0];     void operator delete(void *ptr) { free(ptr); }   };
   static constexpr size_type default_chunk_size{512};
 private:   std::unique_ptr<chunk> _begin;
   chunk *_current;
   size_type _size;
   size_type _initial_chunk_size = default_chunk_size;
 public:   class fragment_iterator       : public std::iterator<std::input_iterator_tag, bytes_view> {     chunk *_current = nullptr;   public:     fragment_iterator() = default;     fragment_iterator(chunk *current) : _current(current) {}     fragment_iterator(const fragment_iterator &) = default;     fragment_iterator &operator=(const fragment_iterator &) = default;     bytes_view operator*() const;     bytes_view operator->() const;     fragment_iterator &operator++();     fragment_iterator operator++(int);     bool operator==(const fragment_iterator &other) const;     bool operator!=(const fragment_iterator &other) const;   };
   using const_iterator = fragment_iterator;
   class output_iterator {   public:     using iterator_category = std::output_iterator_tag;     using difference_type = std::ptrdiff_t;     using value_type = bytes_ostream::value_type;     using pointer = bytes_ostream::value_type *;     using reference = bytes_ostream::value_type &;     friend class bytes_ostream;   private:     bytes_ostream *_ostream = nullptr;   private:     explicit output_iterator(bytes_ostream &os);   public:     reference operator*() const;     output_iterator &operator++();     output_iterator operator++(int);   };
 private:   size_type current_space_left() const;
   size_type next_alloc_size(size_t data_size) const;
   [[gnu::always_inline]] value_type *alloc(size_type size) {     if (__builtin_expect(size <= current_space_left(), true)) {       auto ret = _current->data + _current->offset;       _current->offset += size;       _size += size;       return ret;     } else {       return alloc_new(size);     }   }
   [[gnu::noinline]] value_type *alloc_new(size_type size) {     auto alloc_size = next_alloc_size(size);     auto space = malloc(alloc_size);     if (!space) {       throw std::bad_alloc();     }     auto new_chunk = std::unique_ptr<chunk>(new (space) chunk());     new_chunk->offset = size;     new_chunk->size = alloc_size - sizeof(chunk);     if (_current) {       _current->next = std::move(new_chunk);       _current = _current->next.get();     } else {       _begin = std::move(new_chunk);       _current = _begin.get();     }     _size += size;     return _current->data;   }
 public:   explicit bytes_ostream(size_t initial_chunk_size) noexcept       : _begin(), _current(nullptr), _size(0),         _initial_chunk_size(initial_chunk_size) {}
   bytes_ostream() noexcept : bytes_ostream(default_chunk_size) {}
   [[gnu::always_inline]] inline void write(bytes_view v) {     if (v.empty()) {       return;     }     auto this_size = std::min(v.size(), size_t(current_space_left()));     if (__builtin_expect(this_size, true)) {       memcpy(_current->data + _current->offset, v.begin(), this_size);       _current->offset += this_size;       _size += this_size;       v.remove_prefix(this_size);     }     while (!v.empty()) {       auto this_size = std::min(v.size(), size_t(max_chunk_size()));       std::copy_n(v.begin(), this_size, alloc_new(this_size));       v.remove_prefix(this_size);     }   }
   [[gnu::always_inline]] void write(const char *ptr, size_t size) {     write(bytes_view(reinterpret_cast<const signed char *>(ptr), size));   }
   bool is_linearized() const { return !_begin || !_begin->next; }
   bytes_view view() const {     assert(is_linearized());     if (!_current) {       return bytes_view();     }     return bytes_view(_current->data, _size);   }
   bytes_view linearize() {     if (is_linearized()) {       return view();     }     auto space = malloc(_size + sizeof(chunk));     if (!space) {       throw std::bad_alloc();     }     auto new_chunk = std::unique_ptr<chunk>(new (space) chunk());     new_chunk->offset = _size;     new_chunk->size = _size;     auto dst = new_chunk->data;     auto r = _begin.get();     while (r) {       auto next = r->next.get();       dst = std::copy_n(r->data, r->offset, dst);       r = next;     }     _current = new_chunk.get();     _begin = std::move(new_chunk);     return bytes_view(_current->data, _size);   }
   size_type size() const { return _size; }
   size_type size_bytes() const { return _size; }
   bool empty() const { return _size == 0; }
   void reserve(size_t size) {}
   void append(const bytes_ostream &o) {     for (auto &&bv : o.fragments()) {       write(bv);     }   }
   void remove_suffix(size_t n) {     _size -= n;     auto left = _size;     auto current = _begin.get();     while (current) {       if (current->offset >= left) {         current->offset = left;         _current = current;         current->next.reset();         return;       }       left -= current->offset;       current = current->next.get();     }   }
   fragment_iterator begin() const { return {_begin.get()}; }
   fragment_iterator end() const { return {nullptr}; }
   output_iterator write_begin() { return output_iterator(*this); }
   boost::iterator_range<fragment_iterator> fragments() const {     return {begin(), end()};   }
   struct position {     chunk *_chunk;     size_type _offset;   };
 };
   class abstract_type;
  class column_definition;
  using cql_protocol_version_type = uint8_t;
  class cql_serialization_format {
   cql_protocol_version_type _version;
 public:   static constexpr cql_protocol_version_type latest_version = 4;
   explicit cql_serialization_format(cql_protocol_version_type version)       : _version(version) {}
   static cql_serialization_format latest() {     return cql_serialization_format{latest_version};   }
   static cql_serialization_format internal() { return latest(); }
   bool using_32_bits_for_collections() const { return _version >= 3; }
   bool operator==(cql_serialization_format x) const {     return _version == x._version;   }
   bool operator!=(cql_serialization_format x) const;
   cql_protocol_version_type protocol_version() const;
   friend std::ostream &operator<<(std::ostream &out,                                   const cql_serialization_format &sf);
   bool   collection_format_unchanged(cql_serialization_format other =                                   cql_serialization_format::latest()) const;
 };
  namespace utils {
 template <typename T, size_t N> class small_vector {   static_assert(N > 0);   static_assert(std::is_nothrow_move_constructible_v<T>);   static_assert(std::is_nothrow_move_assignable_v<T>);   static_assert(std::is_nothrow_destructible_v<T>); private:   T *_begin;   T *_end;   T *_capacity_end;   union internal {     internal();     ~internal();     T storage[N];   };   internal _internal; private:   bool uses_internal_storage() const noexcept {     return _begin == _internal.storage;   }   [[gnu::cold]] [[gnu::noinline]] void expand(size_t new_capacity) {     auto ptr =         static_cast<T *>(::aligned_alloc(alignof(T), new_capacity * sizeof(T)));     if (!ptr) {       throw std::bad_alloc();     }     auto n_end = std::uninitialized_move(begin(), end(), ptr);     std::destroy(begin(), end());     if (!uses_internal_storage()) {       std::free(_begin);     }     _begin = ptr;     _end = n_end;     _capacity_end = ptr + new_capacity;   }   [[gnu::cold]] [[gnu::noinline]] void   slow_copy_assignment(const small_vector &other) {     auto ptr =         static_cast<T *>(::aligned_alloc(alignof(T), other.size() * sizeof(T)));     if (!ptr) {       throw std::bad_alloc();     }     auto n_end = ptr;     try {       n_end = std::uninitialized_copy(other.begin(), other.end(), n_end);     } catch (...) {       std::free(ptr);       throw;     }     std::destroy(begin(), end());     if (!uses_internal_storage()) {       std::free(_begin);     }     _begin = ptr;     _end = n_end;     _capacity_end = n_end;   }   void reserve_at_least(size_t n) {     if (__builtin_expect(_begin + n > _capacity_end, false)) {       expand(std::max(n, capacity() * 2));     }   }   [[noreturn]] [[gnu::cold]] [[gnu::noinline]] void throw_out_of_range() {     throw std::out_of_range("out of range small vector access");   } public:   using value_type = T;   using pointer = T *;   using const_pointer = const T *;   using reference = T &;   using const_reference = const T &;   using iterator = T *;   using const_iterator = const T *;   using reverse_iterator = std::reverse_iterator<iterator>;   using const_reverse_iterator = std::reverse_iterator<const_iterator>;   small_vector() noexcept       : _begin(_internal.storage), _end(_begin), _capacity_end(_begin + N) {}   template <typename InputIterator>   small_vector(InputIterator first, InputIterator last) : small_vector() {     if constexpr (std::is_base_of_v<std::forward_iterator_tag,                                     typename std::iterator_traits<                                         InputIterator>::iterator_category>) {       reserve(std::distance(first, last));       _end = std::uninitialized_copy(first, last, _end);     } else {       std::copy(first, last, std::back_inserter(*this));     }   }   small_vector(std::initializer_list<T> list)       : small_vector(list.begin(), list.end()) {}   small_vector(small_vector &&other) noexcept {     if (other.uses_internal_storage()) {       _begin = _internal.storage;       _capacity_end = _begin + N;       if constexpr (std::is_trivially_copyable_v<T>) {         std::memcpy(_internal.storage, other._internal.storage, N * sizeof(T));         _end = _begin + other.size();       } else {         _end = _begin;         for (auto &e : other) {           new (_end++) T(std::move(e));           e.~T();         }       }       other._end = other._internal.storage;     } else {       _begin = std::exchange(other._begin, other._internal.storage);       _end = std::exchange(other._end, other._internal.storage);       _capacity_end =           std::exchange(other._capacity_end, other._internal.storage + N);     }   }   small_vector(const small_vector &other) noexcept : small_vector() {     reserve(other.size());     _end = std::uninitialized_copy(other.begin(), other.end(), _end);   }   small_vector &operator=(small_vector &&other) noexcept {     clear();     if (other.uses_internal_storage()) {       if (__builtin_expect(!uses_internal_storage(), false)) {         std::free(_begin);         _begin = _internal.storage;       }       _capacity_end = _begin + N;       if constexpr (std::is_trivially_copyable_v<T>) {         std::memcpy(_internal.storage, other._internal.storage, N * sizeof(T));         _end = _begin + other.size();       } else {         _end = _begin;         for (auto &e : other) {           new (_end++) T(std::move(e));           e.~T();         }       }       other._end = other._internal.storage;     } else {       if (__builtin_expect(!uses_internal_storage(), false)) {         std::free(_begin);       }       _begin = std::exchange(other._begin, other._internal.storage);       _end = std::exchange(other._end, other._internal.storage);       _capacity_end =           std::exchange(other._capacity_end, other._internal.storage + N);     }     return *this;   }   small_vector &operator=(const small_vector &other) {     if constexpr (std::is_nothrow_copy_constructible_v<T>) {       if (capacity() >= other.size()) {         clear();         _end = std::uninitialized_copy(other.begin(), other.end(), _end);         return *this;       }     }     slow_copy_assignment(other);     return *this;   }   ~small_vector() {     clear();     if (__builtin_expect(!uses_internal_storage(), false)) {       std::free(_begin);     }   }   void reserve(size_t n) {     if (__builtin_expect(_begin + n > _capacity_end, false)) {       expand(n);     }   }   void clear() noexcept {     std::destroy(_begin, _end);     _end = _begin;   }   iterator begin() noexcept { return _begin; }   const_iterator begin() const noexcept { return _begin; }   const_iterator cbegin() const noexcept { return _begin; }   iterator end() noexcept { return _end; }   const_iterator end() const noexcept { return _end; }   const_iterator cend() const noexcept { return _end; }   reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }   const_reverse_iterator rbegin() const noexcept {     return const_reverse_iterator(end());   }   const_reverse_iterator crbegin() const noexcept {     return const_reverse_iterator(end());   }   reverse_iterator rend() noexcept { return reverse_iterator(begin()); }   const_reverse_iterator rend() const noexcept {     return const_reverse_iterator(begin());   }   const_reverse_iterator crend() const noexcept {     return const_reverse_iterator(begin());   }   T *data() noexcept { return _begin; }   const T *data() const noexcept { return _begin; }   T &front() noexcept { return *begin(); }   const T &front() const noexcept { return *begin(); }   T &back() noexcept { return end()[-1]; }   const T &back() const noexcept { return end()[-1]; }   T &operator[](size_t idx) noexcept { return data()[idx]; }   const T &operator[](size_t idx) const noexcept { return data()[idx]; }   T &at(size_t idx) {     if (__builtin_expect(idx >= size(), false)) {       throw_out_of_range();     }     return operator[](idx);   }   const T &at(size_t idx) const {     if (__builtin_expect(idx >= size(), false)) {       throw_out_of_range();     }     return operator[](idx);   }   bool empty() const noexcept { return _begin == _end; }   size_t size() const noexcept { return _end - _begin; }   size_t capacity() const noexcept { return _capacity_end - _begin; }   template <typename... Args> T &emplace_back(Args &&... args) {     if (__builtin_expect(_end == _capacity_end, false)) {       expand(std::max<size_t>(capacity() * 2, 1));     }     auto &ref = *new (_end) T(std::forward<Args>(args)...);     ++_end;     return ref;   }   T &push_back(const T &value) { return emplace_back(value); }   T &push_back(T &&value) { return emplace_back(std::move(value)); }   template <typename InputIterator>   iterator insert(const_iterator cpos, InputIterator first,                   InputIterator last) {     if constexpr (std::is_base_of_v<std::forward_iterator_tag,                                     typename std::iterator_traits<                                         InputIterator>::iterator_category>) {       if (first == last) {         return const_cast<iterator>(cpos);       }       auto idx = cpos - _begin;       auto new_count = std::distance(first, last);       reserve_at_least(size() + new_count);       auto pos = _begin + idx;       auto after = std::distance(pos, end());       if (__builtin_expect(pos == end(), true)) {         _end = std::uninitialized_copy(first, last, end());         return pos;       } else if (after > new_count) {         std::uninitialized_move(end() - new_count, end(), end());         std::move_backward(pos, end() - new_count, end());         try {           std::copy(first, last, pos);         } catch (...) {           std::move(pos + new_count, end() + new_count, pos);           std::destroy(end(), end() + new_count);           throw;         }       } else {         std::uninitialized_move(pos, end(), pos + new_count);         auto mid = std::next(first, after);         try {           std::uninitialized_copy(mid, last, end());           try {             std::copy(first, mid, pos);           } catch (...) {             std::destroy(end(), pos + new_count);             throw;           }         } catch (...) {           std::move(pos + new_count, end() + new_count, pos);           std::destroy(pos + new_count, end() + new_count);           throw;         }       }       _end += new_count;       return pos;     } else {       auto start = cpos - _begin;       auto idx = start;       while (first != last) {         try {           insert(begin() + idx, *first);           ++first;           ++idx;         } catch (...) {           erase(begin() + start, begin() + idx);           throw;         }       }       return begin() + idx;     }   }   template <typename... Args>   iterator emplace(const_iterator cpos, Args &&... args) {     auto idx = cpos - _begin;     reserve_at_least(size() + 1);     auto pos = _begin + idx;     if (pos != _end) {       new (_end) T(std::move(_end[-1]));       std::move_backward(pos, _end - 1, _end);       pos->~T();     }     try {       new (pos) T(std::forward<Args>(args)...);     } catch (...) {       if (pos != _end) {         new (pos) T(std::move(pos[1]));         std::move(pos + 2, _end + 1, pos + 1);         _end->~T();       }       throw;     }     _end++;     return pos;   }   iterator insert(const_iterator cpos, const T &obj) {     return emplace(cpos, obj);   }   iterator insert(const_iterator cpos, T &&obj) {     return emplace(cpos, std::move(obj));   }   void resize(size_t n) {     if (n < size()) {       erase(end() - (size() - n), end());     } else if (n > size()) {       reserve_at_least(n);       _end = std::uninitialized_value_construct_n(_end, n - size());     }   }   void resize(size_t n, const T &value) {     if (n < size()) {       erase(end() - (size() - n), end());     } else if (n > size()) {       reserve_at_least(n);       auto nend = _begin + n;       std::uninitialized_fill(_end, nend, value);       _end = nend;     }   }   void pop_back() noexcept { (--_end)->~T(); }   iterator erase(const_iterator cit) noexcept { return erase(cit, cit + 1); }   iterator erase(const_iterator cfirst, const_iterator clast) noexcept {     auto first = const_cast<iterator>(cfirst);     auto last = const_cast<iterator>(clast);     std::move(last, end(), first);     auto nend = _end - (clast - cfirst);     std::destroy(nend, _end);     _end = nend;     return first;   }   void swap(small_vector &other) noexcept { std::swap(*this, other); }   bool operator==(const small_vector &other) const noexcept {     return size() == other.size() && std::equal(_begin, _end, other.begin());   }   bool operator!=(const small_vector &other) const noexcept {     return !(*this == other);   } };
 }
   namespace utils {
 struct chunked_vector_free_deleter {   void operator()(void *x) const { ::free(x); } };
 template <typename T, size_t max_contiguous_allocation = 128 * 1024> class chunked_vector {   static_assert(std::is_nothrow_move_constructible<T>::value,                 "T must be nothrow move constructible");   using chunk_ptr = std::unique_ptr<T[], chunked_vector_free_deleter>;   utils::small_vector<chunk_ptr, 1> _chunks;   size_t _size = 0;   size_t _capacity = 0; private:   static size_t max_chunk_capacity() {     return std::max(max_contiguous_allocation / sizeof(T), size_t(1));   }   void reserve_for_push_back() {     if (_size == _capacity) {       do_reserve_for_push_back();     }   }   void do_reserve_for_push_back();   void make_room(size_t n);   chunk_ptr new_chunk(size_t n);   T *addr(size_t i) const {     return &_chunks[i / max_chunk_capacity()][i % max_chunk_capacity()];   }   void check_bounds(size_t i) const {     if (i >= _size) {       throw std::out_of_range("chunked_vector out of range access");     }   }   static void migrate(T *begin, T *end, T *result); public:   using value_type = T;   using size_type = size_t;   using difference_type = ssize_t;   using reference = T &;   using const_reference = const T &;   using pointer = T *;   using const_pointer = const T *; public:   ;   bool empty() const { return !_size; }; public:   template <class ValueType> class iterator_type {     const chunk_ptr *_chunks;     size_t _i;   public:     using iterator_category = std::random_access_iterator_tag;     using value_type = ValueType;     using difference_type = ssize_t;     using pointer = ValueType *;     using reference = ValueType &;   private:   public:     iterator_type() = default;     iterator_type(const iterator_type<std::remove_const_t<ValueType>> &x)         : _chunks(x._chunks), _i(x._i) {}     reference operator*() const { return *addr(); }     pointer operator->() const { return addr(); }     reference operator[](ssize_t n) const { return *(*this + n); }     iterator_type &operator++() {       ++_i;       return *this;     }     iterator_type operator++(int) {       auto x = *this;       ++_i;       return x;     }     iterator_type &operator--() {       --_i;       return *this;     }     iterator_type operator--(int) {       auto x = *this;       --_i;       return x;     }     iterator_type &operator+=(ssize_t n) {       _i += n;       return *this;     }     iterator_type &operator-=(ssize_t n) {       _i -= n;       return *this;     }     iterator_type operator+(ssize_t n) const {       auto x = *this;       return x += n;     }     iterator_type operator-(ssize_t n) const {       auto x = *this;       return x -= n;     }     friend iterator_type operator+(ssize_t n, iterator_type a) { return a + n; }     friend ssize_t operator-(iterator_type a, iterator_type b) {       return a._i - b._i;     }     bool operator==(iterator_type x) const { return _i == x._i; }     bool operator!=(iterator_type x) const { return _i != x._i; }     bool operator<(iterator_type x) const { return _i < x._i; }     bool operator<=(iterator_type x) const { return _i <= x._i; }     bool operator>(iterator_type x) const { return _i > x._i; }     bool operator>=(iterator_type x) const { return _i >= x._i; }     friend class chunked_vector;   };   using iterator = iterator_type<T>;   using const_iterator = iterator_type<const T>; public:   const_iterator cbegin() const;   std::reverse_iterator<const_iterator> rend() const;   std::reverse_iterator<const_iterator> crbegin() const;   std::reverse_iterator<const_iterator> crend() const; public:   bool operator==(const chunked_vector &x) const;   bool operator!=(const chunked_vector &x) const; };
 }
  class marshal_exception : public std::exception {
   sstring _why;
 public:   marshal_exception() = delete;
   marshal_exception(sstring why) : _why(sstring("marshaling error: ") + why) {}
   virtual const char *what() const noexcept override { return _why.c_str(); }
 };
#include <seastar/net/ip.hh>
#include <seastar/util/backtrace.hh>
 template <typename TypesIterator, typename InputIt1, typename InputIt2,           typename Compare> int prefix_equality_tri_compare(TypesIterator types, InputIt1 first1,                                 InputIt1 last1, InputIt2 first2, InputIt2 last2,                                 Compare comp);
  class data_value;
  using data_type = shared_ptr<const abstract_type>;
  class abstract_type : public enable_shared_from_this<abstract_type> {
   sstring _name;
   std::optional<uint32_t> _value_length_if_fixed;
 public:   enum class kind : int8_t {     ascii,     boolean,     byte,     bytes,     counter,     date,     decimal,     double_kind,     duration,     empty,     float_kind,     inet,     int32,     list,     long_kind,     map,     reversed,     set,     short_kind,     simple_date,     time,     timestamp,     timeuuid,     tuple,     user,     utf8,     uuid,     varint,   };
 private:   kind _kind;
 public:   kind get_kind() const;
                                  public:                        bool is_reversed() const;
   friend class list_type_impl;
 private:   mutable sstring _cql3_type_name;
 protected:   friend class tuple_type_impl;
   friend class data_value;
   friend class reversed_type_impl;
   template <typename T> friend const T &value_cast(const data_value &value);
   ;
 };
  static int tri_compare(data_type t, bytes_view e1, bytes_view e2);
  extern thread_local const shared_ptr<const abstract_type> bytes_type;
  template <typename T> T read_simple(bytes_view &v) ;
#include <boost/range/adaptor/transformed.hpp>
  enum class allow_prefixes {
 no, yes };
  template <allow_prefixes AllowPrefixes = allow_prefixes::no> class compound_type final {
 private:   const std::vector<data_type> _types;
   const bool _byte_order_equal;
   const bool _byte_order_comparable;
   const bool _is_reversed;
 public:   static constexpr bool is_prefixable = AllowPrefixes == allow_prefixes::yes;
   using prefix_type = compound_type<allow_prefixes::yes>;
   using value_type = std::vector<bytes>;
   using size_type = uint16_t;
   compound_type(std::vector<data_type> types)       : _types(std::move(types)),         _byte_order_equal(             std::all_of(_types.begin(), _types.end(),                         [](auto t) { return t->is_byte_order_equal(); }
)),         _byte_order_comparable(false),         _is_reversed(_types.size() == 1 && _types[0]->is_reversed()) {}
   compound_type(compound_type &&) = default;
   auto const &types() const { return _types; }
   bool is_singular() const { return _types.size() == 1; }
   prefix_type as_prefix() { return prefix_type(_types); }
 private:   template <typename RangeOfSerializedComponents, typename CharOutputIterator>   static void serialize_value(RangeOfSerializedComponents &&values,                               CharOutputIterator &out) {     for (auto &&val : values) {       assert(val.size() <= std::numeric_limits<size_type>::max());       write<size_type>(out, size_type(val.size()));       out = std::copy(val.begin(), val.end(), out);     }   }
 public:   ;
   ;
   class iterator       : public std::iterator<std::input_iterator_tag, const bytes_view> {   private:     bytes_view _v;     bytes_view _current;   private:     void read_current() {       size_type len;       {         if (_v.empty()) {           _v = bytes_view(nullptr, 0);           return;         }         len = read_simple<size_type>(_v);         if (_v.size() < len) {           throw_with_backtrace<marshal_exception>(               format("compound_type iterator - not enough bytes, expected "                      "{:d}, got {:d}",                      len, _v.size()));         }       }       _current = bytes_view(_v.begin(), len);       _v.remove_prefix(len);     }   public:     struct end_iterator_tag {};     iterator operator++(int) {       iterator i(*this);       ++(*this);       return i;     }     const value_type &operator*() const { return _current; }     const value_type *operator->() const { return &_current; }     bool operator!=(const iterator &i) const;     bool operator==(const iterator &i) const;   };
   static iterator begin(const bytes_view &v);
   static iterator end(const bytes_view &v);
   static boost::iterator_range<iterator> components(const bytes_view &v);
 };
#include <boost/dynamic_bitset.hpp>
#include <boost/range/join.hpp>
 enum class ordinal_column_id : column_count_type {
};
  class column_set {
 public:   using bitset = boost::dynamic_bitset<uint64_t>;
   using size_type = bitset::size_type;
   static_assert(       static_cast<column_count_type>(boost::dynamic_bitset<uint64_t>::npos) ==       ~static_cast<column_count_type>(0));
   static constexpr ordinal_column_id npos =       static_cast<ordinal_column_id>(bitset::npos);
 private:   bitset _mask;
 };
  using table_schema_version = utils::UUID;
  class schema_registry_entry;
  namespace db {
 class extensions;
 }
  enum class column_kind {
   partition_key,   clustering_key,   static_column,   regular_column };
  enum class cf_type : uint8_t {
   standard,   super, };
  struct speculative_retry {
   enum class type { NONE, CUSTOM, PERCENTILE, ALWAYS };
 private:   type _t;
   double _v;
 public:   speculative_retry(type t, double v);
 };
  class index_metadata final {
};
  class thrift_schema {
   bool _compound = true;
   bool _is_dynamic = false;
 public:   friend class schema;
 };
  static constexpr int DEFAULT_MIN_COMPACTION_THRESHOLD = 4;
  static constexpr int DEFAULT_MAX_COMPACTION_THRESHOLD = 32;
  static constexpr int DEFAULT_MIN_INDEX_INTERVAL = 128;
  static constexpr int DEFAULT_GC_GRACE_SECONDS = 864000;
  class column_mapping_entry {
 public: };
  class raw_view_info final {
   utils::UUID _base_id;
   sstring _base_name;
   bool _include_all_columns;
   sstring _where_clause;
 public: };
  class view_info;
  class v3_columns {
   bool _is_dense = false;
   bool _is_compound = false;
   std::vector<column_definition> _columns;
   std::unordered_map<bytes, const column_definition *> _columns_by_name;
 public: public: };
  namespace query {
 class partition_slice;
 }
  class schema final : public enable_lw_shared_from_this<schema> {
   friend class v3_columns;
 public:   struct dropped_column {     data_type type;     api::timestamp_type timestamp;   };
   using extensions_map = std::map<sstring, ::shared_ptr<schema_extension>>;
 private:   struct raw_schema {     utils::UUID _id;     sstring _ks_name;     sstring _cf_name;     std::vector<column_definition> _columns;     sstring _comment;     gc_clock::duration _default_time_to_live = gc_clock::duration::zero();     data_type _regular_column_name_type;     data_type _default_validation_class = bytes_type;     double _bloom_filter_fp_chance = 0.01;     extensions_map _extensions;     bool _is_dense = false;     bool _is_compound = true;     bool _is_counter = false;     cf_type _type = cf_type::standard;     int32_t _gc_grace_seconds = DEFAULT_GC_GRACE_SECONDS;     double _dc_local_read_repair_chance = 0.1;     double _read_repair_chance = 0.0;     double _crc_check_chance = 1;     int32_t _min_compaction_threshold = DEFAULT_MIN_COMPACTION_THRESHOLD;     int32_t _max_compaction_threshold = DEFAULT_MAX_COMPACTION_THRESHOLD;     int32_t _min_index_interval = DEFAULT_MIN_INDEX_INTERVAL;     int32_t _max_index_interval = 2048;     int32_t _memtable_flush_period = 0;     speculative_retry _speculative_retry =         ::speculative_retry(speculative_retry::type::PERCENTILE, 0.99);     bool _compaction_enabled = true;     table_schema_version _version;     std::unordered_map<sstring, dropped_column> _dropped_columns;     std::map<bytes, data_type> _collections;     std::unordered_map<sstring, index_metadata> _indices_by_name;     bool _wait_for_sync = false;   };
   raw_schema _raw;
   thrift_schema _thrift;
   v3_columns _v3_columns;
   mutable schema_registry_entry *_registry_entry = nullptr;
   std::unique_ptr<::view_info> _view_info;
   const std::array<column_count_type, 3> _offsets;
   column_count_type _static_column_count;
   friend class db::extensions;
   friend class schema_builder;
 public:   using row_column_ids_are_ordered_by_name = std::true_type;
   struct column {     bytes name;     data_type type;   };
 private:   void rebuild();
   schema(const raw_schema &, std::optional<raw_view_info>);
 public:   schema(std::optional<utils::UUID> id, std::string_view ks_name,          std::string_view cf_name, std::vector<column> partition_key,          std::vector<column> clustering_key,          std::vector<column> regular_columns,          std::vector<column> static_columns, data_type regular_column_name_type,          std::string_view comment = {}
);
   schema(const schema &);
   ~schema();
   bool is_static_compact_table() const;
   bool has_static_columns() const;
   friend class schema_registry_entry;
 public: };
#include <any>
class migrate_fn_type {
   std::any _migrators;
   uint32_t _align = 0;
 };
  struct blob_storage {
   struct [[gnu::packed]] ref_type {     blob_storage *ptr;     ref_type() {}     ref_type(blob_storage * ptr) : ptr(ptr) {}     operator blob_storage *() const { return ptr; }     blob_storage *operator->() const { return ptr; }     blob_storage &operator*() const { return *ptr; }   };
   using size_type = uint32_t;
   using char_type = bytes_view::value_type;
   ref_type *backref;
   size_type size;
   size_type frag_size;
   ref_type next;
   char_type data[];
   blob_storage(ref_type *backref, size_type size, size_type frag_size) noexcept       : backref(backref), size(size), frag_size(frag_size), next(nullptr) {     *backref = this;   }
 }
  __attribute__((packed));
  class managed_bytes {
   static thread_local std::unordered_map<       const blob_storage *, std::unique_ptr<bytes_view::value_type[]>>       _lc_state;
 private:   static constexpr size_t max_inline_size = 15;
   struct small_blob {     bytes_view::value_type data[max_inline_size];     int8_t size;   };
   union u {     blob_storage::ref_type ptr;     small_blob small;   }
 _u;
   static_assert(sizeof(small_blob) > sizeof(blob_storage *),                 "inline size too small");
 private:   bool external() const;
   template <typename Func>   friend std::result_of_t<Func()> with_linearized_managed_bytes(Func &&func);
 };
  class table;
  using column_family = table;
  class partition_key_view;
  class clustering_key_prefix;
  class clustering_key_prefix_view;
  using clustering_key = clustering_key_prefix;
  template <typename TopLevel, typename TopLevelView> class compound_wrapper {
 protected:   managed_bytes _bytes;
 protected: public:   struct with_schema_wrapper {     const schema &s;     const TopLevel &key;   };
   ;
   template <typename T>   static TopLevel from_singular(const schema &s, const T &v);
   TopLevelView view() const;
   struct tri_compare {     typename TopLevel::compound _t;   };
   bool equal(const schema &s, const TopLevel &other) const;
   operator bytes_view() const;
   size_t size(const schema &s) const;
   size_t external_memory_usage() const;
   size_t memory_usage() const;
 };
  template <typename TopLevel> class prefix_view_on_prefix_compound {
 public:   using iterator = typename compound_type<allow_prefixes::yes>::iterator;
   struct less_compare_with_prefix {};
 };
  template <typename TopLevel, typename TopLevelView, typename FullTopLevel> class prefix_compound_wrapper     : public compound_wrapper<TopLevel, TopLevelView> {
   using base = compound_wrapper<TopLevel, TopLevelView>;
 };
  class partition_key     : public compound_wrapper<partition_key, partition_key_view> {
 public:   using c_type = compound_type<allow_prefixes::no>;
   ;
   using compound = lw_shared_ptr<c_type>;
 };
  class clustering_key_prefix     : public prefix_compound_wrapper<           clustering_key_prefix, clustering_key_prefix_view, clustering_key> {
 public:   ;
   using compound = lw_shared_ptr<compound_type<allow_prefixes::yes>>;
 };
  template <typename T> class range_bound {
   T _value;
   bool _inclusive;
 public:   ;
 };
  template <typename T> class nonwrapping_range;
  template <typename T> class wrapping_range {
   template <typename U> using optional = std::optional<U>;
 };
  template <typename T> class nonwrapping_range {
   template <typename U> using optional = std::optional<U>;
 public:   using bound = range_bound<T>;
 };
  GCC6_CONCEPT(template <template <typename> typename T, typename U>              concept bool Range =                  std::is_same<T<U>, wrapping_range<U>>::value ||                  std::is_same<T<U>, nonwrapping_range<U>>::value;
 ) namespace std {
   template <typename T> struct hash<wrapping_range<T>> {     using argument_type = wrapping_range<T>;     using result_type = decltype(std::hash<T>()(std::declval<T>()));     result_type operator()(argument_type const &s) const {       auto hash = std::hash<T>();       auto left = s.start() ? hash(s.start()->value()) : 0;       auto right = s.end() ? hash(s.end()->value()) : 0;       return 31 * left + right;     }   };
   template <typename T> struct hash<nonwrapping_range<T>> {     using argument_type = nonwrapping_range<T>;     using result_type = decltype(std::hash<T>()(std::declval<T>()));     result_type operator()(argument_type const &s) const {       return hash<wrapping_range<T>>()(s);     }   };
 }
  enum class bound_kind : uint8_t {
   excl_end = 0,   incl_start = 1,   incl_end = 6,   excl_start = 7, };
  std::ostream &operator<<(std::ostream &out, const bound_kind k);
  int32_t weight(bound_kind k);
  class bound_view {
   const static thread_local clustering_key _empty_prefix;
   std::reference_wrapper<const clustering_key_prefix> _prefix;
   bound_kind _kind;
   template <template <typename> typename R>   GCC6_CONCEPT(requires Range<R, clustering_key_prefix_view>)   static std::optional<typename R<       clustering_key_prefix_view>::bound> to_range_bound(const bound_view &bv) {     if (&bv._prefix.get() == &_empty_prefix) {       return {};     }     bool inclusive =         bv._kind != bound_kind::excl_end && bv._kind != bound_kind::excl_start;     return {typename R<clustering_key_prefix_view>::bound(         bv._prefix.get().view(), inclusive)};   }
   friend std::ostream &operator<<(std::ostream &out, const bound_view &b) {     return out << "{bound: prefix=" << b._prefix.get() << ", kind=" << b._kind                << "}";   }
 };
 namespace dht {
 class token;
 enum class token_kind {   before_all_keys,   key,   after_all_keys, };
 class token {   static inline int64_t normalize(int64_t t) {     return t == std::numeric_limits<int64_t>::min()                ? std::numeric_limits<int64_t>::max()                : t;   } public:   using kind = token_kind;   kind _kind;   int64_t _data;   token() : _kind(kind::before_all_keys) {}   token(kind k, int64_t d) : _kind(std::move(k)), _data(normalize(d)) {}   token(kind k, const bytes &b) : _kind(std::move(k)) {     if (b.size() != sizeof(_data)) {       throw std::runtime_error(           fmt::format("Wrong token bytes size: expected {} but got {}",                       sizeof(_data), b.size()));     }     std::copy_n(b.begin(), sizeof(_data), reinterpret_cast<int8_t *>(&_data));     _data = net::ntoh(_data);   }                   };
 }
   namespace dht {
 class decorated_key;
 class ring_position;
 using partition_range = nonwrapping_range<ring_position>;
 class decorated_key { public:   dht::token _token;   partition_key _key;   struct less_comparator {     schema_ptr s;                       };   bool equal(const schema &s, const decorated_key &other) const;   bool less_compare(const schema &s, const decorated_key &other) const;   bool less_compare(const schema &s, const ring_position &other) const;   int tri_compare(const schema &s, const decorated_key &other) const;   int tri_compare(const schema &s, const ring_position &other) const;   const dht::token &token() const ;   const partition_key &key() const ;   size_t external_memory_usage() const ;   size_t memory_usage() const ; };
 class decorated_key_equals_comparator {   const schema &_schema; public:   explicit decorated_key_equals_comparator(const schema &schema)       : _schema(schema) {} };
 using decorated_key_opt = std::optional<decorated_key>;
 }
   template <typename EnumType, EnumType... Items> struct super_enum {
   using enum_type = EnumType;
   template <enum_type... values> struct max {     static constexpr enum_type max_of(enum_type a, enum_type b) {       return a > b ? a : b;     }     template <enum_type first, enum_type second, enum_type... rest>     static constexpr enum_type get() {       return max_of(first, get<second, rest...>());     }     template <enum_type first> static constexpr enum_type get() {       return first;     }     static constexpr enum_type value = get<values...>();   };
   template <enum_type... values> struct min {     static constexpr enum_type min_of(enum_type a, enum_type b) {       return a < b ? a : b;     }     template <enum_type first, enum_type second, enum_type... rest>     static constexpr enum_type get() {       return min_of(first, get<second, rest...>());     }     template <enum_type first> static constexpr enum_type get() {       return first;     }     static constexpr enum_type value = get<values...>();   };
   using sequence_type = typename std::underlying_type<enum_type>::type;
   template <enum_type Elem> static constexpr sequence_type sequence_for() {     return static_cast<sequence_type>(Elem);   }
   static sequence_type sequence_for(enum_type elem) {     return static_cast<sequence_type>(elem);   }
   static constexpr sequence_type max_sequence =       sequence_for<max<Items...>::value>();
   static constexpr sequence_type min_sequence =       sequence_for<min<Items...>::value>();
   static_assert(min_sequence >= 0, "negative enum values unsupported");
 };
  class bad_enum_set_mask : public std::invalid_argument {
 public:   bad_enum_set_mask()       : std::invalid_argument(             "Bit mask contains invalid enumeration indices.") {}
 };
  template <typename Enum> class enum_set {
 public:   using mask_type = size_t;
   using enum_type = typename Enum::enum_type;
 private:   static constexpr int mask_digits = std::numeric_limits<mask_type>::digits;
   using mask_iterator = seastar::bitsets::set_iterator<mask_digits>;
   mask_type _mask;
   constexpr enum_set(mask_type mask) : _mask(mask) {}
   template <enum_type Elem> static constexpr unsigned shift_for() {     return Enum::template sequence_for<Elem>();   }
   static auto make_iterator(mask_iterator iter) {     return boost::make_transform_iterator(         std::move(iter),         [](typename Enum::sequence_type s) { return enum_type(s); });   }
 public:   using iterator =       std::invoke_result_t<decltype(&enum_set::make_iterator), mask_iterator>;
   constexpr enum_set() : _mask(0) {}
   static constexpr enum_set from_mask(mask_type mask) {     const auto bit_range =         seastar::bitsets::for_each_set(std::bitset<mask_digits>(mask));     if (!std::all_of(bit_range.begin(), bit_range.end(),                      &Enum::is_valid_sequence)) {       throw bad_enum_set_mask();     }     return enum_set(mask);   };
   struct prepared {     mask_type mask;   };
   ;
   static_assert(std::numeric_limits<mask_type>::max() >=                     ((size_t)1 << Enum::max_sequence),                 "mask type too small");
   ;
                  iterator end() const;
   template <enum_type... items> struct frozen {     template <enum_type first> static constexpr mask_type make_mask();     static constexpr mask_type make_mask();     template <enum_type first, enum_type second, enum_type... rest>     static constexpr mask_type make_mask();     static constexpr mask_type mask = make_mask<items...>();     template <enum_type Elem> static constexpr bool contains();     static bool contains(enum_type e);     static bool contains(prepared e);     static constexpr enum_set<Enum> unfreeze();   };
   template <enum_type... items> static constexpr enum_set<Enum> of() {     return frozen<items...>::unfreeze();   }
 };
#include <random>
#include <seastar/core/sharded.hh>
#include <seastar/net/inet_address.hh>
namespace gms {
 class inet_address { private:   net::inet_address _addr; public:   inet_address() = default;   inet_address(int32_t ip) : inet_address(uint32_t(ip)) {}   explicit inet_address(uint32_t ip) : _addr(net::ipv4_address(ip)) {}   inet_address(const net::inet_address &addr) : _addr(addr) {}   inet_address(const socket_address &sa) : inet_address(sa.addr()) {}   const net::inet_address &addr() const { return _addr; }   inet_address(const inet_address &) = default;   operator const seastar::net::inet_address &() const { return _addr; }   inet_address(const sstring &addr) {     if (addr == "localhost") {       _addr = net::ipv4_address("127.0.0.1");     } else {       _addr = net::inet_address(addr);     }   }   bytes_view bytes() const {     return bytes_view(reinterpret_cast<const int8_t *>(_addr.data()),                       _addr.size());   }   uint32_t raw_addr() const { return addr().as_ipv4_address().ip; }   sstring to_sstring() const { return format("{}", *this); }   friend inline bool operator==(const inet_address &x, const inet_address &y) {     return x._addr == y._addr;   }   friend inline bool operator!=(const inet_address &x, const inet_address &y) {     using namespace std::rel_ops;     return x._addr != y._addr;   }   friend inline bool operator<(const inet_address &x, const inet_address &y) {     return x.bytes() < y.bytes();   }   friend struct std::hash<inet_address>;   using opt_family = std::optional<net::inet_address::family>;   static future<inet_address> lookup(sstring, opt_family family = {},                                      opt_family preferred = {}); };
 std::ostream &operator<<(std::ostream &os, const inet_address &x);
 }
   namespace tracing {
 using elapsed_clock = std::chrono::steady_clock;
 extern logging::logger tracing_logger;
 class trace_state_ptr;
 class tracing;
 class backend_registry;
 enum class trace_type : uint8_t {   NONE,   QUERY,   REPAIR, };
 extern std::vector<sstring> trace_type_names;
 class span_id { private:   uint64_t _id = illegal_id; public:   static constexpr uint64_t illegal_id = 0; public: };
 struct one_session_records;
 using records_bulk = std::deque<lw_shared_ptr<one_session_records>>;
 struct backend_session_state_base {   virtual ~backend_session_state_base(){}; };
 struct i_tracing_backend_helper {   using wall_clock = std::chrono::system_clock; protected:   tracing &_local_tracing; public:   i_tracing_backend_helper(tracing &tr) : _local_tracing(tr) {}   virtual ~i_tracing_backend_helper() {}   virtual future<> start() = 0;   virtual future<> stop() = 0;   virtual void write_records_bulk(records_bulk &bulk) = 0;   virtual std::unique_ptr<backend_session_state_base>   allocate_session_state() const = 0; private:   friend class tracing; };
 struct event_record {   sstring message;   elapsed_clock::duration elapsed;   i_tracing_backend_helper::wall_clock::time_point event_time_point;   event_record(       sstring message_, elapsed_clock::duration elapsed_,       i_tracing_backend_helper::wall_clock::time_point event_time_point_)       : message(std::move(message_)), elapsed(elapsed_),         event_time_point(event_time_point_) {} };
 struct session_record {   gms::inet_address client;   std::map<sstring, sstring> parameters;   std::set<sstring> tables;   sstring username;   sstring request;   size_t request_size = 0;   size_t response_size = 0;   std::chrono::system_clock::time_point started_at;   trace_type command = trace_type::NONE;   elapsed_clock::duration elapsed;   std::chrono::seconds slow_query_record_ttl; private:   bool _consumed = false; public:   session_record() : username("<unauthenticated request>"), elapsed(-1) {} };
 class one_session_records { private:   shared_ptr<tracing> _local_tracing_ptr; public:   utils::UUID session_id;   session_record session_rec;   std::chrono::seconds ttl;   std::deque<event_record> events_recs;   std::unique_ptr<backend_session_state_base> backend_state_ptr;   bool do_log_slow_query = false;   uint64_t *budget_ptr;   span_id parent_id;   span_id my_span_id; private:   bool _is_pending_for_write = false; };
 class tracing : public seastar::async_sharded_service<tracing> { public:   static const gc_clock::duration write_period;   static constexpr int max_pending_sessions = 1000;   static constexpr int exp_trace_events_per_session = 10;   static constexpr int max_pending_trace_records =       max_pending_sessions * exp_trace_events_per_session;   static constexpr int write_event_sessions_threshold = 100;   static constexpr int write_event_records_threshold =       write_event_sessions_threshold * exp_trace_events_per_session;   static constexpr int log_warning_period = 10000;   static const std::chrono::microseconds default_slow_query_duraion_threshold;   static const std::chrono::seconds default_slow_query_record_ttl;   struct stats {     uint64_t dropped_sessions = 0;     uint64_t dropped_records = 0;     uint64_t trace_records_count = 0;     uint64_t trace_errors = 0;   } stats; private:   uint64_t _active_sessions = 0;   uint64_t _cached_records = 0;   uint64_t _flushing_records = 0;   uint64_t _pending_for_write_records_count = 0;   records_bulk _pending_for_write_records_bulk;   timer<lowres_clock> _write_timer;   bool _down = true;   bool _slow_query_logging_enabled = false;   std::unique_ptr<i_tracing_backend_helper> _tracing_backend_helper_ptr;   sstring _thread_name;   const backend_registry &_backend_registry;   sstring _tracing_backend_helper_class_name;   seastar::metrics::metric_groups _metrics;   double _trace_probability = 0.0;   uint64_t _normalized_trace_probability = 0;   std::ranlux48_base _gen;   std::chrono::microseconds _slow_query_duration_threshold;   std::chrono::seconds _slow_query_record_ttl; public:   i_tracing_backend_helper &backend_helper();   const sstring &get_thread_name() const;   static seastar::sharded<tracing> &tracing_instance();   static tracing &get_local_tracing_instance();   bool started() const;   static future<> create_tracing(const backend_registry &br,                                  sstring tracing_backend_helper_class_name);   static future<> start_tracing();   tracing(const backend_registry &br,           sstring tracing_backend_helper_class_name);   future<> start();   bool slow_query_tracing_enabled() const;   void set_slow_query_threshold(std::chrono::microseconds new_threshold);   std::chrono::microseconds slow_query_threshold() const;   void set_slow_query_record_ttl(std::chrono::seconds new_ttl);   std::chrono::seconds slow_query_record_ttl() const; private:   void write_timer_callback();   bool may_create_new_session(       const std::optional<utils::UUID> &session_id = std::nullopt); };
 }
   class position_in_partition_view;
  namespace query {
 using column_id_vector = utils::small_vector<column_id, 8>;
 template <typename T> using range = wrapping_range<T>;
 using ring_position = dht::ring_position;
 using clustering_range = nonwrapping_range<clustering_key_prefix>;
 extern const dht::partition_range full_partition_range;
 extern const clustering_range full_clustering_range;
 typedef std::vector<clustering_range> clustering_row_ranges;
 void trim_clustering_row_ranges_to(const schema &s,                                    clustering_row_ranges &ranges,                                    position_in_partition_view pos,                                    bool reversed = false);
 void trim_clustering_row_ranges_to(const schema &s,                                    clustering_row_ranges &ranges,                                    const clustering_key &key,                                    bool reversed = false);
 class specific_ranges {};
 constexpr auto max_rows = std::numeric_limits<uint32_t>::max();
 class partition_slice { public:   enum class option {     send_clustering_key,     send_partition_key,     send_timestamp,     send_expiry,     reversed,     distinct,     collections_as_maps,     send_ttl,     allow_short_read,     with_digest,     bypass_cache,     always_return_static_content,   };   using option_set = enum_set<super_enum<       option, option::send_clustering_key, option::send_partition_key,       option::send_timestamp, option::send_expiry, option::reversed,       option::distinct, option::collections_as_maps, option::send_ttl,       option::allow_short_read, option::with_digest, option::bypass_cache,       option::always_return_static_content>>; public:   partition_slice(       clustering_row_ranges row_ranges, column_id_vector static_columns,       column_id_vector regular_columns, option_set options,       std::unique_ptr<specific_ranges> specific_ranges = nullptr,       cql_serialization_format = cql_serialization_format::internal(),       uint32_t partition_row_limit = max_rows);   partition_slice(clustering_row_ranges ranges, const schema &schema,                   const column_set &mask, option_set options);   partition_slice(const partition_slice &); };
 constexpr auto max_partitions = std::numeric_limits<uint32_t>::max();
 }
  class position_in_partition_view {
   friend class position_in_partition;
 public: private: public:   struct partition_start_tag_t {};
   class printer {   public:   };
 };
#include <boost/intrusive/set.hpp>
namespace bi = boost::intrusive;
  class range_tombstone final {
 };
  class row {
   class cell_entry {     friend class row;   public:     struct compare {};   };
 private: public: private:   ;
 };
  class row_marker;
  class rows_entry {
   friend class cache_tracker;
   friend class size_calculator;
 };
  namespace db {
 using timeout_clock = seastar::lowres_clock;
 }
   class clustering_row {
 public:   clustering_row(const schema &s, const clustering_row &other);
   class printer {   public:   };
 };
  class static_row {
   row _cells;
 public:   static_row() = default;
 };
  class partition_start final {
   dht::decorated_key _key;
   tombstone _partition_tombstone;
 public: };
  class partition_end final {
 public: };
  GCC6_CONCEPT(template <typename T, typename ReturnType>              concept bool MutationFragmentConsumer() {
                return requires(T t, static_row sr, clustering_row cr,                                range_tombstone rt, partition_start ph,                                partition_end pe) {                  { t.consume(std::move(sr)) }                  ->ReturnType;                  { t.consume(std::move(cr)) }                  ->ReturnType;                  { t.consume(std::move(rt)) }                  ->ReturnType;                  { t.consume(std::move(ph)) }                  ->ReturnType;                  { t.consume(std::move(pe)) }                  ->ReturnType;                };
              }
 ) class mutation_fragment {
   bool equal(const schema &s, const mutation_fragment &other) const;
   bool mergeable_with(const mutation_fragment &mf) const;
   class printer {     const schema &_schema;     const mutation_fragment &_mutation_fragment;   public:     printer(const schema &s, const mutation_fragment &mf);     printer(const printer &) = delete;     printer(printer &&) = delete;     friend std::ostream &operator<<(std::ostream &os, const printer &p);   };
   friend std::ostream &operator<<(std::ostream &os, const printer &p);
 };
  using mutation_fragment_opt = optimized_optional<mutation_fragment>;
  namespace streamed_mutation {
 class forwarding_tag;
 using forwarding = bool_class<forwarding_tag>;
 }
  GCC6_CONCEPT(template <typename F> concept bool StreamedMutationTranformer() {
   return requires(F f, mutation_fragment mf, schema_ptr s) {     { f(std::move(mf)) }     ->mutation_fragment;     { f(s) }     ->schema_ptr;   };
 }
 ) class mutation final {
   mutation() = default;
   friend class optimized_optional<mutation>;
 public:   const dht::decorated_key &decorated_key() const;
 public:   mutation sliced(const query::clustering_row_ranges &) const;
 private:   friend std::ostream &operator<<(std::ostream &os, const mutation &m);
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
   void upgrade_schema(const schema_ptr &s);
 };
  template <typename Consumer> inline future<> consume_partitions(flat_mutation_reader &reader,                                    Consumer consumer,                                    db::timeout_clock::time_point timeout) {
   static_assert(       std::is_same<future<stop_iteration>,                    futurize_t<std::result_of_t<Consumer(mutation &&)>>>::value,       "bad Consumer signature");
   using futurator = futurize<std::result_of_t<Consumer(mutation &&)>>;
   return do_with(       std::move(consumer), [&reader, timeout](Consumer &c) -> future<> {         return repeat([&reader, &c, timeout]() {           return read_mutation_from_flat_mutation_reader(reader, timeout)               .then([&c](mutation_opt &&mo) -> future<stop_iteration> {                 if (!mo) {                   return make_ready_future<stop_iteration>(stop_iteration::yes);                 }                 return futurator::apply(c, std::move(*mo));               });         });       }
);
 }
  namespace ser {
 class mutation_view;
 }
  class frozen_mutation final {
 private:   partition_key deserialize_key() const;
   ser::mutation_view mutation_view() const;
 public:   frozen_mutation(const mutation &m);
   mutation unfreeze(schema_ptr s) const;
   struct printer;
 };
  struct reader_resources {
   int count = 0;
   ssize_t memory = 0;
 };
  class reader_concurrency_semaphore;
  class reader_permit {
   void release();
 };
  reader_permit no_reader_permit();
  class mutation_source {
   using partition_range = const dht::partition_range &;
   using io_priority = const io_priority_class &;
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
