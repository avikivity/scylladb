#include <atomic>
extern std::atomic<int64_t> clocks_offset;
#include <seastar/core/sstring.hh>
namespace seastar {
template <typename T>
class shared_ptr;
template <typename T>
shared_ptr<T> make_shared(T&&);
template <typename T, typename... A>
shared_ptr<T> make_shared(A&&... a);
}
using namespace seastar;
using seastar::shared_ptr;
using seastar::make_shared;
#include <seastar/util/gcc6-concepts.hh>
GCC6_CONCEPT(
    template<typename H>
    concept bool Hasher() {
        return requires(H& h, const char* ptr, size_t size) {
            { h.update(ptr, size) } -> void;
        };
    }
)
class hasher {
public:
    virtual ~hasher() = default;
    virtual void update(const char* ptr, size_t size) = 0;
};
GCC6_CONCEPT(static_assert(Hasher<hasher>());)
template<typename T, typename Enable = void>
struct appending_hash;
template<typename H, typename T, typename... Args>
GCC6_CONCEPT(requires Hasher<H>())
void feed_hash(H& h, const T& value, Args&&... args);
#include <seastar/core/lowres_clock.hh>
class gc_clock final {
public:
    using base = seastar::lowres_system_clock;
    using rep = int64_t;
    using period = std::ratio<1, 1>; 
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<gc_clock, duration>;
    static constexpr auto is_steady = base::is_steady;
    static time_point now();
    static int32_t as_int32(duration d);
    static int32_t as_int32(time_point tp);
};
using expiry_opt = std::optional<gc_clock::time_point>;
using ttl_opt = std::optional<gc_clock::duration>;
static constexpr gc_clock::duration max_ttl = gc_clock::duration{20 * 365 * 24 * 60 * 60};
std::ostream& operator<<(std::ostream& os, gc_clock::time_point tp);
class db_clock final {
public:
    using base = std::chrono::system_clock;
    using rep = int64_t;
    using period = std::ratio<1, 1000>; 
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<db_clock, duration>;
    static constexpr bool is_steady = base::is_steady;
    static time_point now() {
        return time_point();
    }
};
gc_clock::time_point to_gc_clock(db_clock::time_point tp);
std::ostream& operator<<(std::ostream&, db_clock::time_point);
#include <seastar/core/shared_ptr.hh>
using column_count_type = uint32_t;
using column_id = column_count_type;
class schema;
class schema_extension;
using schema_ptr = seastar::lw_shared_ptr<const schema>;
namespace api {
using timestamp_type = int64_t;
timestamp_type constexpr missing_timestamp = std::numeric_limits<timestamp_type>::min();
timestamp_type constexpr min_timestamp = std::numeric_limits<timestamp_type>::min() + 1;
timestamp_type constexpr max_timestamp = std::numeric_limits<timestamp_type>::max();
class timestamp_clock final {
    using base = std::chrono::system_clock;
public:
    using rep = timestamp_type;
    using duration = std::chrono::microseconds;
    using period = typename duration::period;
    using time_point = std::chrono::time_point<timestamp_clock, duration>;
    static constexpr bool is_steady = base::is_steady;
    static time_point now();
};
timestamp_type new_timestamp();
}
/* For debugging and log messages. */
std::string format_timestamp(api::timestamp_type);
GCC6_CONCEPT(
template<typename T>
concept bool HasTriCompare =
    requires(const T& t) {
        { t.compare(t) } -> int;
    } && std::is_same<std::result_of_t<decltype(&T::compare)(T, T)>, int>::value; 
)
template<typename T>
class with_relational_operators {
private:
    template<typename U>
    GCC6_CONCEPT( requires HasTriCompare<U> )
    int do_compare(const U& t) const;
public:
    bool operator<(const T& t) const ;
    bool operator<=(const T& t) const;
    bool operator>(const T& t) const;
    bool operator>=(const T& t) const;
    bool operator==(const T& t) const;
    bool operator!=(const T& t) const;
};
/**
 * Represents deletion operation. Can be commuted with other tombstones via apply() method.
 * Can be empty.
 */
struct tombstone final : public with_relational_operators<tombstone> {
    api::timestamp_type timestamp;
    gc_clock::time_point deletion_time;
    tombstone(api::timestamp_type timestamp, gc_clock::time_point deletion_time);
    tombstone();
    int compare(const tombstone& t) const;
    explicit operator bool() const;
    void apply(const tombstone& t) noexcept;
    void apply_reversibly(tombstone& t) noexcept;
    void revert(tombstone& t) noexcept;
    tombstone operator+(const tombstone& t);
    friend std::ostream& operator<<(std::ostream& out, const tombstone& t);
};
template<>
struct appending_hash<tombstone> {
    template<typename Hasher>
    void operator()(Hasher& h, const tombstone& t) const;
};
using can_gc_fn = std::function<bool(tombstone)>;
static can_gc_fn always_gc = [] (tombstone) { return true; };
#include <seastar/util/optimized_optional.hh>
template<typename CharT>
class basic_mutable_view {
    CharT* _begin = nullptr;
    CharT* _end = nullptr;
public:
    using value_type = CharT;
    using pointer = CharT*;
    using iterator = CharT*;
    using const_iterator = CharT*;
    basic_mutable_view() = default;
    template<typename U, U N>
    basic_mutable_view(basic_sstring<CharT, U, N>& str)
        : _begin(str.begin())
        , _end(str.end())
    { }
    basic_mutable_view(CharT* ptr, size_t length)
        : _begin(ptr)
        , _end(ptr + length)
    { }
    operator std::basic_string_view<CharT>() const noexcept {
        return std::basic_string_view<CharT>(begin(), size());
    }
    CharT& operator[](size_t idx) const { return _begin[idx]; }
    iterator begin() const { return _begin; }
    iterator end() const { return _end; }
    CharT* data() const { return _begin; }
    size_t size() const { return _end - _begin; }
    bool empty() const { return _begin == _end; }
    void remove_prefix(size_t n) {
        _begin += n;
    }
    void remove_suffix(size_t n) {
        _end -= n;
    }
};
using bytes = basic_sstring<int8_t, uint32_t, 31, false>;
using bytes_view = std::basic_string_view<int8_t>;
using bytes_mutable_view = basic_mutable_view<bytes_view::value_type>;
using bytes_opt = std::optional<bytes>;
using sstring_view = std::string_view;
inline sstring_view to_sstring_view(bytes_view view) {
    return {reinterpret_cast<const char*>(view.data()), view.size()};
}
namespace std {
template <>
struct hash<bytes_view> {
    size_t operator()(bytes_view v) const {
        return hash<sstring_view>()({reinterpret_cast<const char*>(v.begin()), v.size()});
    }
};
}
bytes from_hex(sstring_view s);
sstring to_hex(bytes_view b);
sstring to_hex(const bytes& b);
sstring to_hex(const bytes_opt& b);
std::ostream& operator<<(std::ostream& os, const bytes& b);
std::ostream& operator<<(std::ostream& os, const bytes_opt& b);
namespace std {
std::ostream& operator<<(std::ostream& os, const bytes_view& b);
}
template<>
struct appending_hash<bytes> {
    template<typename Hasher>
    void operator()(Hasher& h, const bytes& v) const;
};
template<>
struct appending_hash<bytes_view> {
    template<typename Hasher>
    void operator()(Hasher& h, bytes_view v) const;
};
int32_t compare_unsigned(bytes_view v1, bytes_view v2);
#include <seastar/core/print.hh>
#include <seastar/net/byteorder.hh>
class UTFDataFormatException { };
class EOFException { };
static constexpr size_t serialize_int8_size = 1;
static constexpr size_t serialize_bool_size = 1;
static constexpr size_t serialize_int16_size = 2;
static constexpr size_t serialize_int32_size = 4;
static constexpr size_t serialize_int64_size = 8;
namespace internal_impl {
template <typename ExplicitIntegerType, typename CharOutputIterator, typename IntegerType>
GCC6_CONCEPT(requires std::is_integral<ExplicitIntegerType>::value && std::is_integral<IntegerType>::value && requires (CharOutputIterator it) {
    *it++ = 'a';
})
inline
void serialize_int(CharOutputIterator& out, IntegerType val) {
    ExplicitIntegerType nval = net::hton(ExplicitIntegerType(val));
    out = std::copy_n(reinterpret_cast<const char*>(&nval), sizeof(nval), out);
}
}
template <typename CharOutputIterator>
inline
void serialize_int8(CharOutputIterator& out, uint8_t val) {
    internal_impl::serialize_int<uint8_t>(out, val);
}
template <typename CharOutputIterator>
inline
void serialize_int16(CharOutputIterator& out, uint16_t val) {
    internal_impl::serialize_int<uint16_t>(out, val);
}
template <typename CharOutputIterator>
inline
void serialize_int32(CharOutputIterator& out, uint32_t val) {
    internal_impl::serialize_int<uint32_t>(out, val);
}
template <typename CharOutputIterator>
inline
void serialize_int64(CharOutputIterator& out, uint64_t val) {
    internal_impl::serialize_int<uint64_t>(out, val);
}
template <typename CharOutputIterator>
inline
void serialize_bool(CharOutputIterator& out, bool val) {
    serialize_int8(out, val ? 1 : 0);
}
template <typename CharOutputIterator>
GCC6_CONCEPT(requires requires (CharOutputIterator it) {
    *it++ = 'a';
})
inline
void serialize_string(CharOutputIterator& out, const sstring& s) {
    for (char c : s) {
        if (c == '\0') {
            throw UTFDataFormatException();
        }
    }
    if (s.size() > std::numeric_limits<uint16_t>::max()) {
        throw UTFDataFormatException();
    }
    serialize_int16(out, s.size());
    out = std::copy(s.begin(), s.end(), out);
}
template <typename CharOutputIterator>
GCC6_CONCEPT(requires requires (CharOutputIterator it) {
    *it++ = 'a';
})
inline
void serialize_string(CharOutputIterator& out, const char* s) {
    auto len = strlen(s);
    if (len > std::numeric_limits<uint16_t>::max()) {
        throw UTFDataFormatException();
    }
    serialize_int16(out, len);
    out = std::copy_n(s, len, out);
}
inline
size_t serialize_string_size(const sstring& s) {;
    return serialize_int16_size + s.size();
}
template<typename T, typename CharOutputIterator>
static inline
void write(CharOutputIterator& out, const T& val) {
    auto v = net::ntoh(val);
    out = std::copy_n(reinterpret_cast<char*>(&v), sizeof(v), out);
}
namespace utils {
class UUID {
private:
    int64_t most_sig_bits;
    int64_t least_sig_bits;
public:
    UUID() : most_sig_bits(0), least_sig_bits(0) {}
    UUID(int64_t most_sig_bits, int64_t least_sig_bits)
        : most_sig_bits(most_sig_bits), least_sig_bits(least_sig_bits) {}
    explicit UUID(const sstring& uuid_string) : UUID(sstring_view(uuid_string)) { }
    explicit UUID(const char * s) : UUID(sstring_view(s)) {}
    explicit UUID(sstring_view uuid_string);
    int64_t get_most_significant_bits() const {
        return most_sig_bits;
    }
    int64_t get_least_significant_bits() const {
        return least_sig_bits;
    }
    int version() const {
        return (most_sig_bits >> 12) & 0xf;
    }
    bool is_timestamp() const {
        return version() == 1;
    }
    int64_t timestamp() const {
        assert(is_timestamp());
        return ((most_sig_bits & 0xFFF) << 48) |
               (((most_sig_bits >> 16) & 0xFFFF) << 32) |
               (((uint64_t)most_sig_bits) >> 32);
    }
    sstring to_sstring() const {
        return format("{:08x}-{:04x}-{:04x}-{:04x}-{:012x}",
                ((uint64_t)most_sig_bits >> 32),
                ((uint64_t)most_sig_bits >> 16 & 0xffff),
                ((uint64_t)most_sig_bits & 0xffff),
                ((uint64_t)least_sig_bits >> 48 & 0xffff),
                ((uint64_t)least_sig_bits & 0xffffffffffffLL));
    }
    friend std::ostream& operator<<(std::ostream& out, const UUID& uuid);
    bool operator==(const UUID& v) const {
        return most_sig_bits == v.most_sig_bits
                && least_sig_bits == v.least_sig_bits
                ;
    }
    bool operator!=(const UUID& v) const {
        return !(*this == v);
    }
    bool operator<(const UUID& v) const {
         if (most_sig_bits != v.most_sig_bits) {
             return uint64_t(most_sig_bits) < uint64_t(v.most_sig_bits);
         } else {
             return uint64_t(least_sig_bits) < uint64_t(v.least_sig_bits);
         }
    }
    bool operator>(const UUID& v) const {
        return v < *this;
    }
    bool operator<=(const UUID& v) const {
        return !(*this > v);
    }
    bool operator>=(const UUID& v) const {
        return !(*this < v);
    }
    bytes serialize() const {
        bytes b(bytes::initialized_later(), serialized_size());
        auto i = b.begin();
        serialize(i);
        return b;
    }
    static size_t serialized_size() noexcept {
        return 16;
    }
    template <typename CharOutputIterator>
    void serialize(CharOutputIterator& out) const {
        serialize_int64(out, most_sig_bits);
        serialize_int64(out, least_sig_bits);
    }
};
UUID make_random_uuid();
}
template<>
struct appending_hash<utils::UUID> {
    template<typename Hasher>
    void operator()(Hasher& h, const utils::UUID& id) const {
        feed_hash(h, id.get_most_significant_bits());
        feed_hash(h, id.get_least_significant_bits());
    }
};
namespace std {
template<>
struct hash<utils::UUID> {
    size_t operator()(const utils::UUID& id) const {
        auto hilo = id.get_most_significant_bits()
                ^ id.get_least_significant_bits();
        return size_t((hilo >> 32) ^ hilo);
    }
};
}
#include <seastar/util/log.hh>
namespace logging {
using log_level = seastar::log_level;
using logger = seastar::logger;
using registry = seastar::logger_registry;
inline registry& logger_registry() noexcept {
    return seastar::global_logger_registry();
}
using settings = seastar::logging_settings;
inline void apply_settings(const settings& s) {
    seastar::apply_logging_settings(s);
}
using seastar::pretty_type_name;
using seastar::level_name;
}
namespace meta {
template<typename... Ts>
struct list { };
namespace internal {
template<bool... Vs>
constexpr ssize_t do_find_if_unpacked() {
    ssize_t i = -1;
    ssize_t j = 0;
    (..., ((Vs && i == -1) ? i = j : j++));
    return i;
}
template<ssize_t N>
struct negative_to_empty : std::integral_constant<size_t, N> { };
template<>
struct negative_to_empty<-1> { };
template<typename T>
struct is_same_as {
    template<typename U>
    using type = std::is_same<T, U>;
};
template<template<class> typename Predicate, typename... Ts>
struct do_find_if : internal::negative_to_empty<internal::do_find_if_unpacked<Predicate<Ts>::value...>()> { };
template<template<class> typename Predicate, typename... Ts>
struct do_find_if<Predicate, meta::list<Ts...>> : internal::negative_to_empty<internal::do_find_if_unpacked<Predicate<Ts>::value...>()> { };
}
template<template<class> typename Predicate, typename... Ts>
constexpr size_t find_if = internal::do_find_if<Predicate, Ts...>::value;
template<typename T, typename... Ts>
constexpr size_t find = find_if<internal::is_same_as<T>::template type, Ts...>;
namespace internal {
template<size_t N, typename... Ts>
struct do_get_unpacked { };
template<size_t N, typename T, typename... Ts>
struct do_get_unpacked<N, T, Ts...> : do_get_unpacked<N - 1, Ts...> { };
template<typename T, typename... Ts>
struct do_get_unpacked<0, T, Ts...> {
    using type = T;
};
template<size_t N, typename... Ts>
struct do_get : do_get_unpacked<N, Ts...> { };
template<size_t N, typename... Ts>
struct do_get<N, meta::list<Ts...>> : do_get_unpacked<N, Ts...> { };
}
template<size_t N, typename... Ts>
using get = typename internal::do_get<N, Ts...>::type;
namespace internal {
template<size_t N, typename Result, typename... Ts>
struct do_take_unpacked { };
template<typename... Ts>
struct do_take_unpacked<0, list<Ts...>> {
    using type = list<Ts...>;
};
template<typename... Ts, typename U, typename... Us>
struct do_take_unpacked<0, list<Ts...>, U, Us...> {
    using type = list<Ts...>;
};
template<size_t N, typename... Ts, typename U, typename... Us>
struct do_take_unpacked<N, list<Ts...>, U, Us...> {
    using type = typename do_take_unpacked<N - 1, list<Ts..., U>, Us...>::type;
};
template<size_t N, typename Result, typename... Ts>
struct do_take : do_take_unpacked<N, Result, Ts...> { };
template<size_t N, typename Result, typename... Ts>
struct do_take<N, Result, meta::list<Ts...>> : do_take_unpacked<N, Result, Ts...> { };
}
template<size_t N, typename... Ts>
using take = typename internal::do_take<N, list<>, Ts...>::type;
namespace internal {
template<typename... Ts>
struct do_for_each_unpacked {
    template<typename Function>
    static constexpr void run(Function&& fn) {
        (..., fn(static_cast<Ts*>(nullptr)));
    }
};
template<typename... Ts>
struct do_for_each : do_for_each_unpacked<Ts...> { };
template<typename... Ts>
struct do_for_each<meta::list<Ts...>> : do_for_each_unpacked<Ts...> { };
}
template<typename... Ts, typename Function>
constexpr void for_each(Function&& fn) {
    internal::do_for_each<Ts...>::run(std::forward<Function>(fn));
};
namespace internal {
template<typename... Ts>
struct get_size : std::integral_constant<size_t, sizeof...(Ts)> { };
template<typename... Ts>
struct get_size<meta::list<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> { };
}
template<typename... Ts>
constexpr size_t size = internal::get_size<Ts...>::value;
template<template <class> typename Predicate, typename... Ts>
static constexpr bool all_of = std::conjunction_v<Predicate<Ts>...>;
}
#include <boost/range/algorithm/copy.hpp>
enum class mutable_view { no, yes, };
GCC6_CONCEPT(
template<typename T>
concept bool FragmentRange = requires (T range) {
    typename T::fragment_type;
    requires std::is_same_v<typename T::fragment_type, bytes_view>
        || std::is_same_v<typename T::fragment_type, bytes_mutable_view>;
    { *range.begin() } -> typename T::fragment_type;
    { *range.end() } -> typename T::fragment_type;
    { range.size_bytes() } -> size_t;
    { range.empty() } -> bool; 
};
)
template<typename T, typename = void>
struct is_fragment_range : std::false_type { };
template<typename T>
struct is_fragment_range<T, std::void_t<typename T::fragment_type>> : std::true_type { };
template<typename T>
static constexpr bool is_fragment_range_v = is_fragment_range<T>::value;
template <typename T>
GCC6_CONCEPT(
    requires FragmentRange<T>
)
class fragment_range_view {
    const T* _range;
public:
    using fragment_type = typename T::fragment_type;
    using iterator = typename T::const_iterator;
    using const_iterator = typename T::const_iterator;
public:
    explicit fragment_range_view(const T& range) : _range(&range) { }
    const_iterator begin() const { return _range->begin(); }
    const_iterator end() const { return _range->end(); }
    size_t size_bytes() const { return _range->size_bytes(); }
    bool empty() const { return _range->empty(); }
};
#include <map>
#include <seastar/core/iostream.hh>
#include <seastar/core/simple-stream.hh>
/**
 * Utility for writing data into a buffer when its final size is not known up front.
 *
 * Internally the data is written into a chain of chunks allocated on-demand.
 * No resizing of previously written data happens.
 *
 */
class bytes_ostream {
public:
    using size_type = bytes::size_type;
    using value_type = bytes::value_type;
    using fragment_type = bytes_view;
    static constexpr size_type max_chunk_size() { return 128 * 1024; }
private:
    static_assert(sizeof(value_type) == 1, "value_type is assumed to be one byte long");
    struct chunk {
        std::unique_ptr<chunk> next;
        ~chunk() {
            auto p = std::move(next);
            while (p) {
                auto p_next = std::move(p->next);
                p = std::move(p_next);
            }
        }
        size_type offset; 
        size_type size;
        value_type data[0];
        void operator delete(void* ptr) { free(ptr); }
    };
    static constexpr size_type default_chunk_size{512};
private:
    std::unique_ptr<chunk> _begin;
    chunk* _current;
    size_type _size;
    size_type _initial_chunk_size = default_chunk_size;
public:
    class fragment_iterator : public std::iterator<std::input_iterator_tag, bytes_view> {
        chunk* _current = nullptr;
    public:
        fragment_iterator() = default;
        fragment_iterator(chunk* current) : _current(current) {}
        fragment_iterator(const fragment_iterator&) = default;
        fragment_iterator& operator=(const fragment_iterator&) = default;
        bytes_view operator*() const ;
        bytes_view operator->() const ;
        fragment_iterator& operator++() ;
        fragment_iterator operator++(int) ;
        bool operator==(const fragment_iterator& other) const ;
        bool operator!=(const fragment_iterator& other) const ;
    };
    using const_iterator = fragment_iterator;
    class output_iterator {
    public:
        using iterator_category = std::output_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = bytes_ostream::value_type;
        using pointer = bytes_ostream::value_type*;
        using reference = bytes_ostream::value_type&;
        friend class bytes_ostream;
    private:
        bytes_ostream* _ostream = nullptr;
    private:
        explicit output_iterator(bytes_ostream& os)  ;
    public:
        reference operator*() const ;
        output_iterator& operator++() ;
        output_iterator operator++(int) ;
    };
private:
     size_type current_space_left() const ;
    size_type next_alloc_size(size_t data_size) const ;
    [[gnu::always_inline]]
    value_type* alloc(size_type size) {
        if (__builtin_expect(size <= current_space_left(), true)) {
            auto ret = _current->data + _current->offset;
            _current->offset += size;
            _size += size;
            return ret;
        } else {
            return alloc_new(size);
        }
    }
    [[gnu::noinline]]
    value_type* alloc_new(size_type size) {
            auto alloc_size = next_alloc_size(size);
            auto space = malloc(alloc_size);
            if (!space) {
                throw std::bad_alloc();
            }
            auto new_chunk = std::unique_ptr<chunk>(new (space) chunk());
            new_chunk->offset = size;
            new_chunk->size = alloc_size - sizeof(chunk);
            if (_current) {
                _current->next = std::move(new_chunk);
                _current = _current->next.get();
            } else {
                _begin = std::move(new_chunk);
                _current = _begin.get();
            }
            _size += size;
            return _current->data;
    }
public:
    explicit bytes_ostream(size_t initial_chunk_size) noexcept
        : _begin()
        , _current(nullptr)
        , _size(0)
        , _initial_chunk_size(initial_chunk_size)
    { }
    bytes_ostream() noexcept : bytes_ostream(default_chunk_size) {}
    bytes_ostream(bytes_ostream&& o) noexcept
        : _begin(std::move(o._begin))
        , _current(o._current)
        , _size(o._size)
        , _initial_chunk_size(o._initial_chunk_size)
    {
        o._current = nullptr;
        o._size = 0;
    }
    bytes_ostream(const bytes_ostream& o)
        : _begin()
        , _current(nullptr)
        , _size(0)
        , _initial_chunk_size(o._initial_chunk_size)
    {
        append(o);
    }
    bytes_ostream& operator=(const bytes_ostream& o) {
        if (this != &o) {
            auto x = bytes_ostream(o);
            *this = std::move(x);
        }
        return *this;
    }
    bytes_ostream& operator=(bytes_ostream&& o) noexcept {
        if (this != &o) {
            this->~bytes_ostream();
            new (this) bytes_ostream(std::move(o));
        }
        return *this;
    }
    template <typename T>
    struct place_holder {
        value_type* ptr;
        seastar::simple_output_stream get_stream() {
            return seastar::simple_output_stream(reinterpret_cast<char*>(ptr), sizeof(T));
        }
    };
    template <typename T>
    inline
    std::enable_if_t<std::is_fundamental<T>::value, place_holder<T>>
    write_place_holder() {
        return place_holder<T>{alloc(sizeof(T))};
    }
    [[gnu::always_inline]]
    value_type* write_place_holder(size_type size) {
        return alloc(size);
    }
    [[gnu::always_inline]]
    inline void write(bytes_view v) {
        if (v.empty()) {
            return;
        }
        auto this_size = std::min(v.size(), size_t(current_space_left()));
        if (__builtin_expect(this_size, true)) {
            memcpy(_current->data + _current->offset, v.begin(), this_size);
            _current->offset += this_size;
            _size += this_size;
            v.remove_prefix(this_size);
        }
        while (!v.empty()) {
            auto this_size = std::min(v.size(), size_t(max_chunk_size()));
            std::copy_n(v.begin(), this_size, alloc_new(this_size));
            v.remove_prefix(this_size);
        }
    }
    [[gnu::always_inline]]
    void write(const char* ptr, size_t size) {
        write(bytes_view(reinterpret_cast<const signed char*>(ptr), size));
    }
    bool is_linearized() const {
        return !_begin || !_begin->next;
    }
    bytes_view view() const {
        assert(is_linearized());
        if (!_current) {
            return bytes_view();
        }
        return bytes_view(_current->data, _size);
    }
    bytes_view linearize() {
        if (is_linearized()) {
            return view();
        }
        auto space = malloc(_size + sizeof(chunk));
        if (!space) {
            throw std::bad_alloc();
        }
        auto new_chunk = std::unique_ptr<chunk>(new (space) chunk());
        new_chunk->offset = _size;
        new_chunk->size = _size;
        auto dst = new_chunk->data;
        auto r = _begin.get();
        while (r) {
            auto next = r->next.get();
            dst = std::copy_n(r->data, r->offset, dst);
            r = next;
        }
        _current = new_chunk.get();
        _begin = std::move(new_chunk);
        return bytes_view(_current->data, _size);
    }
    size_type size() const {
        return _size;
    }
    size_type size_bytes() const {
        return _size;
    }
    bool empty() const {
        return _size == 0;
    }
    void reserve(size_t size) {
    }
    void append(const bytes_ostream& o) {
        for (auto&& bv : o.fragments()) {
            write(bv);
        }
    }
    void remove_suffix(size_t n) {
        _size -= n;
        auto left = _size;
        auto current = _begin.get();
        while (current) {
            if (current->offset >= left) {
                current->offset = left;
                _current = current;
                current->next.reset();
                return;
            }
            left -= current->offset;
            current = current->next.get();
        }
    }
    fragment_iterator begin() const { return { _begin.get() }; }
    fragment_iterator end() const { return { nullptr }; }
    output_iterator write_begin() { return output_iterator(*this); }
    boost::iterator_range<fragment_iterator> fragments() const {
        return { begin(), end() };
    }
    struct position {
        chunk* _chunk;
        size_type _offset;
    };
    position pos() const ;
    size_type written_since(position pos) ;
    void retract(position pos) ;
    void reduce_chunk_count() ;
    bool operator==(const bytes_ostream& other) const ;
    bool operator!=(const bytes_ostream& other) const {
        return !(*this == other);
    }
    void clear() {
        if (_begin) {
            _begin->offset = 0;
            _size = 0;
            _current = _begin.get();
            _begin->next.reset();
        }
    }
};
class fragmented_temporary_buffer {
    using vector_type = std::vector<seastar::temporary_buffer<char>>;
public:
    static constexpr size_t default_fragment_size = 128 * 1024;
    class view;
    class istream;
    class reader;
    using ostream = seastar::memory_output_stream<vector_type::iterator>;
    fragmented_temporary_buffer() = default;
    fragmented_temporary_buffer(std::vector<seastar::temporary_buffer<char>> fragments, size_t size_bytes) noexcept;
    explicit operator view() const noexcept;
    istream get_istream() const noexcept;
    ostream get_ostream() noexcept ;
    size_t size_bytes() const;
    bool empty() const;
    void remove_prefix(size_t n) noexcept;
    void remove_suffix(size_t n) noexcept;
};
namespace fragmented_temporary_buffer_concepts {
GCC6_CONCEPT(
template<typename T>
concept bool ExceptionThrower = requires(T obj, size_t n) {
    obj.throw_out_of_range(n, n);
};
)
}
class fragmented_temporary_buffer::reader {
    std::vector<temporary_buffer<char>> _fragments;
    size_t _left = 0;
public:
    future<fragmented_temporary_buffer> read_exactly(input_stream<char>& in, size_t length);
};
#include <list>
namespace ser {
template<typename FragmentIterator>
class buffer_view {
public:
    using fragment_type = bytes_view;
    class iterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = bytes_view;
        using pointer = const bytes_view*;
        using reference = const bytes_view&;
        using difference_type = std::ptrdiff_t;
        iterator() = default;
        iterator(bytes_view current, size_t left, FragmentIterator next);
        bytes_view operator*() const;
        const bytes_view* operator->() const;
        iterator& operator++();
        iterator operator++(int);
        bool operator==(const iterator& other) const;
        bool operator!=(const iterator& other) const;
    };
    using const_iterator = iterator;
    explicit buffer_view(bytes_view current);
    buffer_view(bytes_view current, size_t size, FragmentIterator it);
    explicit buffer_view(typename seastar::memory_input_stream<FragmentIterator>::simple stream);
    explicit buffer_view(typename seastar::memory_input_stream<FragmentIterator>::fragmented stream);
    iterator begin() const;
    iterator end() const;
    size_t size_bytes() const;
    bool empty() const;
    bytes linearize() const;
};
}
/*
 * Import the auto generated forward decleration code
 */
class abstract_type;
class collection_type_impl;
template<mutable_view is_mutable>
class basic_atomic_cell_view {
protected:
    friend class atomic_cell;
public:
    using pointer_type = std::conditional_t<is_mutable == mutable_view::no, const uint8_t*, uint8_t*>;
protected:
    friend class atomic_cell_or_collection;
public:
    operator basic_atomic_cell_view<mutable_view::no>() const noexcept;
    void swap(basic_atomic_cell_view& other) noexcept;
    bool is_counter_update() const;
    bool is_live() const;
    bool is_live(tombstone t, bool is_counter) const;
    bool is_live(tombstone t, gc_clock::time_point now, bool is_counter) const;
    bool is_live_and_has_ttl() const;
    bool is_dead(gc_clock::time_point now) const;
    bool is_covered_by(tombstone t, bool is_counter) const;
    api::timestamp_type timestamp() const;
    void set_timestamp(api::timestamp_type ts);
    size_t value_size() const;
    bool is_value_fragmented() const;
    int64_t counter_update_value() const;
    gc_clock::time_point deletion_time() const;
    gc_clock::time_point expiry() const;
    gc_clock::duration ttl() const;
    bool has_expired(gc_clock::time_point now) const;
    bytes_view serialize() const;
};
class atomic_cell_view final : public basic_atomic_cell_view<mutable_view::no> {
    friend class atomic_cell;
public:
    friend std::ostream& operator<<(std::ostream& os, const atomic_cell_view& acv);
    class printer {
        const abstract_type& _type;
        const atomic_cell_view& _cell;
    public:
        printer(const abstract_type& type, const atomic_cell_view& cell) : _type(type), _cell(cell) {}
        friend std::ostream& operator<<(std::ostream& os, const printer& acvp);
    };
};
class atomic_cell_mutable_view final : public basic_atomic_cell_view<mutable_view::yes> {
public:
    friend class atomic_cell;
};
using atomic_cell_ref = atomic_cell_mutable_view;
class atomic_cell final : public basic_atomic_cell_view<mutable_view::yes> {
public:
    class collection_member_tag;
    using collection_member = bool_class<collection_member_tag>;
    atomic_cell(atomic_cell&&) = default;
    atomic_cell& operator=(const atomic_cell&) = delete;
    atomic_cell& operator=(atomic_cell&&) = default;
    void swap(atomic_cell& other) noexcept;
    operator atomic_cell_view() const;
    atomic_cell(const abstract_type& t, atomic_cell_view other);
    static atomic_cell make_dead(api::timestamp_type timestamp, gc_clock::time_point deletion_time);
    static atomic_cell make_live(const abstract_type& type, api::timestamp_type timestamp, bytes_view value,
                                 collection_member = collection_member::no);
    static atomic_cell make_live(const abstract_type& type, api::timestamp_type timestamp, ser::buffer_view<bytes_ostream::fragment_iterator> value,
                                 collection_member = collection_member::no);
    static atomic_cell make_live(const abstract_type& type, api::timestamp_type timestamp, const fragmented_temporary_buffer::view& value,
                                 collection_member = collection_member::no);
    static atomic_cell make_live(const abstract_type& type, api::timestamp_type timestamp, const bytes& value,
                                 collection_member cm = collection_member::no);
    static atomic_cell make_live_counter_update(api::timestamp_type timestamp, int64_t value);
    static atomic_cell make_live(const abstract_type&, api::timestamp_type timestamp, bytes_view value,
        gc_clock::time_point expiry, gc_clock::duration ttl, collection_member = collection_member::no);
    static atomic_cell make_live(const abstract_type&, api::timestamp_type timestamp, ser::buffer_view<bytes_ostream::fragment_iterator> value,
        gc_clock::time_point expiry, gc_clock::duration ttl, collection_member = collection_member::no);
    static atomic_cell make_live(const abstract_type&, api::timestamp_type timestamp, const fragmented_temporary_buffer::view& value,
        gc_clock::time_point expiry, gc_clock::duration ttl, collection_member = collection_member::no);
    static atomic_cell make_live(const abstract_type& type, api::timestamp_type timestamp, const bytes& value,
                                 gc_clock::time_point expiry, gc_clock::duration ttl, collection_member cm = collection_member::no);
    static atomic_cell make_live(const abstract_type& type, api::timestamp_type timestamp, bytes_view value, ttl_opt ttl, collection_member cm = collection_member::no);
    static atomic_cell make_live_uninitialized(const abstract_type& type, api::timestamp_type timestamp, size_t size);
    friend class atomic_cell_or_collection;
    friend std::ostream& operator<<(std::ostream& os, const atomic_cell& ac);
    class printer : atomic_cell_view::printer {
    public:
        printer(const abstract_type& type, const atomic_cell_view& cell);
        friend std::ostream& operator<<(std::ostream& os, const printer& acvp);
    };
};
class column_definition;
int compare_atomic_cell_for_merge(atomic_cell_view left, atomic_cell_view right);
void merge_column(const abstract_type& def,
        atomic_cell_or_collection& old,
        const atomic_cell_or_collection& neww);
using cql_protocol_version_type = uint8_t;
class cql_serialization_format {
    cql_protocol_version_type _version;
public:
    static constexpr cql_protocol_version_type latest_version = 4;
    explicit cql_serialization_format(cql_protocol_version_type version) : _version(version) {}
    static cql_serialization_format latest() { return cql_serialization_format{latest_version}; }
    static cql_serialization_format internal() { return latest(); }
    bool using_32_bits_for_collections() const { return _version >= 3; }
    bool operator==(cql_serialization_format x) const { return _version == x._version; }
    bool operator!=(cql_serialization_format x) const { return !operator==(x); }
    cql_protocol_version_type protocol_version() const { return _version; }
    friend std::ostream& operator<<(std::ostream& out, const cql_serialization_format& sf) {
        return out << static_cast<int>(sf._version);
    }
    bool collection_format_unchanged(cql_serialization_format other = cql_serialization_format::latest()) const {
        return using_32_bits_for_collections() == other.using_32_bits_for_collections();
    }
};
#include <unordered_set>
#include <set>
namespace utils {
template<typename T, size_t N>
class small_vector {
    static_assert(N > 0);
    static_assert(std::is_nothrow_move_constructible_v<T>);
    static_assert(std::is_nothrow_move_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);
private:
    T* _begin;
    T* _end;
    T* _capacity_end;
    union internal {
        internal() { }
        ~internal() { }
        T storage[N];
    };
    internal _internal;
private:
    bool uses_internal_storage() const noexcept {
        return _begin == _internal.storage;
    }
    [[gnu::cold]] [[gnu::noinline]]
    void expand(size_t new_capacity) {
        auto ptr = static_cast<T*>(::aligned_alloc(alignof(T), new_capacity * sizeof(T)));
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
    [[gnu::cold]] [[gnu::noinline]]
    void slow_copy_assignment(const small_vector& other) {
        auto ptr = static_cast<T*>(::aligned_alloc(alignof(T), other.size() * sizeof(T)));
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
    [[noreturn]] [[gnu::cold]] [[gnu::noinline]]
    void throw_out_of_range() {
        throw std::out_of_range("out of range small vector access");
    }
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    small_vector() noexcept
        : _begin(_internal.storage)
        , _end(_begin)
        , _capacity_end(_begin + N)
    { }
    template<typename InputIterator>
    small_vector(InputIterator first, InputIterator last) : small_vector() {
        if constexpr (std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<InputIterator>::iterator_category>) {
            reserve(std::distance(first, last));
            _end = std::uninitialized_copy(first, last, _end);
        } else {
            std::copy(first, last, std::back_inserter(*this));
        }
    }
    small_vector(std::initializer_list<T> list) : small_vector(list.begin(), list.end()) { }
    small_vector(small_vector&& other) noexcept {
        if (other.uses_internal_storage()) {
            _begin = _internal.storage;
            _capacity_end = _begin + N;
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(_internal.storage, other._internal.storage, N * sizeof(T));
                _end = _begin + other.size();
            } else {
                _end = _begin;
                for (auto& e : other) {
                    new (_end++) T(std::move(e));
                    e.~T();
                }
            }
            other._end = other._internal.storage;
        } else {
            _begin = std::exchange(other._begin, other._internal.storage);
            _end = std::exchange(other._end, other._internal.storage);
            _capacity_end = std::exchange(other._capacity_end, other._internal.storage + N);
        }
    }
    small_vector(const small_vector& other) noexcept : small_vector() {
        reserve(other.size());
        _end = std::uninitialized_copy(other.begin(), other.end(), _end);
    }
    small_vector& operator=(small_vector&& other) noexcept {
        clear();
        if (other.uses_internal_storage()) {
            if (__builtin_expect(!uses_internal_storage(), false)) {
                std::free(_begin);
                _begin = _internal.storage;
            }
            _capacity_end = _begin + N;
            if constexpr (std::is_trivially_copyable_v<T>) {
                std::memcpy(_internal.storage, other._internal.storage, N * sizeof(T));
                _end = _begin + other.size();
            } else {
                _end = _begin;
                for (auto& e : other) {
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
            _capacity_end = std::exchange(other._capacity_end, other._internal.storage + N);
        }
        return *this;
    }
    small_vector& operator=(const small_vector& other) {
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
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }
    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }
    T* data() noexcept { return _begin; }
    const T* data() const noexcept { return _begin; }
    T& front() noexcept { return *begin(); }
    const T& front() const noexcept { return *begin(); }
    T& back() noexcept { return end()[-1]; }
    const T& back() const noexcept { return end()[-1]; }
    T& operator[](size_t idx) noexcept { return data()[idx]; }
    const T& operator[](size_t idx) const noexcept { return data()[idx]; }
    T& at(size_t idx) {
        if (__builtin_expect(idx >= size(), false)) {
            throw_out_of_range();
        }
        return operator[](idx);
    }
    const T& at(size_t idx) const {
        if (__builtin_expect(idx >= size(), false)) {
            throw_out_of_range();
        }
        return operator[](idx);
    }
    bool empty() const noexcept { return _begin == _end; }
    size_t size() const noexcept { return _end - _begin; }
    size_t capacity() const noexcept { return _capacity_end - _begin; }
    template<typename... Args>
    T& emplace_back(Args&&... args) {
        if (__builtin_expect(_end == _capacity_end, false)) {
            expand(std::max<size_t>(capacity() * 2, 1));
        }
        auto& ref = *new (_end) T(std::forward<Args>(args)...);
        ++_end;
        return ref;
    }
    T& push_back(const T& value) {
        return emplace_back(value);
    }
    T& push_back(T&& value) {
        return emplace_back(std::move(value));
    }
    template<typename InputIterator>
    iterator insert(const_iterator cpos, InputIterator first, InputIterator last) {
        if constexpr (std::is_base_of_v<std::forward_iterator_tag, typename std::iterator_traits<InputIterator>::iterator_category>) {
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
    template<typename... Args>
    iterator emplace(const_iterator cpos, Args&&... args) {
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
    iterator insert(const_iterator cpos, const T& obj) {
        return emplace(cpos, obj);
    }
    iterator insert(const_iterator cpos, T&& obj) {
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
    void resize(size_t n, const T& value) {
        if (n < size()) {
            erase(end() - (size() - n), end());
        } else if (n > size()) {
            reserve_at_least(n);
            auto nend = _begin + n;
            std::uninitialized_fill(_end, nend, value);
            _end = nend;
        }
    }
    void pop_back() noexcept {
        (--_end)->~T();
    }
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
    void swap(small_vector& other) noexcept {
        std::swap(*this, other);
    }
    bool operator==(const small_vector& other) const noexcept {
        return size() == other.size() && std::equal(_begin, _end, other.begin());
    }
    bool operator!=(const small_vector& other) const noexcept {
        return !(*this == other);
    }
};
}
namespace utils {
struct chunked_vector_free_deleter {
    void operator()(void* x) const { ::free(x); }
};
template <typename T, size_t max_contiguous_allocation = 128*1024>
class chunked_vector {
    static_assert(std::is_nothrow_move_constructible<T>::value, "T must be nothrow move constructible");
    using chunk_ptr = std::unique_ptr<T[], chunked_vector_free_deleter>;
    utils::small_vector<chunk_ptr, 1> _chunks;
    size_t _size = 0;
    size_t _capacity = 0;
private:
    static size_t max_chunk_capacity() {
        return std::max(max_contiguous_allocation / sizeof(T), size_t(1));
    }
    void reserve_for_push_back() {
        if (_size == _capacity) {
            do_reserve_for_push_back();
        }
    }
    void do_reserve_for_push_back();
    void make_room(size_t n);
    chunk_ptr new_chunk(size_t n);
    T* addr(size_t i) const {
        return &_chunks[i / max_chunk_capacity()][i % max_chunk_capacity()];
    }
    void check_bounds(size_t i) const {
        if (i >= _size) {
            throw std::out_of_range("chunked_vector out of range access");
        }
    }
    static void migrate(T* begin, T* end, T* result);
public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = ssize_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
public:
    chunked_vector() = default;
    chunked_vector(const chunked_vector& x);
    chunked_vector(chunked_vector&& x) noexcept;
    template <typename Iterator>
    chunked_vector(Iterator begin, Iterator end);
    explicit chunked_vector(size_t n, const T& value = T());
    ~chunked_vector();
    chunked_vector& operator=(const chunked_vector& x);
    chunked_vector& operator=(chunked_vector&& x) noexcept;
    bool empty() const {
        return !_size;
    }
    size_t size() const {
        return _size;
    }
    T& operator[](size_t i) {
        return *addr(i);
    }
    const T& operator[](size_t i) const {
        return *addr(i);
    }
    T& at(size_t i) {
        check_bounds(i);
        return *addr(i);
    }
    const T& at(size_t i) const {
        check_bounds(i);
        return *addr(i);
    }
    void push_back(const T& x) {
        reserve_for_push_back();
        new (addr(_size)) T(x);
        ++_size;
    }
    void push_back(T&& x) {
        reserve_for_push_back();
        new (addr(_size)) T(std::move(x));
        ++_size;
    }
    template <typename... Args>
    T& emplace_back(Args&&... args) {
        reserve_for_push_back();
        auto& ret = *new (addr(_size)) T(std::forward<Args>(args)...);
        ++_size;
        return ret;
    }
    void pop_back() {
        --_size;
        addr(_size)->~T();
    }
    const T& back() const {
        return *addr(_size - 1);
    }
    T& back() {
        return *addr(_size - 1);
    }
    void clear();
    void shrink_to_fit();
    void resize(size_t n);
    void reserve(size_t n) {
        if (n > _capacity) {
            make_room(n);
        }
    }
    size_t memory_size() const {
        return _capacity * sizeof(T);
    }
public:
    template <class ValueType>
    class iterator_type {
        const chunk_ptr* _chunks;
        size_t _i;
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = ValueType;
        using difference_type = ssize_t;
        using pointer = ValueType*;
        using reference = ValueType&;
    private:
        pointer addr() const {
            return &_chunks[_i / max_chunk_capacity()][_i % max_chunk_capacity()];
        }
        iterator_type(const chunk_ptr* chunks, size_t i) : _chunks(chunks), _i(i) {}
    public:
        iterator_type() = default;
        iterator_type(const iterator_type<std::remove_const_t<ValueType>>& x) : _chunks(x._chunks), _i(x._i) {} 
        reference operator*() const {
            return *addr();
        }
        pointer operator->() const {
            return addr();
        }
        reference operator[](ssize_t n) const {
            return *(*this + n);
        }
        iterator_type& operator++() {
            ++_i;
            return *this;
        }
        iterator_type operator++(int) {
            auto x = *this;
            ++_i;
            return x;
        }
        iterator_type& operator--() {
            --_i;
            return *this;
        }
        iterator_type operator--(int) {
            auto x = *this;
            --_i;
            return x;
        }
        iterator_type& operator+=(ssize_t n) {
            _i += n;
            return *this;
        }
        iterator_type& operator-=(ssize_t n) {
            _i -= n;
            return *this;
        }
        iterator_type operator+(ssize_t n) const {
            auto x = *this;
            return x += n;
        }
        iterator_type operator-(ssize_t n) const {
            auto x = *this;
            return x -= n;
        }
        friend iterator_type operator+(ssize_t n, iterator_type a) {
            return a + n;
        }
        friend ssize_t operator-(iterator_type a, iterator_type b) {
            return a._i - b._i;
        }
        bool operator==(iterator_type x) const {
            return _i == x._i;
        }
        bool operator!=(iterator_type x) const {
            return _i != x._i;
        }
        bool operator<(iterator_type x) const {
            return _i < x._i;
        }
        bool operator<=(iterator_type x) const {
            return _i <= x._i;
        }
        bool operator>(iterator_type x) const {
            return _i > x._i;
        }
        bool operator>=(iterator_type x) const {
            return _i >= x._i;
        }
        friend class chunked_vector;
    };
    using iterator = iterator_type<T>;
    using const_iterator = iterator_type<const T>;
public:
    const T& front() const { return *cbegin(); }
    T& front() { return *begin(); }
    iterator begin() { return iterator(_chunks.data(), 0); }
    iterator end() { return iterator(_chunks.data(), _size); }
    const_iterator begin() const { return const_iterator(_chunks.data(), 0); }
    const_iterator end() const { return const_iterator(_chunks.data(), _size); }
    const_iterator cbegin() const { return const_iterator(_chunks.data(), 0); }
    const_iterator cend() const { return const_iterator(_chunks.data(), _size); }
    std::reverse_iterator<iterator> rbegin() { return std::reverse_iterator(end()); }
    std::reverse_iterator<iterator> rend() { return std::reverse_iterator(begin()); }
    std::reverse_iterator<const_iterator> rbegin() const { return std::reverse_iterator(end()); }
    std::reverse_iterator<const_iterator> rend() const { return std::reverse_iterator(begin()); }
    std::reverse_iterator<const_iterator> crbegin() const { return std::reverse_iterator(cend()); }
    std::reverse_iterator<const_iterator> crend() const { return std::reverse_iterator(cbegin()); }
public:
    bool operator==(const chunked_vector& x) const {
        return boost::equal(*this, x);
    }
    bool operator!=(const chunked_vector& x) const {
        return !operator==(x);
    }
};
}
template<typename Iterator>
static inline
sstring join(sstring delimiter, Iterator begin, Iterator end) {
    std::ostringstream oss;
    while (begin != end) {
        oss << *begin;
        ++begin;
        if (begin != end) {
            oss << delimiter;
        }
    }
    return oss.str();
}
template<typename PrintableRange>
static inline
sstring join(sstring delimiter, const PrintableRange& items) {
    return join(delimiter, items.begin(), items.end());
}
template<bool NeedsComma, typename Printable>
struct print_with_comma {
    const Printable& v;
};
template<bool NeedsComma, typename Printable>
std::ostream& operator<<(std::ostream& os, const print_with_comma<NeedsComma, Printable>& x) {
    os << x.v;
    if (NeedsComma) {
        os << ", ";
    }
    return os;
}
namespace std {
template<typename Printable>
static inline
sstring
to_string(const std::vector<Printable>& items) {
    return "[" + join(", ", items) + "]";
}
template<typename Printable>
static inline
sstring
to_string(const std::set<Printable>& items) {
    return "{" + join(", ", items) + "}";
}
template<typename Printable>
static inline
sstring
to_string(const std::unordered_set<Printable>& items) {
    return "{" + join(", ", items) + "}";
}
template<typename Printable>
static inline
sstring
to_string(std::initializer_list<Printable> items) {
    return "[" + join(", ", std::begin(items), std::end(items)) + "]";
}
template <typename K, typename V>
std::ostream& operator<<(std::ostream& os, const std::pair<K, V>& p) {
    os << "{" << p.first << ", " << p.second << "}";
    return os;
}
template<typename... T, size_t... I>
std::ostream& print_tuple(std::ostream& os, const std::tuple<T...>& p, std::index_sequence<I...>) {
    return ((os << "{" ) << ... << print_with_comma<I < sizeof...(I) - 1, T>{std::get<I>(p)}) << "}";
}
template <typename... T>
std::ostream& operator<<(std::ostream& os, const std::tuple<T...>& p) {
    return print_tuple(os, p, std::make_index_sequence<sizeof...(T)>());
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::unordered_set<T>& items) {
    os << "{" << join(", ", items) << "}";
    return os;
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& items) {
    os << "{" << join(", ", items) << "}";
    return os;
}
template<typename T, size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& items) {
    os << "{" << join(", ", items) << "}";
    return os;
}
template <typename K, typename V, typename... Args>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<K, V, Args...>& items) {
    os << "{" << join(", ", items) << "}";
    return os;
}
template <typename K, typename V, typename... Args>
std::ostream& operator<<(std::ostream& os, const std::map<K, V, Args...>& items) {
    os << "{" << join(", ", items) << "}";
    return os;
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const utils::chunked_vector<T>& items) {
    os << "[" << join(", ", items) << "]";
    return os;
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::list<T>& items) {
    os << "[" << join(", ", items) << "]";
    return os;
}
template <typename T>
std::ostream& operator<<(std::ostream& os, const std::optional<T>& opt) {
    if (opt) {
        os << "{" << *opt << "}";
    } else {
        os << "{}";
    }
    return os;
}
}
template <class Value, class Tag>
class cql_duration_counter final {
public:
    using value_type = Value;
    explicit constexpr cql_duration_counter(value_type count) noexcept : _count(count) {}
    constexpr operator value_type() const noexcept { return _count; }
private:
    value_type _count;
};
using months_counter = cql_duration_counter<int32_t, struct month_tag>;
using days_counter = cql_duration_counter<int32_t, struct day_tag>;
using nanoseconds_counter = cql_duration_counter<int64_t, struct nanosecond_tag>;
class cql_duration_error : public std::invalid_argument {
public:
    explicit cql_duration_error(std::string_view what) : std::invalid_argument(what.data()) {}
    virtual ~cql_duration_error() = default;
};
class cql_duration final {
public:
    using common_counter_type = int64_t;
    static_assert(
            (sizeof(common_counter_type) >= sizeof(months_counter::value_type)) &&
            (sizeof(common_counter_type) >= sizeof(days_counter::value_type)) &&
            (sizeof(common_counter_type) >= sizeof(nanoseconds_counter::value_type)),
            "The common counter type is smaller than one of the component counter types.");
    constexpr cql_duration() noexcept = default;
    constexpr cql_duration(months_counter m, days_counter d, nanoseconds_counter n) noexcept :
            months(m),
            days(d),
            nanoseconds(n) {}
    explicit cql_duration(std::string_view s);
    months_counter::value_type months{0};
    days_counter::value_type days{0};
    nanoseconds_counter::value_type nanoseconds{0};
};
std::ostream& operator<<(std::ostream& os, const cql_duration& d);
seastar::sstring to_string(const cql_duration&);
bool operator==(const cql_duration&, const cql_duration&) noexcept;
bool operator!=(const cql_duration&, const cql_duration&) noexcept;
class marshal_exception : public std::exception {
    sstring _why;
public:
    marshal_exception() = delete;
    marshal_exception(sstring why) : _why(sstring("marshaling error: ") + why) {}
    virtual const char* what() const noexcept override { return _why.c_str(); }
};
#include <seastar/net/ip.hh>
#include <seastar/util/backtrace.hh>
namespace seastar { class logger; }
typedef std::function<bool (const std::system_error &)> system_error_lambda_t;
bool check_exception(system_error_lambda_t f);
bool is_system_error_errno(int err_no);
bool is_timeout_exception(std::exception_ptr e);
class storage_io_error : public std::exception {
private:
    std::error_code _code;
    std::string _what;
public:
    storage_io_error(std::system_error& e) noexcept
        : _code{e.code()}
        , _what{std::string("Storage I/O error: ") + std::to_string(e.code().value()) + ": " + e.what()}
    { }
    virtual const char* what() const noexcept override {
        return _what.c_str();
    }
    const std::error_code& code() const { return _code; }
};
class tuple_type_impl;
class big_decimal;
namespace cql3 {
class cql3_type;
class column_specification;
}
enum class lexicographical_relation : int8_t {
    before_all_prefixed,
    before_all_strictly_prefixed,
    after_all_prefixed
};
template <typename TypesIterator, typename InputIt1, typename InputIt2, typename Compare>
int prefix_equality_tri_compare(TypesIterator types, InputIt1 first1, InputIt1 last1,
        InputIt2 first2, InputIt2 last2, Compare comp);
template <typename TypesIterator, typename InputIt1, typename InputIt2, typename Equality>
bool is_prefixed_by(TypesIterator types, InputIt1 first1, InputIt1 last1,
        InputIt2 first2, InputIt2 last2, Equality equality);
struct runtime_exception : public std::exception {
public:
    runtime_exception(sstring why);
    virtual const char* what() const noexcept override;
};
struct empty_t {};
class empty_value_exception : public std::exception {
public:
    virtual const char* what() const noexcept override;
};
[[noreturn]] void on_types_internal_error(const sstring& reason);
template <typename T>
class emptyable {
    static_assert(std::is_default_constructible<T>::value, "must be default constructible");
public:
    emptyable();
    emptyable(const T& x);
    emptyable(T&& x);
    emptyable(empty_t);
    template <typename... U>
    emptyable(U&&... args);
    bool empty() const;
    operator const T& () const;
    operator T&& () &&;
    const T& get() const &;
    T&& get() &&;
};
template <typename T>
inline
bool
operator==(const emptyable<T>& me1, const emptyable<T>& me2);
template <typename T>
inline
bool
operator<(const emptyable<T>& me1, const emptyable<T>& me2);
template <typename T>
class has_empty {
    template <typename X>
    constexpr static auto check(const X* x) -> std::enable_if_t<std::is_same<bool, decltype(x->empty())>::value, bool> {
        return true;
    }
    template <typename X>
    constexpr static auto check(...) -> bool {
        return false;
    }
public:
    constexpr static bool value = check<T>(nullptr);
};
template <typename T>
using maybe_empty =
        std::conditional_t<has_empty<T>::value, T, emptyable<T>>;
class abstract_type;
class data_value;
struct ascii_native_type {
    using primary_type = sstring;
    primary_type string;
};
struct simple_date_native_type {
    using primary_type = uint32_t;
    primary_type days;
};
struct date_type_native_type {
    using primary_type = db_clock::time_point;
    primary_type tp;
};
struct time_native_type {
    using primary_type = int64_t;
    primary_type nanoseconds;
};
struct timeuuid_native_type {
    using primary_type = utils::UUID;
    primary_type uuid;
};
using data_type = shared_ptr<const abstract_type>;
template <typename T>
const T& value_cast(const data_value& value);
template <typename T>
T&& value_cast(data_value&& value);
class data_value {
    void* _value;  
    data_type _type;
private:
    data_value(void* value, data_type type) : _value(value), _type(std::move(type)) {}
    template <typename T>
    static data_value make_new(data_type type, T&& value);
public:
    ~data_value();
    data_value(const data_value&);
    data_value(data_value&& x) noexcept : _value(x._value), _type(std::move(x._type)) {
        x._value = nullptr;
    }
    explicit data_value(bytes);
    data_value(sstring&&);
    data_value(std::string_view);
    data_value(const char*);
    data_value(const std::string&);
    data_value(const sstring&);
    data_value(ascii_native_type);
    data_value(bool);
    data_value(int8_t);
    data_value(int16_t);
    data_value(int32_t);
    data_value(int64_t);
    data_value(utils::UUID);
    data_value(float);
    data_value(double);
    data_value(net::ipv4_address);
    data_value(net::ipv6_address);
    data_value(seastar::net::inet_address);
    data_value(simple_date_native_type);
    data_value(db_clock::time_point);
    data_value(time_native_type);
    data_value(timeuuid_native_type);
    data_value(date_type_native_type);
    data_value(big_decimal);
    data_value(cql_duration);
    explicit data_value(std::optional<bytes>);
    template <typename NativeType>
    data_value(std::optional<NativeType>);
    template <typename NativeType>
    data_value(const std::unordered_set<NativeType>&);
    data_value& operator=(const data_value&);
    data_value& operator=(data_value&&);
    const data_type& type() const {
        return _type;
    }
    bool is_null() const {   
        return !_value;
    }
    size_t serialized_size() const;
    void serialize(bytes::iterator& out) const;
    bytes_opt serialize() const;
    bytes serialize_nonnull() const;
    friend bool operator==(const data_value& x, const data_value& y);
    friend inline bool operator!=(const data_value& x, const data_value& y);
    friend class abstract_type;
    static data_value make_null(data_type type) {
        return data_value(nullptr, std::move(type));
    }
    template <typename T>
    static data_value make(data_type type, std::unique_ptr<T> value) {
        return data_value(value.release(), std::move(type));
    }
    friend class empty_type_impl;
    template <typename T> friend const T& value_cast(const data_value&);
    template <typename T> friend T&& value_cast(data_value&&);
    friend std::ostream& operator<<(std::ostream&, const data_value&);
    friend data_value make_tuple_value(data_type, maybe_empty<std::vector<data_value>>);
    friend data_value make_set_value(data_type, maybe_empty<std::vector<data_value>>);
    friend data_value make_list_value(data_type, maybe_empty<std::vector<data_value>>);
    friend data_value make_map_value(data_type, maybe_empty<std::vector<std::pair<data_value, data_value>>>);
    friend data_value make_user_value(data_type, std::vector<data_value>);
    template <typename Func>
    friend inline auto visit(const data_value& v, Func&& f);
};
template<typename T>
inline bytes serialized(T v) {
    return data_value(v).serialize_nonnull();
}
class serialized_compare;
class serialized_tri_compare;
class user_type_impl;
class abstract_type : public enable_shared_from_this<abstract_type> {
    sstring _name;
    std::optional<uint32_t> _value_length_if_fixed;
public:
    enum class kind : int8_t {
        ascii,
        boolean,
        byte,
        bytes,
        counter,
        date,
        decimal,
        double_kind,
        duration,
        empty,
        float_kind,
        inet,
        int32,
        list,
        long_kind,
        map,
        reversed,
        set,
        short_kind,
        simple_date,
        time,
        timestamp,
        timeuuid,
        tuple,
        user,
        utf8,
        uuid,
        varint,
    };
private:
    kind _kind;
public:
    kind get_kind() const { return _kind; }
    virtual ~abstract_type();
    bool less(bytes_view v1, bytes_view v2) const;
    serialized_compare as_less_comparator() const ;
    serialized_tri_compare as_tri_comparator() const ;
    static data_type parse_type(const sstring& name);
    size_t hash(bytes_view v) const;
    bool equal(bytes_view v1, bytes_view v2) const;
    int32_t compare(bytes_view v1, bytes_view v2) const;
    data_value deserialize(bytes_view v) const;
    data_value deserialize_value(bytes_view v) const;
    void validate(bytes_view v, cql_serialization_format sf) const;
    virtual void validate(const fragmented_temporary_buffer::view& view, cql_serialization_format sf) const;
    bool is_compatible_with(const abstract_type& previous) const;
    /*
     * Types which are wrappers over other types return the inner type.
     * For example the reversed_type returns the type it is reversing.
     */
    shared_ptr<const abstract_type> underlying_type() const;
    /**
     * Returns true if values of the other AbstractType can be read and "reasonably" interpreted by the this
     * AbstractType. Note that this is a weaker version of isCompatibleWith, as it does not require that both type
     * compare values the same way.
     *
     * The restriction on the other type being "reasonably" interpreted is to prevent, for example, IntegerType from
     * being compatible with all other types.  Even though any byte string is a valid IntegerType value, it doesn't
     * necessarily make sense to interpret a UUID or a UTF8 string as an integer.
     *
     * Note that a type should be compatible with at least itself.
     */
    bool is_value_compatible_with(const abstract_type& other) const;
    bool references_user_type(const sstring& keyspace, const bytes& name) const;
    std::optional<data_type> update_user_type(const shared_ptr<const user_type_impl> updated) const;
    bool references_duration() const;
    std::optional<uint32_t> value_length_if_fixed() const;
public:
    bytes decompose(const data_value& value) const;
    const sstring& name() const;
    bool is_byte_order_equal() const;
    sstring get_string(const bytes& b) const;
    sstring to_string(bytes_view bv) const;
    sstring to_string(const bytes& b) const;
    sstring to_string_impl(const data_value& v) const;
    bytes from_string(sstring_view text) const;
    bool is_counter() const;
    bool is_string() const;
    bool is_collection() const;
    bool is_map() const;
    bool is_set() const;
    bool is_list() const;
    bool is_listlike() const;
    bool is_multi_cell() const;
    bool is_atomic() const;
    bool is_reversed() const;
    bool is_tuple() const;
    bool is_user_type() const;
    bool is_native() const;
    cql3::cql3_type as_cql3_type() const;
    const sstring& cql3_type_name() const;
    virtual shared_ptr<const abstract_type> freeze() const;
    friend class list_type_impl;
private:
    mutable sstring _cql3_type_name;
protected:
    void* native_value_clone(const void* from) const;
    const std::type_info& native_typeid() const;
    static const void* get_value_ptr(const data_value& v) {
        return v._value;
    }
    friend void write_collection_value(bytes::iterator& out, cql_serialization_format sf, data_type type, const data_value& value);
    friend class tuple_type_impl;
    friend class data_value;
    friend class reversed_type_impl;
    template <typename T> friend const T& value_cast(const data_value& value);
    template <typename T> friend T&& value_cast(data_value&& value);
    friend bool operator==(const abstract_type& x, const abstract_type& y);
};
bool operator==(const abstract_type& x, const abstract_type& y);
template <typename T>
const T& value_cast(const data_value& value);
template <typename T>
T&& value_cast(data_value&& value);
template <typename NativeType, typename AbstractType = abstract_type>
class concrete_type : public AbstractType {
public:
    using native_type = maybe_empty<NativeType>;
    using AbstractType::AbstractType;
public:
    data_value make_value(std::unique_ptr<native_type> value) const {
        return data_value::make(this->shared_from_this(), std::move(value));
    }
    data_value make_value(native_type value) const {
        return make_value(std::make_unique<native_type>(std::move(value)));
    }
    data_value make_null() const {
        return data_value::make_null(this->shared_from_this());
    }
    data_value make_empty() const {
        return make_value(native_type(empty_t()));
    }
    const native_type& from_value(const void* v) const {
        return *reinterpret_cast<const native_type*>(v);
    }
    const native_type& from_value(const data_value& v) const {
        return this->from_value(AbstractType::get_value_ptr(v));
    }
    friend class abstract_type;
};
bool operator==(const data_value& x, const data_value& y);
inline bool operator!=(const data_value& x, const data_value& y)
{
    return !(x == y);
}
using bytes_view_opt = std::optional<bytes_view>;
static inline
bool optional_less_compare(data_type t, bytes_view_opt e1, bytes_view_opt e2) {
    if (bool(e1) != bool(e2)) {
        return bool(e2);
    }
    if (!e1) {
        return false;
    }
    return t->less(*e1, *e2);
}
static inline
bool optional_equal(data_type t, bytes_view_opt e1, bytes_view_opt e2) {
    if (bool(e1) != bool(e2)) {
        return false;
    }
    if (!e1) {
        return true;
    }
    return t->equal(*e1, *e2);
}
static inline
bool less_compare(data_type t, bytes_view e1, bytes_view e2) {
    return t->less(e1, e2);
}
static inline
int tri_compare(data_type t, bytes_view e1, bytes_view e2) {
    try {
        return t->compare(e1, e2);
    } catch (const marshal_exception& e) {
        on_types_internal_error(e.what());
    }
}
inline
int
tri_compare_opt(data_type t, bytes_view_opt v1, bytes_view_opt v2) {
    if (!v1 || !v2) {
        return int(bool(v1)) - int(bool(v2));
    } else {
        return tri_compare(std::move(t), *v1, *v2);
    }
}
static inline
bool equal(data_type t, bytes_view e1, bytes_view e2) {
    return t->equal(e1, e2);
}
class row_tombstone;
class collection_type_impl;
using collection_type = shared_ptr<const collection_type_impl>;
template <typename... T>
struct simple_tuple_hash;
template <>
struct simple_tuple_hash<> {
    size_t operator()() const { return 0; }
};
template <typename Arg0, typename... Args >
struct simple_tuple_hash<std::vector<Arg0>, Args...> {
    size_t operator()(const std::vector<Arg0>& vec, const Args&... args) const {
        size_t h0 = 0;
        size_t h1;
        for (auto&& i : vec) {
            h1 = std::hash<Arg0>()(i);
            h0 = h0 ^ ((h1 << 7) | (h1 >> (std::numeric_limits<size_t>::digits - 7)));
        }
        h1 = simple_tuple_hash<Args...>()(args...);
        return h0 ^ ((h1 << 7) | (h1 >> (std::numeric_limits<size_t>::digits - 7)));
    }
};
template <typename Arg0, typename... Args>
struct simple_tuple_hash<Arg0, Args...> {
    size_t operator()(const Arg0& arg0, const Args&... args) const {
        size_t h0 = std::hash<Arg0>()(arg0);
        size_t h1 = simple_tuple_hash<Args...>()(args...);
        return h0 ^ ((h1 << 7) | (h1 >> (std::numeric_limits<size_t>::digits - 7)));
    }
};
template <typename InternedType, typename... BaseTypes>
class type_interning_helper {
    using key_type = std::tuple<BaseTypes...>;
    using value_type = shared_ptr<const InternedType>;
    struct hash_type {
        size_t operator()(const key_type& k) const {
            return apply(simple_tuple_hash<BaseTypes...>(), k);
        }
    };
    using map_type = std::unordered_map<key_type, value_type, hash_type>;
    static thread_local map_type _instances;
public:
    static shared_ptr<const InternedType> get_instance(BaseTypes... keys) {
        auto key = std::make_tuple(keys...);
        auto i = _instances.find(key);
        if (i == _instances.end()) {
            auto v = ::make_shared<InternedType>(std::move(keys)...);
            i = _instances.insert(std::make_pair(std::move(key), std::move(v))).first;
        }
        return i->second;
    }
};
template <typename InternedType, typename... BaseTypes>
thread_local typename type_interning_helper<InternedType, BaseTypes...>::map_type
    type_interning_helper<InternedType, BaseTypes...>::_instances;
class reversed_type_impl : public abstract_type {
    using intern = type_interning_helper<reversed_type_impl, data_type>;
    friend struct shared_ptr_make_helper<reversed_type_impl, true>;
    data_type _underlying_type;
    reversed_type_impl(data_type t);
public:
    const data_type& underlying_type() const {
        return _underlying_type;
    }
    static shared_ptr<const reversed_type_impl> get_instance(data_type type) {
        return intern::get_instance(std::move(type));
    }
};
using reversed_type = shared_ptr<const reversed_type_impl>;
class map_type_impl;
using map_type = shared_ptr<const map_type_impl>;
class set_type_impl;
using set_type = shared_ptr<const set_type_impl>;
class list_type_impl;
using list_type = shared_ptr<const list_type_impl>;
inline
size_t hash_value(const shared_ptr<const abstract_type>& x) {
    return std::hash<const abstract_type*>()(x.get());
}
template <typename Type>
shared_ptr<const abstract_type> data_type_for();
class serialized_compare {
    data_type _type;
public:
    serialized_compare(data_type type) : _type(type) {}
    bool operator()(const bytes& v1, const bytes& v2) const {
        return _type->less(v1, v2);
    }
};
inline
serialized_compare
abstract_type::as_less_comparator() const {
    return serialized_compare(shared_from_this());
}
class serialized_tri_compare {
    data_type _type;
public:
    serialized_tri_compare(data_type type) : _type(type) {}
    int operator()(const bytes_view& v1, const bytes_view& v2) const {
        return _type->compare(v1, v2);
    }
};
inline
serialized_tri_compare
abstract_type::as_tri_comparator() const {
    return serialized_tri_compare(shared_from_this());
}
using key_compare = serialized_compare;
extern thread_local const shared_ptr<const abstract_type> byte_type;
extern thread_local const shared_ptr<const abstract_type> short_type;
extern thread_local const shared_ptr<const abstract_type> int32_type;
extern thread_local const shared_ptr<const abstract_type> long_type;
extern thread_local const shared_ptr<const abstract_type> ascii_type;
extern thread_local const shared_ptr<const abstract_type> bytes_type;
extern thread_local const shared_ptr<const abstract_type> utf8_type;
extern thread_local const shared_ptr<const abstract_type> boolean_type;
extern thread_local const shared_ptr<const abstract_type> date_type;
extern thread_local const shared_ptr<const abstract_type> timeuuid_type;
extern thread_local const shared_ptr<const abstract_type> timestamp_type;
extern thread_local const shared_ptr<const abstract_type> simple_date_type;
extern thread_local const shared_ptr<const abstract_type> time_type;
extern thread_local const shared_ptr<const abstract_type> uuid_type;
extern thread_local const shared_ptr<const abstract_type> inet_addr_type;
extern thread_local const shared_ptr<const abstract_type> float_type;
extern thread_local const shared_ptr<const abstract_type> double_type;
extern thread_local const shared_ptr<const abstract_type> varint_type;
extern thread_local const shared_ptr<const abstract_type> decimal_type;
extern thread_local const shared_ptr<const abstract_type> counter_type;
extern thread_local const shared_ptr<const abstract_type> duration_type;
extern thread_local const data_type empty_type;
template <>
inline
shared_ptr<const abstract_type> data_type_for<int8_t>() {
    return byte_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<int16_t>() {
    return short_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<int32_t>() {
    return int32_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<int64_t>() {
    return long_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<sstring>() {
    return utf8_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<bytes>() {
    return bytes_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<utils::UUID>() {
    return uuid_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<date_type_native_type>() {
    return date_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<simple_date_native_type>() {
    return simple_date_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<db_clock::time_point>() {
    return timestamp_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<ascii_native_type>() {
    return ascii_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<time_native_type>() {
    return time_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<timeuuid_native_type>() {
    return timeuuid_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<net::inet_address>() {
    return inet_addr_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<bool>() {
    return boolean_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<float>() {
    return float_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<double>() {
    return double_type;
}
template <>
inline
shared_ptr<const abstract_type> data_type_for<big_decimal>() {
    return decimal_type;
}
namespace std {
}
bytes
to_bytes(const char* x);
bytes
to_bytes(const std::string& x);
bytes_view
to_bytes_view(const std::string& x);
bytes
to_bytes(bytes_view x);
bytes_opt
to_bytes_opt(bytes_view_opt bv);
std::vector<bytes_opt> to_bytes_opt_vec(const std::vector<bytes_view_opt>&);
bytes_view_opt
as_bytes_view_opt(const bytes_opt& bv);
bytes
to_bytes(const sstring& x);
bytes_view
to_bytes_view(const sstring& x);
bytes
to_bytes(const utils::UUID& uuid) {
    struct {
        uint64_t msb;
        uint64_t lsb;
    } tmp = { net::hton(uint64_t(uuid.get_most_significant_bits())),
        net::hton(uint64_t(uuid.get_least_significant_bits())) };
    return bytes(reinterpret_cast<int8_t*>(&tmp), 16);
}
template <typename T>
struct comparator {
    comparator() = default;
    comparator(std::function<int32_t (T& v1, T& v2)> fn)
        : _compare_fn(std::move(fn))
    { }
    int32_t compare() { return _compare_fn(); }
private:
    std::function<int32_t (T& v1, T& v2)> _compare_fn;
};
inline bool
less_unsigned(bytes_view v1, bytes_view v2) {
    return compare_unsigned(v1, v2) < 0;
}
class serialized_hash {
private:
    data_type _type;
public:
    serialized_hash(data_type type) : _type(type) {}
    size_t operator()(const bytes& v) const {
        return _type->hash(v);
    }
};
class serialized_equal {
private:
    data_type _type;
public:
    serialized_equal(data_type type) : _type(type) {}
    bool operator()(const bytes& v1, const bytes& v2) const {
        return _type->equal(v1, v2);
    }
};
template<typename Type>
static inline
typename Type::value_type deserialize_value(Type& t, bytes_view v) {
    return t.deserialize_value(v);
}
template<typename T>
T read_simple(bytes_view& v) {
    if (v.size() < sizeof(T)) {
        throw_with_backtrace<marshal_exception>(format("read_simple - not enough bytes (expected {:d}, got {:d})", sizeof(T), v.size()));
    }
    auto p = v.begin();
    v.remove_prefix(sizeof(T));
    return net::ntoh(*reinterpret_cast<const net::packed<T>*>(p));
}
template<typename T>
T read_simple_exactly(bytes_view v) {
    if (v.size() != sizeof(T)) {
        throw_with_backtrace<marshal_exception>(format("read_simple_exactly - size mismatch (expected {:d}, got {:d})", sizeof(T), v.size()));
    }
    auto p = v.begin();
    return net::ntoh(*reinterpret_cast<const net::packed<T>*>(p));
}
bytes_view
read_simple_bytes(bytes_view& v, size_t n);
template<typename T>
std::optional<T> read_simple_opt(bytes_view& v);
sstring read_simple_short_string(bytes_view& v);
size_t collection_size_len(cql_serialization_format sf);
size_t collection_value_len(cql_serialization_format sf);
void write_collection_size(bytes::iterator& out, int size, cql_serialization_format sf);
void write_collection_value(bytes::iterator& out, cql_serialization_format sf, bytes_view val_bytes);
void write_collection_value(bytes::iterator& out, cql_serialization_format sf, data_type type, const data_value& value);
using user_type = shared_ptr<const user_type_impl>;
using tuple_type = shared_ptr<const tuple_type_impl>;
#include <boost/range/adaptor/transformed.hpp>
namespace unimplemented {
enum class cause {
    API,
    INDEXES,
    LWT,
    PAGING,
    AUTH,
    PERMISSIONS,
    TRIGGERS,
    COUNTERS,
    METRICS,
    MIGRATIONS,
    GOSSIP,
    TOKEN_RESTRICTION,
    LEGACY_COMPOSITE_KEYS,
    COLLECTION_RANGE_TOMBSTONES,
    RANGE_DELETES,
    THRIFT,
    VALIDATION,
    REVERSED,
    COMPRESSION,
    NONATOMIC,
    CONSISTENCY,
    HINT,
    SUPER,
    WRAP_AROUND, 
    STORAGE_SERVICE,
    SCHEMA_CHANGE,
    MIXED_CF,
    SSTABLE_FORMAT_M,
};
[[noreturn]] void fail(cause what);
void warn(cause what);
}
namespace std {
template <>
struct hash<unimplemented::cause> : seastar::enum_hash<unimplemented::cause> {};
}
enum class allow_prefixes { no, yes };
template<allow_prefixes AllowPrefixes = allow_prefixes::no>
class compound_type final {
private:
    const std::vector<data_type> _types;
    const bool _byte_order_equal;
    const bool _byte_order_comparable;
    const bool _is_reversed;
public:
    static constexpr bool is_prefixable = AllowPrefixes == allow_prefixes::yes;
    using prefix_type = compound_type<allow_prefixes::yes>;
    using value_type = std::vector<bytes>;
    using size_type = uint16_t;
    compound_type(std::vector<data_type> types)
        : _types(std::move(types))
        , _byte_order_equal(std::all_of(_types.begin(), _types.end(), [] (auto t) {
                return t->is_byte_order_equal();
            }))
        , _byte_order_comparable(false)
        , _is_reversed(_types.size() == 1 && _types[0]->is_reversed())
    { }
    compound_type(compound_type&&) = default;
    auto const& types() const {
        return _types;
    }
    bool is_singular() const {
        return _types.size() == 1;
    }
    prefix_type as_prefix() {
        return prefix_type(_types);
    }
private:
    /*
     * Format:
     *   <len(value1)><value1><len(value2)><value2>...<len(value_n)><value_n>
     *
     */
    template<typename RangeOfSerializedComponents, typename CharOutputIterator>
    static void serialize_value(RangeOfSerializedComponents&& values, CharOutputIterator& out) {
        for (auto&& val : values) {
            assert(val.size() <= std::numeric_limits<size_type>::max());
            write<size_type>(out, size_type(val.size()));
            out = std::copy(val.begin(), val.end(), out);
        }
    }
    template <typename RangeOfSerializedComponents>
    static size_t serialized_size(RangeOfSerializedComponents&& values) {
        size_t len = 0;
        for (auto&& val : values) {
            len += sizeof(size_type) + val.size();
        }
        return len;
    }
public:
    bytes serialize_single(bytes&& v) {
        return serialize_value({std::move(v)});
    }
    template<typename RangeOfSerializedComponents>
    static bytes serialize_value(RangeOfSerializedComponents&& values);
    template<typename T>
    static bytes serialize_value(std::initializer_list<T> values);
    bytes serialize_optionals(const std::vector<bytes_opt>& values);
    bytes serialize_value_deep(const std::vector<data_value>& values);
    bytes decompose_value(const value_type& values);
    class iterator : public std::iterator<std::input_iterator_tag, const bytes_view> {
    private:
        bytes_view _v;
        bytes_view _current;
    private:
        void read_current() {
            size_type len;
            {
                if (_v.empty()) {
                    _v = bytes_view(nullptr, 0);
                    return;
                }
                len = read_simple<size_type>(_v);
                if (_v.size() < len) {
                    throw_with_backtrace<marshal_exception>(format("compound_type iterator - not enough bytes, expected {:d}, got {:d}", len, _v.size()));
                }
            }
            _current = bytes_view(_v.begin(), len);
            _v.remove_prefix(len);
        }
    public:
        struct end_iterator_tag {};
        iterator(const bytes_view& v) : _v(v) {
            read_current();
        }
        iterator(end_iterator_tag, const bytes_view& v) : _v(nullptr, 0) {}
        iterator& operator++() {
            read_current();
            return *this;
        }
        iterator operator++(int) {
            iterator i(*this);
            ++(*this);
            return i;
        }
        const value_type& operator*() const { return _current; }
        const value_type* operator->() const { return &_current; }
        bool operator!=(const iterator& i) const { return _v.begin() != i._v.begin(); }
        bool operator==(const iterator& i) const { return _v.begin() == i._v.begin(); }
    };
    static iterator begin(const bytes_view& v) {
        return iterator(v);
    }
    static iterator end(const bytes_view& v) {
        return iterator(typename iterator::end_iterator_tag(), v);
    }
    static boost::iterator_range<iterator> components(const bytes_view& v) {
        return { begin(v), end(v) };
    }
    value_type deserialize_value(bytes_view v) {
        std::vector<bytes> result;
        result.reserve(_types.size());
        std::transform(begin(v), end(v), std::back_inserter(result), [] (auto&& v) {
            return bytes(v.begin(), v.end());
        });
        return result;
    }
    bool less(bytes_view b1, bytes_view b2) {
        return compare(b1, b2) < 0;
    }
    size_t hash(bytes_view v) {
        if (_byte_order_equal) {
            return std::hash<bytes_view>()(v);
        }
        auto t = _types.begin();
        size_t h = 0;
        for (auto&& value : components(v)) {
            h ^= (*t)->hash(value);
            ++t;
        }
        return h;
    }
    int compare(bytes_view b1, bytes_view b2) {
        if (_byte_order_comparable) {
            if (_is_reversed) {
                return compare_unsigned(b2, b1);
            } else {
                return compare_unsigned(b1, b2);
            }
        }
        return lexicographical_tri_compare(_types.begin(), _types.end(),
            begin(b1), end(b1), begin(b2), end(b2), [] (auto&& type, auto&& v1, auto&& v2) {
                return type->compare(v1, v2);
            });
    }
    bool is_full(bytes_view v) const {
        assert(AllowPrefixes == allow_prefixes::yes);
        return std::distance(begin(v), end(v)) == (ssize_t)_types.size();
    }
    bool is_empty(bytes_view v) const {
        return begin(v) == end(v);
    }
    void validate(bytes_view v) {
        warn(unimplemented::cause::VALIDATION);
    }
    bool equal(bytes_view v1, bytes_view v2) {
        if (_byte_order_equal) {
            return compare_unsigned(v1, v2) == 0;
        }
        return compare(v1, v2) == 0;
    }
};
using compound_prefix = compound_type<allow_prefixes::yes>;
#include <boost/range/join.hpp>
#include <boost/dynamic_bitset.hpp>
namespace dht {
class i_partitioner;
}
using column_count_type = uint32_t;
using column_id = column_count_type;
enum class ordinal_column_id: column_count_type {};
std::ostream& operator<<(std::ostream& os, ordinal_column_id id);
class column_set {
public:
    using bitset = boost::dynamic_bitset<uint64_t>;
    using size_type = bitset::size_type;
    static_assert(static_cast<column_count_type>(boost::dynamic_bitset<uint64_t>::npos) == ~static_cast<column_count_type>(0));
    static constexpr ordinal_column_id npos = static_cast<ordinal_column_id>(bitset::npos);
    explicit column_set(column_count_type num_bits = 0)
        : _mask(num_bits)
    {
    }
    void resize(column_count_type num_bits) {
        _mask.resize(num_bits);
    }
    void set(ordinal_column_id id) {
        column_count_type bit = static_cast<column_count_type>(id);
        _mask.set(bit);
    }
    bool test(ordinal_column_id id) const {
        column_count_type bit = static_cast<column_count_type>(id);
        return _mask.test(bit);
    }
    size_type count() const { return _mask.count(); }
    ordinal_column_id find_first() const {
        return static_cast<ordinal_column_id>(_mask.find_first());
    }
    ordinal_column_id find_next(ordinal_column_id pos) const {
        return static_cast<ordinal_column_id>(_mask.find_next(static_cast<column_count_type>(pos)));
    }
    void union_with(const column_set& with) {
        _mask |= with._mask;
    }
private:
    bitset _mask;
};
using table_schema_version = utils::UUID;
class schema;
class schema_registry_entry;
class schema_builder;
namespace cell_comparator {
sstring to_sstring(const schema& s);
bool check_compound(sstring comparator);
void read_collections(schema_builder& builder, sstring comparator);
}
namespace db {
class extensions;
}
enum class column_kind { partition_key, clustering_key, static_column, regular_column };
enum class column_view_virtual { no, yes };
sstring to_sstring(column_kind k);
bool is_compatible(column_kind k1, column_kind k2);
enum class cf_type : uint8_t {
    standard,
    super,
};
inline sstring cf_type_to_sstring(cf_type t) {
    if (t == cf_type::standard) {
        return "Standard";
    } else if (t == cf_type::super) {
        return "Super";
    }
    throw std::invalid_argument(format("unknown type: {:d}\n", uint8_t(t)));
}
inline cf_type sstring_to_cf_type(sstring name) {
    if (name == "Standard") {
        return cf_type::standard;
    } else if (name == "Super") {
        return cf_type::super;
    }
    throw std::invalid_argument(format("unknown type: {}\n", name));
}
struct speculative_retry {
    enum class type {
        NONE, CUSTOM, PERCENTILE, ALWAYS
    };
private:
    type _t;
    double _v;
public:
    speculative_retry(type t, double v) : _t(t), _v(v) {}
    sstring to_sstring() const {
        if (_t == type::NONE) {
            return "NONE";
        } else if (_t == type::ALWAYS) {
            return "ALWAYS";
        } else if (_t == type::CUSTOM) {
            return format("{:.2f}ms", _v);
        } else if (_t == type::PERCENTILE) {
            return format("{:.1f}PERCENTILE", 100 * _v);
        } else {
            throw std::invalid_argument(format("unknown type: {:d}\n", uint8_t(_t)));
        }
    }
    static speculative_retry from_sstring(sstring str) {
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        sstring ms("MS");
        sstring percentile("PERCENTILE");
        auto convert = [&str] (sstring& t) {
            try {
                return boost::lexical_cast<double>(str.substr(0, str.size() - t.size()));
            } catch (boost::bad_lexical_cast& e) {
                throw std::invalid_argument(format("cannot convert {} to speculative_retry\n", str));
            }
        };
        type t;
        double v = 0;
        if (str == "NONE") {
            t = type::NONE;
        } else if (str == "ALWAYS") {
            t = type::ALWAYS;
        } else if (str.compare(str.size() - ms.size(), ms.size(), ms) == 0) {
            t = type::CUSTOM;
            v = convert(ms);
        } else if (str.compare(str.size() - percentile.size(), percentile.size(), percentile) == 0) {
            t = type::PERCENTILE;
            v = convert(percentile) / 100;
        } else {
            throw std::invalid_argument(format("cannot convert {} to speculative_retry\n", str));
        }
        return speculative_retry(t, v);
    }
    type get_type() const {
        return _t;
    }
    double get_value() const {
        return _v;
    }
    bool operator==(const speculative_retry& other) const {
        return _t == other._t && _v == other._v;
    }
    bool operator!=(const speculative_retry& other) const {
        return !(*this == other);
    }
};
typedef std::unordered_map<sstring, sstring> index_options_map;
enum class index_metadata_kind {
    keys,
    custom,
    composites,
};
class index_metadata final {
};
class schema_builder;
class thrift_schema {
    bool _compound = true;
    bool _is_dynamic = false;
public:
    bool has_compound_comparator() const;
    bool is_dynamic() const;
    friend class schema;
};
bool operator==(const column_definition&, const column_definition&);
inline bool operator!=(const column_definition& a, const column_definition& b) { return !(a == b); }
static constexpr int DEFAULT_MIN_COMPACTION_THRESHOLD = 4;
static constexpr int DEFAULT_MAX_COMPACTION_THRESHOLD = 32;
static constexpr int DEFAULT_MIN_INDEX_INTERVAL = 128;
static constexpr int DEFAULT_GC_GRACE_SECONDS = 864000;
class column_mapping_entry {
    bytes _name;
    data_type _type;
    bool _is_atomic;
public:
    column_mapping_entry(bytes name, data_type type)
        : _name(std::move(name)), _type(std::move(type)), _is_atomic(_type->is_atomic()) { }
    column_mapping_entry(bytes name, sstring type_name);
    column_mapping_entry(const column_mapping_entry&);
    column_mapping_entry& operator=(const column_mapping_entry&);
    column_mapping_entry(column_mapping_entry&&) = default;
    column_mapping_entry& operator=(column_mapping_entry&&) = default;
    const bytes& name() const { return _name; }
    const data_type& type() const { return _type; }
    const sstring& type_name() const { return _type->name(); }
    bool is_atomic() const { return _is_atomic; }
};
class column_mapping {
private:
    std::vector<column_mapping_entry> _columns;
    column_count_type _n_static = 0;
public:
    column_mapping() {}
    column_mapping(std::vector<column_mapping_entry> columns, column_count_type n_static)
            : _columns(std::move(columns))
            , _n_static(n_static)
    { }
    const std::vector<column_mapping_entry>& columns() const { return _columns; }
    column_count_type n_static() const { return _n_static; }
    const column_mapping_entry& column_at(column_kind kind, column_id id) const {
        assert(kind == column_kind::regular_column || kind == column_kind::static_column);
        return kind == column_kind::regular_column ? regular_column_at(id) : static_column_at(id);
    }
    const column_mapping_entry& static_column_at(column_id id) const {
        if (id >= _n_static) {
            throw std::out_of_range(format("static column id {:d} >= {:d}", id, _n_static));
        }
        return _columns[id];
    }
    const column_mapping_entry& regular_column_at(column_id id) const {
        auto n_regular = _columns.size() - _n_static;
        if (id >= n_regular) {
            throw std::out_of_range(format("regular column id {:d} >= {:d}", id, n_regular));
        }
        return _columns[id + _n_static];
    }
    friend std::ostream& operator<<(std::ostream& out, const column_mapping& cm);
};
/**
 * Augments a schema with fields related to materialized views.
 * Effectively immutable.
 */
class raw_view_info final {
    utils::UUID _base_id;
    sstring _base_name;
    bool _include_all_columns;
    sstring _where_clause;
public:
    raw_view_info(utils::UUID base_id, sstring base_name, bool include_all_columns, sstring where_clause);
    const utils::UUID& base_id() const {
        return _base_id;
    }
    const sstring& base_name() const {
        return _base_name;
    }
    bool include_all_columns() const {
        return _include_all_columns;
    }
    const sstring& where_clause() const {
        return _where_clause;
    }
    friend bool operator==(const raw_view_info&, const raw_view_info&);
    friend std::ostream& operator<<(std::ostream& os, const raw_view_info& view);
};
bool operator==(const raw_view_info&, const raw_view_info&);
std::ostream& operator<<(std::ostream& os, const raw_view_info& view);
class view_info;
class v3_columns {
    bool _is_dense = false;
    bool _is_compound = false;
    std::vector<column_definition> _columns;
    std::unordered_map<bytes, const column_definition*> _columns_by_name;
public:
    v3_columns(std::vector<column_definition> columns, bool is_dense, bool is_compound);
    v3_columns() = default;
    v3_columns(v3_columns&&) = default;
    v3_columns& operator=(v3_columns&&) = default;
    v3_columns(const v3_columns&) = delete;
    static v3_columns from_v2_schema(const schema&);
public:
    const std::vector<column_definition>& all_columns() const;
    const std::unordered_map<bytes, const column_definition*>& columns_by_name() const;
    bool is_static_compact() const;
    bool is_compact() const;
    void apply_to(schema_builder&) const;
};
namespace query {
class partition_slice;
}
/**
 * Schema extension. An opaque type representing
 * entries in the "extensions" part of a table/view (see schema_tables).
 *
 * An extension has a name (the mapping key), and it can re-serialize
 * itself to bytes again, when we write back into schema tables.
 *
 * Code using a particular extension can locate it by name in the schema map,
 * and barring the "is_placeholder" says true, cast it to whatever might
 * be the expeceted implementation.
 *
 * We allow placeholder object since an extension written to schema tables
 * might be unavailable on next boot/other node. To avoid loosing the config data,
 * a placeholder object is put into schema map, which at least can
 * re-serialize the data back.
 *
 */
class schema_extension {
public:
    virtual ~schema_extension() {};
    virtual bytes serialize() const = 0;
    virtual bool is_placeholder() const {
        return false;
    }
};
/*
 * Effectively immutable.
 * Not safe to access across cores because of shared_ptr's.
 * Use global_schema_ptr for safe across-shard access.
 */
class schema final : public enable_lw_shared_from_this<schema> {
    friend class v3_columns;
public:
    struct dropped_column {
        data_type type;
        api::timestamp_type timestamp;
        bool operator==(const dropped_column& rhs) const {
            return type == rhs.type && timestamp == rhs.timestamp;
        }
    };
    using extensions_map = std::map<sstring, ::shared_ptr<schema_extension>>;
private:
    struct raw_schema {
        raw_schema(utils::UUID id);
        utils::UUID _id;
        sstring _ks_name;
        sstring _cf_name;
        std::vector<column_definition> _columns;
        sstring _comment;
        gc_clock::duration _default_time_to_live = gc_clock::duration::zero();
        data_type _regular_column_name_type;
        data_type _default_validation_class = bytes_type;
        double _bloom_filter_fp_chance = 0.01;
        extensions_map _extensions;
        bool _is_dense = false;
        bool _is_compound = true;
        bool _is_counter = false;
        cf_type _type = cf_type::standard;
        int32_t _gc_grace_seconds = DEFAULT_GC_GRACE_SECONDS;
        double _dc_local_read_repair_chance = 0.1;
        double _read_repair_chance = 0.0;
        double _crc_check_chance = 1;
        int32_t _min_compaction_threshold = DEFAULT_MIN_COMPACTION_THRESHOLD;
        int32_t _max_compaction_threshold = DEFAULT_MAX_COMPACTION_THRESHOLD;
        int32_t _min_index_interval = DEFAULT_MIN_INDEX_INTERVAL;
        int32_t _max_index_interval = 2048;
        int32_t _memtable_flush_period = 0;
        speculative_retry _speculative_retry = ::speculative_retry(speculative_retry::type::PERCENTILE, 0.99);
        bool _compaction_enabled = true;
        table_schema_version _version;
        std::unordered_map<sstring, dropped_column> _dropped_columns;
        std::map<bytes, data_type> _collections;
        std::unordered_map<sstring, index_metadata> _indices_by_name;
        bool _wait_for_sync = false; 
    };
    raw_schema _raw;
    thrift_schema _thrift;
    v3_columns _v3_columns;
    mutable schema_registry_entry* _registry_entry = nullptr;
    std::unique_ptr<::view_info> _view_info;
    const std::array<column_count_type, 3> _offsets;
     column_count_type column_offset(column_kind k) const ;
    std::unordered_map<bytes, const column_definition*> _columns_by_name;
    lw_shared_ptr<compound_type<allow_prefixes::no>> _partition_key_type;
    lw_shared_ptr<compound_type<allow_prefixes::yes>> _clustering_key_type;
    column_mapping _column_mapping;
    shared_ptr<query::partition_slice> _full_slice;
    column_count_type _clustering_key_size;
    column_count_type _regular_column_count;
    column_count_type _static_column_count;
    extensions_map& extensions() ;
    friend class db::extensions;
    friend class schema_builder;
public:
    using row_column_ids_are_ordered_by_name = std::true_type;
    typedef std::vector<column_definition> columns_type;
    typedef typename columns_type::iterator iterator;
    typedef typename columns_type::const_iterator const_iterator;
    typedef boost::iterator_range<iterator> iterator_range_type;
    typedef boost::iterator_range<const_iterator> const_iterator_range_type;
    static constexpr int32_t NAME_LENGTH = 48;
    struct column {
        bytes name;
        data_type type;
    };
private:
    ::shared_ptr<cql3::column_specification> make_column_specification(const column_definition& def);
    void rebuild();
    schema(const raw_schema&, std::optional<raw_view_info>);
public:
    schema(std::optional<utils::UUID> id,
        std::string_view ks_name,
        std::string_view cf_name,
        std::vector<column> partition_key,
        std::vector<column> clustering_key,
        std::vector<column> regular_columns,
        std::vector<column> static_columns,
        data_type regular_column_name_type,
        std::string_view comment = {});
    schema(const schema&);
    ~schema();
    table_schema_version version() const ;
    double bloom_filter_fp_chance() const ;
    sstring thrift_key_validator() const;
    const extensions_map& extensions() const ;
    bool is_dense() const ;
    bool is_compound() const ;
    bool is_cql3_table() const ;
    bool is_compact_table() const ;
    bool is_static_compact_table() const ;
    thrift_schema& thrift() ;
    const thrift_schema& thrift() const ;
    const utils::UUID& id() const ;
    const sstring& comment() const ;
    bool is_counter() const ;
    const cf_type type() const ;
    bool is_super() const ;
    gc_clock::duration gc_grace_seconds() const ;
    double dc_local_read_repair_chance() const ;
    double read_repair_chance() const ;
    double crc_check_chance() const ;
    int32_t min_compaction_threshold() const ;
    int32_t max_compaction_threshold() const ;
    int32_t min_index_interval() const ;
    int32_t max_index_interval() const ;
    int32_t memtable_flush_period() const ;
    bool compaction_enabled() const ;
    const ::speculative_retry& speculative_retry() const ;
    dht::i_partitioner& get_partitioner() const;
    const column_definition* get_column_definition(const bytes& name) const;
    const column_definition& column_at(column_kind, column_id) const;
    const column_definition& column_at(ordinal_column_id ordinal_id) const;
    const_iterator regular_begin() const;
    const_iterator regular_end() const;
    const_iterator regular_lower_bound(const bytes& name) const;
    const_iterator regular_upper_bound(const bytes& name) const;
    const_iterator static_begin() const;
    const_iterator static_end() const;
    const_iterator static_lower_bound(const bytes& name) const;
    const_iterator static_upper_bound(const bytes& name) const;
    data_type column_name_type(const column_definition& def) const;
    const column_definition& clustering_column_at(column_id id) const;
    const column_definition& regular_column_at(column_id id) const;
    const column_definition& static_column_at(column_id id) const;
    bool is_last_partition_key(const column_definition& def) const;
    bool has_multi_cell_collections() const;
    bool has_static_columns() const;
    column_count_type columns_count(column_kind kind) const;
    column_count_type partition_key_size() const;
    column_count_type clustering_key_size() const;
    column_count_type static_columns_count() const;
    column_count_type regular_columns_count() const;
    column_count_type all_columns_count() const;
    const_iterator_range_type partition_key_columns() const;
    const_iterator_range_type clustering_key_columns() const;
    const_iterator_range_type static_columns() const;
    const_iterator_range_type regular_columns() const;
    typedef boost::range::joined_range<const_iterator_range_type, const_iterator_range_type>
        select_order_range;
    select_order_range all_columns_in_select_order() const;
    uint32_t position(const column_definition& column) const;
    const columns_type& all_columns() const ;
    const std::unordered_map<bytes, const column_definition*>& columns_by_name() const ;
    const auto& dropped_columns() const ;
    const auto& collections() const ;
    gc_clock::duration default_time_to_live() const ;
    data_type make_legacy_default_validator() const;
    const sstring& ks_name() const {
        return _raw._ks_name;
    }
    const sstring& cf_name() const {
        return _raw._cf_name;
    }
    const lw_shared_ptr<compound_type<allow_prefixes::no>>& partition_key_type() const {
        return _partition_key_type;
    }
    const lw_shared_ptr<compound_type<allow_prefixes::yes>>& clustering_key_type() const {
        return _clustering_key_type;
    }
    const lw_shared_ptr<compound_type<allow_prefixes::yes>>& clustering_key_prefix_type() const {
        return _clustering_key_type;
    }
    const data_type& regular_column_name_type() const {
        return _raw._regular_column_name_type;
    }
    const data_type& static_column_name_type() const {
        return utf8_type;
    }
    const std::unique_ptr<::view_info>& view_info() const {
        return _view_info;
    }
    bool is_view() const {
        return bool(_view_info);
    }
    const query::partition_slice& full_slice() const {
        return *_full_slice;
    }
    std::vector<sstring> index_names() const;
    std::vector<index_metadata> indices() const;
    const std::unordered_map<sstring, index_metadata>& all_indices() const;
    bool has_index(const sstring& index_name) const;
    std::optional<index_metadata> find_index_noname(const index_metadata& target) const;
    friend std::ostream& operator<<(std::ostream& os, const schema& s);
    std::ostream& describe(std::ostream& os) const;
    friend bool operator==(const schema&, const schema&);
    const column_mapping& get_column_mapping() const;
    friend class schema_registry_entry;
    schema_registry_entry* registry_entry() const noexcept;
    bool is_synced() const;
    bool equal_columns(const schema&) const;
    bool wait_for_sync_to_commitlog() const {
        return _raw._wait_for_sync;
    }
public:
    const v3_columns& v3() const {
        return _v3_columns;
    }
};
bool operator==(const schema&, const schema&);
using schema_ptr = lw_shared_ptr<const schema>;
class view_ptr final {
    schema_ptr _schema;
public:
    explicit view_ptr(schema_ptr schema) noexcept : _schema(schema) {
        if (schema) {
            assert(_schema->is_view());
        }
    }
    const schema& operator*() const noexcept { return *_schema; }
    const schema* operator->() const noexcept { return _schema.operator->(); }
    const schema* get() const noexcept { return _schema.get(); }
    operator schema_ptr() const noexcept {
        return _schema;
    }
    explicit operator bool() const noexcept {
        return bool(_schema);
    }
    friend std::ostream& operator<<(std::ostream& os, const view_ptr& s);
};
std::ostream& operator<<(std::ostream& os, const view_ptr& view);
utils::UUID generate_legacy_id(const sstring& ks_name, const sstring& cf_name);
class schema_mismatch_error : public std::runtime_error {
public:
    schema_mismatch_error(table_schema_version expected, const schema& access);
};
inline void check_schema_version(table_schema_version expected, const schema& access) {
    if (expected != access.version()) {
        throw_with_backtrace<schema_mismatch_error>(expected, access);
    }
}
namespace sstables {
enum class sstable_version_types { ka, la, mc };
enum class sstable_format_types { big };
inline sstable_version_types from_string(const seastar::sstring& format) {
    if (format == "ka") {
        return sstable_version_types::ka;
    }
    if (format == "la") {
        return sstable_version_types::la;
    }
    if (format == "mc") {
        return sstable_version_types::mc;
    }
    throw std::invalid_argument("Wrong sstable format name: " + format);
}
inline seastar::sstring to_string(sstable_version_types format) {
    switch (format) {
        case sstable_version_types::ka: return "ka";
        case sstable_version_types::la: return "la";
        case sstable_version_types::mc: return "mc";
    }
    throw std::runtime_error("Wrong sstable format");
}
inline bool is_latest_supported(sstable_version_types format) {
    return format == sstable_version_types::mc;
}
inline bool is_later(sstable_version_types a, sstable_version_types b) {
    auto to_int = [] (sstable_version_types x) {
        return static_cast<std::underlying_type_t<sstable_version_types>>(x);
    };
    return to_int(a) > to_int(b);
}
}
template <typename CompoundType>
class legacy_compound_view {
    static_assert(!CompoundType::is_prefixable, "Legacy view not defined for prefixes");
    CompoundType& _type;
    bytes_view _packed;
public:
    legacy_compound_view(CompoundType& c, bytes_view packed)
        : _type(c)
        , _packed(packed)
    { }
    class iterator : public std::iterator<std::input_iterator_tag, bytes::value_type> {
        bool _singular;
        int32_t _offset;
        typename CompoundType::iterator _i;
    public:
        struct end_tag {};
        iterator(const legacy_compound_view& v)
            : _singular(v._type.is_singular())
            , _offset(_singular ? 0 : -2)
            , _i(v._type.begin(v._packed))
        { }
        iterator(const legacy_compound_view& v, end_tag)
            : _offset(-2)
            , _i(v._type.end(v._packed))
        { }
        value_type operator*() const {
            int32_t component_size = _i->size();
            if (_offset == -2) {
                return (component_size >> 8) & 0xff;
            } else if (_offset == -1) {
                return component_size & 0xff;
            } else if (_offset < component_size) {
                return (*_i)[_offset];
            } else { 
                return 0; 
            }
        }
        iterator& operator++() {
            auto component_size = (int32_t) _i->size();
            if (_offset < component_size
                && (!_singular || _offset != (component_size - 1)))
            {
                ++_offset;
            } else {
                ++_i;
                _offset = -2;
            }
            return *this;
        }
        bool operator==(const iterator& other) const {
            return _offset == other._offset && other._i == _i;
        }
        bool operator!=(const iterator& other) const {
            return !(*this == other);
        }
    };
    struct tri_comparator {
        const CompoundType& _type;
        tri_comparator(const CompoundType& type)
            : _type(type)
        { }
        int operator()(bytes_view k1, bytes_view k2) const;
    };
    size_t size() const {
        if (_type.is_singular()) {
            return _type.begin(_packed)->size();
        }
        size_t s = 0;
        for (auto&& component : _type.components(_packed)) {
            s += 2  + component.size() + 1 ;
        }
        return s;
    }
    iterator begin() const {
        return iterator(*this);
    }
    iterator end() const {
        return iterator(*this, typename iterator::end_tag());
    }
};
template <typename CompoundType>
static inline
bytes to_legacy(CompoundType& type, bytes_view packed) {
    legacy_compound_view<CompoundType> lv(type, packed);
    bytes legacy_form(bytes::initialized_later(), lv.size());
    std::copy(lv.begin(), lv.end(), legacy_form.begin());
    return legacy_form;
}
class composite_view;
class composite final {
    bytes _bytes;
    bool _is_compound;
public:
    composite(bytes&& b, bool is_compound)
            : _bytes(std::move(b))
            , _is_compound(is_compound)
    { }
    explicit composite(bytes&& b)
            : _bytes(std::move(b))
            , _is_compound(true)
    { }
    composite()
            : _bytes()
            , _is_compound(true)
    { }
    using size_type = uint16_t;
    using eoc_type = int8_t;
    /*
     * The 'end-of-component' byte should always be 0 for actual column name.
     * However, it can set to 1 for query bounds. This allows to query for the
     * equivalent of 'give me the full range'. That is, if a slice query is:
     *   start = <3><"foo".getBytes()><0>
     *   end   = <3><"foo".getBytes()><1>
     * then we'll return *all* the columns whose first component is "foo".
     * If for a component, the 'end-of-component' is != 0, there should not be any
     * following component. The end-of-component can also be -1 to allow
     * non-inclusive query. For instance:
     *   end = <3><"foo".getBytes()><-1>
     * allows to query everything that is smaller than <3><"foo".getBytes()>, but
     * not <3><"foo".getBytes()> itself.
     */
    enum class eoc : eoc_type {
        start = -1,
        none = 0,
        end = 1
    };
    using component = std::pair<bytes, eoc>;
    using component_view = std::pair<bytes_view, eoc>;
private:
    template<typename Value, typename = std::enable_if_t<!std::is_same<const data_value, std::decay_t<Value>>::value>>
    static size_t size(const Value& val) {
        return val.size();
    }
    static size_t size(const data_value& val) {
        return val.serialized_size();
    }
    template<typename Value, typename CharOutputIterator, typename = std::enable_if_t<!std::is_same<data_value, std::decay_t<Value>>::value>>
    static void write_value(Value&& val, CharOutputIterator& out) {
        out = std::copy(val.begin(), val.end(), out);
    }
    template <typename CharOutputIterator>
    static void write_value(const data_value& val, CharOutputIterator& out) {
        val.serialize(out);
    }
    template<typename RangeOfSerializedComponents, typename CharOutputIterator>
    static void serialize_value(RangeOfSerializedComponents&& values, CharOutputIterator& out, bool is_compound) {
        if (!is_compound) {
            auto it = values.begin();
            write_value(std::forward<decltype(*it)>(*it), out);
            return;
        }
        for (auto&& val : values) {
            write<size_type>(out, static_cast<size_type>(size(val)));
            write_value(std::forward<decltype(val)>(val), out);
            write<eoc_type>(out, eoc_type(eoc::none));
        }
    }
    template <typename RangeOfSerializedComponents>
    static size_t serialized_size(RangeOfSerializedComponents&& values, bool is_compound) {
        size_t len = 0;
        auto it = values.begin();
        if (it != values.end()) {
            auto value_size = size(*it);
            if (value_size > static_cast<size_type>(std::numeric_limits<size_type>::max() - uint8_t(is_compound))) {
                throw std::runtime_error(format("First component size too large: {:d} > {:d}", value_size, std::numeric_limits<size_type>::max() - is_compound));
            }
            if (!is_compound) {
                return value_size;
            }
            len += sizeof(size_type) + value_size + sizeof(eoc_type);
            ++it;
        }
        for ( ; it != values.end(); ++it) {
            auto value_size = size(*it);
            if (value_size > std::numeric_limits<size_type>::max()) {
                throw std::runtime_error(format("Component size too large: {:d} > {:d}", value_size, std::numeric_limits<size_type>::max()));
            }
            len += sizeof(size_type) + value_size + sizeof(eoc_type);
        }
        return len;
    }
public:
    template <typename Describer>
    auto describe_type(sstables::sstable_version_types v, Describer f) const {
        return f(const_cast<bytes&>(_bytes));
    }
    template<typename RangeOfSerializedComponents>
    static composite serialize_value(RangeOfSerializedComponents&& values, bool is_compound = true, eoc marker = eoc::none) {
        auto size = serialized_size(values, is_compound);
        bytes b(bytes::initialized_later(), size);
        auto i = b.begin();
        serialize_value(std::forward<decltype(values)>(values), i, is_compound);
        if (is_compound && !b.empty()) {
            b.back() = eoc_type(marker);
        }
        return composite(std::move(b), is_compound);
    }
    template<typename RangeOfSerializedComponents>
    static composite serialize_static(const schema& s, RangeOfSerializedComponents&& values) {
        auto b = bytes(size_t(2), bytes::value_type(0xff));
        std::vector<bytes_view> sv(s.clustering_key_size());
        b += composite::serialize_value(boost::range::join(sv, std::forward<RangeOfSerializedComponents>(values)), true).release_bytes();
        return composite(std::move(b));
    }
    static eoc to_eoc(int8_t eoc_byte) {
        return eoc_byte == 0 ? eoc::none : (eoc_byte < 0 ? eoc::start : eoc::end);
    }
    class iterator : public std::iterator<std::input_iterator_tag, const component_view> {
        bytes_view _v;
        component_view _current;
    private:
        void read_current() {
            size_type len;
            {
                if (_v.empty()) {
                    _v = bytes_view(nullptr, 0);
                    return;
                }
                len = read_simple<size_type>(_v);
                if (_v.size() < len) {
                    throw_with_backtrace<marshal_exception>(format("composite iterator - not enough bytes, expected {:d}, got {:d}", len, _v.size()));
                }
            }
            auto value = bytes_view(_v.begin(), len);
            _v.remove_prefix(len);
            _current = component_view(std::move(value), to_eoc(read_simple<eoc_type>(_v)));
        }
    public:
        struct end_iterator_tag {};
        iterator(const bytes_view& v, bool is_compound, bool is_static)
                : _v(v) {
            if (is_static) {
                _v.remove_prefix(2);
            }
            if (is_compound) {
                read_current();
            } else {
                _current = component_view(_v, eoc::none);
                _v.remove_prefix(_v.size());
            }
        }
        iterator(end_iterator_tag) : _v(nullptr, 0) {}
        iterator& operator++() {
            read_current();
            return *this;
        }
        iterator operator++(int) {
            iterator i(*this);
            ++(*this);
            return i;
        }
        const value_type& operator*() const { return _current; }
        const value_type* operator->() const { return &_current; }
        bool operator!=(const iterator& i) const { return _v.begin() != i._v.begin(); }
        bool operator==(const iterator& i) const { return _v.begin() == i._v.begin(); }
    };
    iterator begin() const {
        return iterator(_bytes, _is_compound, is_static());
    }
    iterator end() const {
        return iterator(iterator::end_iterator_tag());
    }
    boost::iterator_range<iterator> components() const & {
        return { begin(), end() };
    }
    auto values() const & {
        return components() | boost::adaptors::transformed([](auto&& c) { return c.first; });
    }
    std::vector<component> components() const && {
        std::vector<component> result;
        std::transform(begin(), end(), std::back_inserter(result), [](auto&& p) {
            return component(bytes(p.first.begin(), p.first.end()), p.second);
        });
        return result;
    }
    std::vector<bytes> values() const && {
        std::vector<bytes> result;
        boost::copy(components() | boost::adaptors::transformed([](auto&& c) { return to_bytes(c.first); }), std::back_inserter(result));
        return result;
    }
    const bytes& get_bytes() const {
        return _bytes;
    }
    bytes release_bytes() && {
        return std::move(_bytes);
    }
    size_t size() const {
        return _bytes.size();
    }
    bool empty() const {
        return _bytes.empty();
    }
    static bool is_static(bytes_view bytes, bool is_compound) {
        return is_compound && bytes.size() > 2 && (bytes[0] & bytes[1] & 0xff) == 0xff;
    }
    bool is_static() const {
        return is_static(_bytes, _is_compound);
    }
    bool is_compound() const {
        return _is_compound;
    }
    template <typename ClusteringElement>
    static composite from_clustering_element(const schema& s, const ClusteringElement& ce) {
        return serialize_value(ce.components(s), s.is_compound());
    }
    static composite from_exploded(const std::vector<bytes_view>& v, bool is_compound, eoc marker = eoc::none) {
        if (v.size() == 0) {
            return composite(bytes(size_t(1), bytes::value_type(marker)), is_compound);
        }
        return serialize_value(v, is_compound, marker);
    }
    static composite static_prefix(const schema& s) {
        return serialize_static(s, std::vector<bytes_view>());
    }
    explicit operator bytes_view() const {
        return _bytes;
    }
    template <typename Component>
    friend inline std::ostream& operator<<(std::ostream& os, const std::pair<Component, eoc>& c) {
        return os << "{value=" << c.first << "; eoc=" << format("0x{:02x}", eoc_type(c.second) & 0xff) << "}";
    }
    friend std::ostream& operator<<(std::ostream& os, const composite& v);
    struct tri_compare {
        const std::vector<data_type>& _types;
        tri_compare(const std::vector<data_type>& types) : _types(types) {}
        int operator()(const composite&, const composite&) const;
        int operator()(composite_view, composite_view) const;
    };
};
class composite_view final {
    bytes_view _bytes;
    bool _is_compound;
public:
    composite_view(bytes_view b, bool is_compound = true)
            : _bytes(b)
            , _is_compound(is_compound)
    { }
    composite_view(const composite& c)
            : composite_view(static_cast<bytes_view>(c), c.is_compound())
    { }
    composite_view()
            : _bytes(nullptr, 0)
            , _is_compound(true)
    { }
    std::vector<bytes_view> explode() const {
        if (!_is_compound) {
            return { _bytes };
        }
        std::vector<bytes_view> ret;
        ret.reserve(8);
        for (auto it = begin(), e = end(); it != e; ) {
            ret.push_back(it->first);
            auto marker = it->second;
            ++it;
            if (it != e && marker != composite::eoc::none) {
                throw runtime_exception(format("non-zero component divider found ({:d}) mid", format("0x{:02x}", composite::eoc_type(marker) & 0xff)));
            }
        }
        return ret;
    }
    composite::iterator begin() const {
        return composite::iterator(_bytes, _is_compound, is_static());
    }
    composite::iterator end() const {
        return composite::iterator(composite::iterator::end_iterator_tag());
    }
    boost::iterator_range<composite::iterator> components() const {
        return { begin(), end() };
    }
    composite::eoc last_eoc() const {
        if (!_is_compound || _bytes.empty()) {
            return composite::eoc::none;
        }
        bytes_view v(_bytes);
        v.remove_prefix(v.size() - 1);
        return composite::to_eoc(read_simple<composite::eoc_type>(v));
    }
    auto values() const {
        return components() | boost::adaptors::transformed([](auto&& c) { return c.first; });
    }
    size_t size() const {
        return _bytes.size();
    }
    bool empty() const {
        return _bytes.empty();
    }
    bool is_static() const {
        return composite::is_static(_bytes, _is_compound);
    }
    explicit operator bytes_view() const {
        return _bytes;
    }
    bool operator==(const composite_view& k) const { return k._bytes == _bytes && k._is_compound == _is_compound; }
    bool operator!=(const composite_view& k) const { return !(k == *this); }
    friend inline std::ostream& operator<<(std::ostream& os, composite_view v) {
        return os << "{" << ::join(", ", v.components()) << ", compound=" << v._is_compound << ", static=" << v.is_static() << "}";
    }
};
inline
std::ostream& operator<<(std::ostream& os, const composite& v) {
    return os << composite_view(v);
}
inline
int composite::tri_compare::operator()(const composite& v1, const composite& v2) const {
    return (*this)(composite_view(v1), composite_view(v2));
}
#include <any>
class migrate_fn_type {
    std::any _migrators;
    uint32_t _align = 0;
    uint32_t _index;
private:
    static uint32_t register_migrator(migrate_fn_type* m);
    static void unregister_migrator(uint32_t index);
public:
    explicit migrate_fn_type(size_t align) : _align(align), _index(register_migrator(this)) {}
    virtual ~migrate_fn_type() { unregister_migrator(_index); }
    virtual void migrate(void* src, void* dsts, size_t size) const noexcept = 0;
    virtual size_t size(const void* obj) const = 0;
    size_t align() const { return _align; }
    uint32_t index() const { return _index; }
};
template <typename T>
inline
size_t
size_for_allocation_strategy(const T& obj) {
    return sizeof(T);
}
template <typename T>
class standard_migrator final : public migrate_fn_type {
public:
    standard_migrator() : migrate_fn_type(alignof(T)) {}
    virtual void migrate(void* src, void* dst, size_t size) const noexcept override {
        static_assert(std::is_nothrow_move_constructible<T>::value, "T must be nothrow move-constructible.");
        static_assert(std::is_nothrow_destructible<T>::value, "T must be nothrow destructible.");
        T* src_t = static_cast<T*>(src);
        new (static_cast<T*>(dst)) T(std::move(*src_t));
        src_t->~T();
    }
    virtual size_t size(const void* obj) const override {
        return size_for_allocation_strategy(*static_cast<const T*>(obj));
    }
};
template <typename T>
standard_migrator<T>& get_standard_migrator()
{
    static thread_local standard_migrator<T> instance;
    return instance;
}
class allocation_strategy {
protected:
    size_t _preferred_max_contiguous_allocation = std::numeric_limits<size_t>::max();
    uint64_t _invalidate_counter = 1;
public:
    using migrate_fn = const migrate_fn_type*;
    virtual ~allocation_strategy() {}
    virtual void* alloc(migrate_fn, size_t size, size_t alignment) = 0;
    virtual void free(void* object, size_t size) = 0;
    virtual void free(void* object) = 0;
    virtual size_t object_memory_size_in_allocator(const void* obj) const noexcept = 0;
    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        void* storage = alloc(&get_standard_migrator<T>(), sizeof(T), alignof(T));
        try {
            return new (storage) T(std::forward<Args>(args)...);
        } catch (...) {
            free(storage, sizeof(T));
            throw;
        }
    }
    template<typename T>
    void destroy(T* obj) {
        size_t size = size_for_allocation_strategy(*obj);
        obj->~T();
        free(obj, size);
    }
    size_t preferred_max_contiguous_allocation() const {
        return _preferred_max_contiguous_allocation;
    }
    uint64_t invalidate_counter() const {
        return _invalidate_counter;
    }
    void invalidate_references() {
        ++_invalidate_counter;
    }
};
class standard_allocation_strategy : public allocation_strategy {
public:
    virtual void* alloc(migrate_fn, size_t size, size_t alignment) override {
        seastar::memory::on_alloc_point();
        void* ret;
        if (alignment < sizeof(void*)) {
            alignment = sizeof(void*);
        }
        if (posix_memalign(&ret, alignment, size) != 0) {
            throw std::bad_alloc();
        }
        return ret;
    }
    virtual void free(void* obj, size_t size) override {
        ::free(obj);
    }
    virtual void free(void* obj) override {
        ::free(obj);
    }
    virtual size_t object_memory_size_in_allocator(const void* obj) const noexcept {
        return ::malloc_usable_size(const_cast<void *>(obj));
    }
};
extern standard_allocation_strategy standard_allocation_strategy_instance;
inline
standard_allocation_strategy& standard_allocator() {
    return standard_allocation_strategy_instance;
}
inline
allocation_strategy*& current_allocation_strategy_ptr() {
    static thread_local allocation_strategy* current = &standard_allocation_strategy_instance;
    return current;
}
inline
allocation_strategy& current_allocator() {
    return *current_allocation_strategy_ptr();
}
template<typename T>
inline
auto current_deleter() {
    auto& alloc = current_allocator();
    return [&alloc] (T* obj) {
        alloc.destroy(obj);
    };
}
template<typename T>
struct alloc_strategy_deleter {
    void operator()(T* ptr) const noexcept {
        current_allocator().destroy(ptr);
    }
};
template<typename T>
using alloc_strategy_unique_ptr = std::unique_ptr<T, alloc_strategy_deleter<T>>;
class allocator_lock {
    allocation_strategy* _prev;
public:
    allocator_lock(allocation_strategy& alloc) {
        _prev = current_allocation_strategy_ptr();
        current_allocation_strategy_ptr() = &alloc;
    }
    ~allocator_lock() {
        current_allocation_strategy_ptr() = _prev;
    }
};
template<typename Func>
inline
decltype(auto) with_allocator(allocation_strategy& alloc, Func&& func) {
    allocator_lock l(alloc);
    return func();
}
struct blob_storage {
    struct [[gnu::packed]] ref_type {
        blob_storage* ptr;
        ref_type() {}
        ref_type(blob_storage* ptr) : ptr(ptr) {}
        operator blob_storage*() const { return ptr; }
        blob_storage* operator->() const { return ptr; }
        blob_storage& operator*() const { return *ptr; }
    };
    using size_type = uint32_t;
    using char_type = bytes_view::value_type;
    ref_type* backref;
    size_type size;
    size_type frag_size;
    ref_type next;
    char_type data[];
    blob_storage(ref_type* backref, size_type size, size_type frag_size) noexcept
        : backref(backref)
        , size(size)
        , frag_size(frag_size)
        , next(nullptr)
    {
        *backref = this;
    }
    blob_storage(blob_storage&& o) noexcept
        : backref(o.backref)
        , size(o.size)
        , frag_size(o.frag_size)
        , next(o.next)
    {
        *backref = this;
        o.next = nullptr;
        if (next) {
            next->backref = &next;
        }
        memcpy(data, o.data, frag_size);
    }
} __attribute__((packed));
class managed_bytes {
    static thread_local std::unordered_map<const blob_storage*, std::unique_ptr<bytes_view::value_type[]>> _lc_state;
    struct linearization_context {
        unsigned _nesting = 0;
        std::unordered_map<const blob_storage*, std::unique_ptr<bytes_view::value_type[]>>* _state_ptr = nullptr;
        void enter() {
            ++_nesting;
        }
        void leave() {
            if (!--_nesting && _state_ptr) {
                _state_ptr->clear();
                _state_ptr = nullptr;
            }
        }
        void forget(const blob_storage* p) noexcept;
    };
    static thread_local linearization_context _linearization_context;
public:
    struct linearization_context_guard {
        linearization_context_guard() {
            _linearization_context.enter();
        }
        ~linearization_context_guard() {
            _linearization_context.leave();
        }
    };
private:
    static constexpr size_t max_inline_size = 15;
    struct small_blob {
        bytes_view::value_type data[max_inline_size];
        int8_t size; 
    };
    union u {
        u() {}
        ~u() {}
        blob_storage::ref_type ptr;
        small_blob small;
    } _u;
    static_assert(sizeof(small_blob) > sizeof(blob_storage*), "inline size too small");
private:
    bool external() const {
        return _u.small.size < 0;
    }
    size_t max_seg(allocation_strategy& alctr) {
        return alctr.preferred_max_contiguous_allocation() - sizeof(blob_storage);
    }
    void free_chain(blob_storage* p) noexcept {
        if (p->next && _linearization_context._nesting) {
            _linearization_context.forget(p);
        }
        auto& alctr = current_allocator();
        while (p) {
            auto n = p->next;
            alctr.destroy(p);
            p = n;
        }
    }
    const bytes_view::value_type* read_linearize() const {
        seastar::memory::on_alloc_point();
        if (!external()) {
            return _u.small.data;
        } else  if (!_u.ptr->next) {
            return _u.ptr->data;
        } else {
            return do_linearize();
        }
    }
    bytes_view::value_type& value_at_index(blob_storage::size_type index) {
        if (!external()) {
            return _u.small.data[index];
        }
        blob_storage* a = _u.ptr;
        while (index >= a->frag_size) {
            index -= a->frag_size;
            a = a->next;
        }
        return a->data[index];
    }
    const bytes_view::value_type* do_linearize() const;
public:
    using size_type = blob_storage::size_type;
    struct initialized_later {};
    managed_bytes() {
        _u.small.size = 0;
    }
    managed_bytes(const blob_storage::char_type* ptr, size_type size)
        : managed_bytes(bytes_view(ptr, size)) {}
    managed_bytes(const bytes& b) : managed_bytes(static_cast<bytes_view>(b)) {}
    managed_bytes(initialized_later, size_type size) {
        memory::on_alloc_point();
        if (size <= max_inline_size) {
            _u.small.size = size;
        } else {
            _u.small.size = -1;
            auto& alctr = current_allocator();
            auto maxseg = max_seg(alctr);
            auto now = std::min(size_t(size), maxseg);
            void* p = alctr.alloc(&get_standard_migrator<blob_storage>(),
                sizeof(blob_storage) + now, alignof(blob_storage));
            auto first = new (p) blob_storage(&_u.ptr, size, now);
            auto last = first;
            size -= now;
            try {
                while (size) {
                    auto now = std::min(size_t(size), maxseg);
                    void* p = alctr.alloc(&get_standard_migrator<blob_storage>(),
                        sizeof(blob_storage) + now, alignof(blob_storage));
                    last = new (p) blob_storage(&last->next, 0, now);
                    size -= now;
                }
            } catch (...) {
                free_chain(first);
                throw;
            }
        }
    }
    managed_bytes(bytes_view v) : managed_bytes(initialized_later(), v.size()) {
        if (!external()) {
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Warray-bounds"
            std::copy(v.begin(), v.end(), _u.small.data);
            #pragma GCC diagnostic pop
            return;
        }
        auto p = v.data();
        auto s = v.size();
        auto b = _u.ptr;
        while (s) {
            memcpy(b->data, p, b->frag_size);
            p += b->frag_size;
            s -= b->frag_size;
            b = b->next;
        }
        assert(!b);
    }
    managed_bytes(std::initializer_list<bytes::value_type> b) : managed_bytes(b.begin(), b.size()) {}
    ~managed_bytes() noexcept {
        if (external()) {
            free_chain(_u.ptr);
        }
    }
    managed_bytes(const managed_bytes& o) : managed_bytes(initialized_later(), o.size()) {
        if (!external()) {
            memcpy(data(), o.data(), size());
            return;
        }
        auto s = size();
        const blob_storage::ref_type* next_src = &o._u.ptr;
        blob_storage* blob_src = nullptr;
        size_type size_src = 0;
        size_type offs_src = 0;
        blob_storage::ref_type* next_dst = &_u.ptr;
        blob_storage* blob_dst = nullptr;
        size_type size_dst = 0;
        size_type offs_dst = 0;
        while (s) {
            if (!size_src) {
                blob_src = *next_src;
                next_src = &blob_src->next;
                size_src = blob_src->frag_size;
                offs_src = 0;
            }
            if (!size_dst) {
                blob_dst = *next_dst;
                next_dst = &blob_dst->next;
                size_dst = blob_dst->frag_size;
                offs_dst = 0;
            }
            auto now = std::min(size_src, size_dst);
            memcpy(blob_dst->data + offs_dst, blob_src->data + offs_src, now);
            s -= now;
            offs_src += now; size_src -= now;
            offs_dst += now; size_dst -= now;
        }
        assert(size_src == 0 && size_dst == 0);
    }
    managed_bytes(managed_bytes&& o) noexcept
        : _u(o._u)
    {
        if (external()) {
            if (_u.ptr) {
                _u.ptr->backref = &_u.ptr;
            }
        }
        o._u.small.size = 0;
    }
    managed_bytes& operator=(managed_bytes&& o) noexcept {
        if (this != &o) {
            this->~managed_bytes();
            new (this) managed_bytes(std::move(o));
        }
        return *this;
    }
    managed_bytes& operator=(const managed_bytes& o) {
        if (this != &o) {
            managed_bytes tmp(o);
            this->~managed_bytes();
            new (this) managed_bytes(std::move(tmp));
        }
        return *this;
    }
    bool operator==(const managed_bytes& o) const {
        if (size() != o.size()) {
            return false;
        }
        if (!external()) {
            return bytes_view(*this) == bytes_view(o);
        } else {
            auto a = _u.ptr;
            auto a_data = a->data;
            auto a_remain = a->frag_size;
            a = a->next;
            auto b = o._u.ptr;
            auto b_data = b->data;
            auto b_remain = b->frag_size;
            b = b->next;
            while (a_remain || b_remain) {
                auto now = std::min(a_remain, b_remain);
                if (bytes_view(a_data, now) != bytes_view(b_data, now)) {
                    return false;
                }
                a_data += now;
                a_remain -= now;
                if (!a_remain && a) {
                    a_data = a->data;
                    a_remain = a->frag_size;
                    a = a->next;
                }
                b_data += now;
                b_remain -= now;
                if (!b_remain && b) {
                    b_data = b->data;
                    b_remain = b->frag_size;
                    b = b->next;
                }
            }
            return true;
        }
    }
    bool operator!=(const managed_bytes& o) const {
        return !(*this == o);
    }
    operator bytes_view() const {
        return { data(), size() };
    }
    bool is_fragmented() const {
        return external() && _u.ptr->next;
    }
    operator bytes_mutable_view() {
        assert(!is_fragmented());
        return { data(), size() };
    };
    bytes_view::value_type& operator[](size_type index) {
        return value_at_index(index);
    }
    const bytes_view::value_type& operator[](size_type index) const {
        return const_cast<const bytes_view::value_type&>(
                const_cast<managed_bytes*>(this)->value_at_index(index));
    }
    size_type size() const {
        if (external()) {
            return _u.ptr->size;
        } else {
            return _u.small.size;
        }
    }
    const blob_storage::char_type* begin() const {
        return data();
    }
    const blob_storage::char_type* end() const {
        return data() + size();
    }
    blob_storage::char_type* begin() {
        return data();
    }
    blob_storage::char_type* end() {
        return data() + size();
    }
    bool empty() const {
        return _u.small.size == 0;
    }
    blob_storage::char_type* data() {
        if (external()) {
            assert(!_u.ptr->next);  
            return _u.ptr->data;
        } else {
            return _u.small.data;
        }
    }
    const blob_storage::char_type* data() const {
        return read_linearize();
    }
    size_t external_memory_usage() const {
        if (external()) {
            size_t mem = 0;
            blob_storage* blob = _u.ptr;
            while (blob) {
                mem += blob->frag_size + sizeof(blob_storage);
                blob = blob->next;
            }
            return mem;
        }
        return 0;
    }
    template <typename Func>
    friend std::result_of_t<Func()> with_linearized_managed_bytes(Func&& func);
};
template <typename Func>
inline
std::result_of_t<Func()>
with_linearized_managed_bytes(Func&& func) {
    managed_bytes::linearization_context_guard g;
    return func();
}
namespace std {
template <>
struct hash<managed_bytes> {
    size_t operator()(const managed_bytes& v) const {
        return hash<bytes_view>()(v);
    }
};
}
inline
size_t
size_for_allocation_strategy(const blob_storage& bs) {
    return sizeof(bs) + bs.frag_size;
}
class database;
class keyspace;
class table;
using column_family = table;
class memtable_list;
class mutation;
class mutation_partition;
class schema;
class column_definition;
class column_mapping;
class schema_mutations;
class exploded_clustering_prefix;
class partition_key;
class partition_key_view;
class clustering_key_prefix;
class clustering_key_prefix_view;
using clustering_key = clustering_key_prefix;
using clustering_key_view = clustering_key_prefix_view;
class memtable;
template <typename TopLevelView>
class compound_view_wrapper {
protected:
    bytes_view _bytes;
protected:
    compound_view_wrapper(bytes_view v)
        : _bytes(v)
    { }
    static inline const auto& get_compound_type(const schema& s) {
        return TopLevelView::get_compound_type(s);
    }
public:
    std::vector<bytes> explode(const schema& s) const {
        return get_compound_type(s)->deserialize_value(_bytes);
    }
    bytes_view representation() const {
        return _bytes;
    }
    struct less_compare {
        typename TopLevelView::compound _t;
        less_compare(const schema& s) : _t(get_compound_type(s)) {}
        bool operator()(const TopLevelView& k1, const TopLevelView& k2) const {
            return _t->less(k1.representation(), k2.representation());
        }
    };
    struct tri_compare {
        typename TopLevelView::compound _t;
        tri_compare(const schema &s) : _t(get_compound_type(s)) {}
        int operator()(const TopLevelView& k1, const TopLevelView& k2) const {
            return _t->compare(k1.representation(), k2.representation());
        }
    };
    struct hashing {
        typename TopLevelView::compound _t;
        hashing(const schema& s) : _t(get_compound_type(s)) {}
        size_t operator()(const TopLevelView& o) const {
            return _t->hash(o.representation());
        }
    };
    struct equality {
        typename TopLevelView::compound _t;
        equality(const schema& s) : _t(get_compound_type(s)) {}
        bool operator()(const TopLevelView& o1, const TopLevelView& o2) const {
            return _t->equal(o1.representation(), o2.representation());
        }
    };
    bool equal(const schema& s, const TopLevelView& other) const {
        return get_compound_type(s)->equal(representation(), other.representation());
    }
    auto begin() const {
        return TopLevelView::compound::element_type::begin(representation());
    }
    auto end() const {
        return TopLevelView::compound::element_type::end(representation());
    }
    auto begin(const schema& s) const {
        return begin();
    }
    auto end(const schema& s) const {
        return end();
    }
    bytes_view get_component(const schema& s, size_t idx) const {
        auto it = begin(s);
        std::advance(it, idx);
        return *it;
    }
    auto components() const {
        return TopLevelView::compound::element_type::components(representation());
    }
    auto components(const schema& s) const {
        return components();
    }
    bool is_empty() const {
        return _bytes.empty();
    }
    explicit operator bool() const {
        return !is_empty();
    }
    bool is_empty(const schema& s) const {
        return is_empty();
    }
};
template <typename TopLevel, typename TopLevelView>
class compound_wrapper {
protected:
    managed_bytes _bytes;
protected:
    compound_wrapper(managed_bytes&& b) : _bytes(std::move(b)) {}
    static inline const auto& get_compound_type(const schema& s) {
        return TopLevel::get_compound_type(s);
    }
public:
    struct with_schema_wrapper {
        with_schema_wrapper(const schema& s, const TopLevel& key) : s(s), key(key) {}
        const schema& s;
        const TopLevel& key;
    };
    with_schema_wrapper with_schema(const schema& s) const {
        return with_schema_wrapper(s, *static_cast<const TopLevel*>(this));
    }
    static TopLevel make_empty() {
        return from_exploded(std::vector<bytes>());
    }
    static TopLevel make_empty(const schema&) {
        return make_empty();
    }
    template<typename RangeOfSerializedComponents>
    static TopLevel from_exploded(RangeOfSerializedComponents&& v) {
        return TopLevel::from_range(std::forward<RangeOfSerializedComponents>(v));
    }
    static TopLevel from_exploded(const schema& s, const std::vector<bytes>& v) {
        return from_exploded(v);
    }
    static TopLevel from_exploded_view(const std::vector<bytes_view>& v) {
        return from_exploded(v);
    }
    static TopLevel from_optional_exploded(const schema& s, const std::vector<bytes_opt>& v) {
        return TopLevel::from_bytes(get_compound_type(s)->serialize_optionals(v));
    }
    static TopLevel from_deeply_exploded(const schema& s, const std::vector<data_value>& v) {
        return TopLevel::from_bytes(get_compound_type(s)->serialize_value_deep(v));
    }
    static TopLevel from_single_value(const schema& s, bytes v) {
        return TopLevel::from_bytes(get_compound_type(s)->serialize_single(std::move(v)));
    }
    template <typename T>
    static
    TopLevel from_singular(const schema& s, const T& v) {
        auto ct = get_compound_type(s);
        if (!ct->is_singular()) {
            throw std::invalid_argument("compound is not singular");
        }
        auto type = ct->types()[0];
        return from_single_value(s, type->decompose(v));
    }
    TopLevelView view() const {
        return TopLevelView::from_bytes(_bytes);
    }
    operator TopLevelView() const {
        return view();
    }
    std::vector<bytes> explode(const schema& s) const {
        return get_compound_type(s)->deserialize_value(_bytes);
    }
    std::vector<bytes> explode() const {
        std::vector<bytes> result;
        for (bytes_view c : components()) {
            result.emplace_back(to_bytes(c));
        }
        return result;
    }
    struct tri_compare {
        typename TopLevel::compound _t;
        tri_compare(const schema& s) : _t(get_compound_type(s)) {}
        int operator()(const TopLevel& k1, const TopLevel& k2) const {
            return _t->compare(k1.representation(), k2.representation());
        }
        int operator()(const TopLevelView& k1, const TopLevel& k2) const {
            return _t->compare(k1.representation(), k2.representation());
        }
        int operator()(const TopLevel& k1, const TopLevelView& k2) const {
            return _t->compare(k1.representation(), k2.representation());
        }
    };
    struct less_compare {
        typename TopLevel::compound _t;
        less_compare(const schema& s) : _t(get_compound_type(s)) {}
        bool operator()(const TopLevel& k1, const TopLevel& k2) const {
            return _t->less(k1.representation(), k2.representation());
        }
        bool operator()(const TopLevelView& k1, const TopLevel& k2) const {
            return _t->less(k1.representation(), k2.representation());
        }
        bool operator()(const TopLevel& k1, const TopLevelView& k2) const {
            return _t->less(k1.representation(), k2.representation());
        }
    };
    struct hashing {
        hashing(const schema& s);
        size_t operator()(const TopLevel& o) const;
        size_t operator()(const TopLevelView& o) const;
    };
    struct equality {
        equality(const schema& s);
        bool operator()(const TopLevel& o1, const TopLevel& o2) const;
        bool operator()(const TopLevelView& o1, const TopLevel& o2) const;
        bool operator()(const TopLevel& o1, const TopLevelView& o2) const;
    };
    bool equal(const schema& s, const TopLevel& other) const ;
    bool equal(const schema& s, const TopLevelView& other) const ;
    operator bytes_view() const;
    const managed_bytes& representation() const;
    auto begin(const schema& s) const {
        return get_compound_type(s)->begin(_bytes);
    }
    auto end(const schema& s) const {
        return get_compound_type(s)->end(_bytes);
    }
    bool is_empty() const;
    explicit operator bool() const;
    bool is_empty(const schema& s) const;
    auto components() const {
        return TopLevelView::compound::element_type::components(representation());
    }
    auto components(const schema& s) const;
    bytes_view get_component(const schema& s, size_t idx) const;
    size_t size(const schema& s) const;
    size_t external_memory_usage() const;
    size_t memory_usage() const;
};
template <typename TopLevel, typename PrefixTopLevel>
class prefix_view_on_full_compound {
public:
    using iterator = typename compound_type<allow_prefixes::no>::iterator;
    prefix_view_on_full_compound(const schema& s, bytes_view b, unsigned prefix_len);
    iterator begin() const;
    iterator end() const;
    struct less_compare_with_prefix {
        less_compare_with_prefix(const schema& s);
        bool operator()(const prefix_view_on_full_compound& k1, const PrefixTopLevel& k2) const;
        bool operator()(const PrefixTopLevel& k1, const prefix_view_on_full_compound& k2) const;
    };
};
template <typename TopLevel>
class prefix_view_on_prefix_compound {
public:
    using iterator = typename compound_type<allow_prefixes::yes>::iterator;
    prefix_view_on_prefix_compound(const schema& s, bytes_view b, unsigned prefix_len);
    iterator begin() const;
    iterator end() const;
    struct less_compare_with_prefix {
        less_compare_with_prefix(const schema& s);
        bool operator()(const prefix_view_on_prefix_compound& k1, const TopLevel& k2) const;
        bool operator()(const TopLevel& k1, const prefix_view_on_prefix_compound& k2) const;
    };
};
template <typename TopLevel, typename TopLevelView, typename PrefixTopLevel>
class prefixable_full_compound : public compound_wrapper<TopLevel, TopLevelView> {
    using base = compound_wrapper<TopLevel, TopLevelView>;
protected:
    prefixable_full_compound(bytes&& b) : base(std::move(b)) {}
public:
    using prefix_view_type = prefix_view_on_full_compound<TopLevel, PrefixTopLevel>;
    bool is_prefixed_by(const schema& s, const PrefixTopLevel& prefix) const;
    struct less_compare_with_prefix {
        less_compare_with_prefix(const schema& s);
        bool operator()(const TopLevel& k1, const PrefixTopLevel& k2) const;
        bool operator()(const PrefixTopLevel& k1, const TopLevel& k2) const;
    };
    struct prefix_equality_less_compare {
        prefix_equality_less_compare(const schema& s);
        bool operator()(const TopLevel& k1, const PrefixTopLevel& k2) const;
        bool operator()(const PrefixTopLevel& k1, const TopLevel& k2) const;
    };
    prefix_view_type prefix_view(const schema& s, unsigned prefix_len) const;
};
template <typename TopLevel, typename FullTopLevel>
class prefix_compound_view_wrapper : public compound_view_wrapper<TopLevel> {
    using base = compound_view_wrapper<TopLevel>;
protected:
    prefix_compound_view_wrapper(bytes_view v);
public:
    bool is_full(const schema& s) const;
};
template <typename TopLevel, typename TopLevelView, typename FullTopLevel>
class prefix_compound_wrapper : public compound_wrapper<TopLevel, TopLevelView> {
    using base = compound_wrapper<TopLevel, TopLevelView>;
protected:
    prefix_compound_wrapper(managed_bytes&& b) : base(std::move(b)) {}
public:
    using prefix_view_type = prefix_view_on_prefix_compound<TopLevel>;
    prefix_view_type prefix_view(const schema& s, unsigned prefix_len) const;
    bool is_full(const schema& s) const;
    FullTopLevel to_full(const schema& s) const;
    bool is_prefixed_by(const schema& s, const TopLevel& prefix) const;
    struct prefix_equality_less_compare {
        prefix_equality_less_compare(const schema& s);
        bool operator()(const TopLevel& k1, const TopLevel& k2) const;
    };
    struct prefix_equal_tri_compare {
        prefix_equal_tri_compare(const schema& s);
        int operator()(const TopLevel& k1, const TopLevel& k2) const;
    };
};
class partition_key_view : public compound_view_wrapper<partition_key_view> {
public:
    using c_type = compound_type<allow_prefixes::no>;
private:
    partition_key_view(bytes_view v);
public:
    using compound = lw_shared_ptr<c_type>;
    static partition_key_view from_bytes(bytes_view v);
    static const compound& get_compound_type(const schema& s);
    const legacy_compound_view<c_type> legacy_form(const schema& s) const;
    int legacy_tri_compare(const schema& s, partition_key_view o) const;
    bool legacy_equal(const schema& s, partition_key_view o) const;
    int ring_order_tri_compare(const schema& s, partition_key_view o) const;
    friend std::ostream& operator<<(std::ostream& out, const partition_key_view& pk);
};
class partition_key : public compound_wrapper<partition_key, partition_key_view> {
public:
    using c_type = compound_type<allow_prefixes::no>;
    template<typename RangeOfSerializedComponents>
    static partition_key from_range(RangeOfSerializedComponents&& v);
    /*!
     * \brief create a partition_key from a nodetool style string
     * takes a nodetool style string representation of a partition key and returns a partition_key.
     * With composite keys, columns are concatenate using ':'.
     * For example if a composite key is has two columns (col1, col2) to get the partition key that
     * have col1=val1 and col2=val2 use the string 'val1:val2'
     */
    static partition_key from_nodetool_style_string(const schema_ptr s, const sstring& key);
    partition_key(std::vector<bytes> v);
    partition_key(partition_key&& v) = default;
    partition_key(const partition_key& v) = default;
    partition_key(partition_key& v) = default;
    partition_key& operator=(const partition_key&) = default;
    partition_key& operator=(partition_key&) = default;
    partition_key& operator=(partition_key&&) = default;
    partition_key(partition_key_view key);
    using compound = lw_shared_ptr<c_type>;
    static partition_key from_bytes(bytes_view b);
    static const compound& get_compound_type(const schema& s);
    const legacy_compound_view<c_type> legacy_form(const schema& s) const;
    int legacy_tri_compare(const schema& s, const partition_key& o) const;
    bool legacy_equal(const schema& s, const partition_key& o) const;
    void validate(const schema& s) const;
    friend std::ostream& operator<<(std::ostream& out, const partition_key& pk);
};
std::ostream& operator<<(std::ostream& out, const partition_key::with_schema_wrapper& pk);
class exploded_clustering_prefix {
public:
    exploded_clustering_prefix(std::vector<bytes>&& v);
    exploded_clustering_prefix();
    size_t size() const;
    auto const& components() const;
    explicit operator bool() const;
    bool is_full(const schema& s) const;
    friend std::ostream& operator<<(std::ostream& os, const exploded_clustering_prefix& ecp);
};
class clustering_key_prefix_view : public prefix_compound_view_wrapper<clustering_key_prefix_view, clustering_key> {
public:
    static clustering_key_prefix_view from_bytes(bytes_view v);
    using compound = lw_shared_ptr<compound_type<allow_prefixes::yes>>;
    static const compound& get_compound_type(const schema& s);
    static clustering_key_prefix_view make_empty();};
class clustering_key_prefix : public prefix_compound_wrapper<clustering_key_prefix, clustering_key_prefix_view, clustering_key> {
public:
    template<typename RangeOfSerializedComponents>
    static clustering_key_prefix from_range(RangeOfSerializedComponents&& v);
    clustering_key_prefix(std::vector<bytes> v);
    clustering_key_prefix(clustering_key_prefix&& v) = default;
    clustering_key_prefix(const clustering_key_prefix& v) = default;
    clustering_key_prefix(clustering_key_prefix& v) = default;
    clustering_key_prefix& operator=(const clustering_key_prefix&) = default;
    clustering_key_prefix& operator=(clustering_key_prefix&) = default;
    clustering_key_prefix& operator=(clustering_key_prefix&&) = default;
    clustering_key_prefix(clustering_key_prefix_view v);
    using compound = lw_shared_ptr<compound_type<allow_prefixes::yes>>;
    static clustering_key_prefix from_bytes(bytes_view b);
    static const compound& get_compound_type(const schema& s);
    static clustering_key_prefix from_clustering_prefix(const schema& s, const exploded_clustering_prefix& prefix);
    static bool make_full(const schema& s, clustering_key_prefix& ck);    friend std::ostream& operator<<(std::ostream& out, const clustering_key_prefix& ckp);
};
#include <boost/range/adaptor/sliced.hpp>
template<typename T>
class range_bound {
    T _value;
    bool _inclusive;
public:
    range_bound(T value, bool inclusive = true)
              : _value(std::move(value))
              , _inclusive(inclusive)
    { }
    const T& value() const & { return _value; }
    T&& value() && { return std::move(_value); }
    bool is_inclusive() const { return _inclusive; }
    bool operator==(const range_bound& other) const ;
    template<typename Comparator>
    bool equal(const range_bound& other, Comparator&& cmp) const ;
};
template<typename T>
class nonwrapping_range;
template<typename T>
class wrapping_range {
    template <typename U>
    using optional = std::optional<U>;
public:
    using bound = range_bound<T>;
    template <typename Transformer>
    using transformed_type = typename std::remove_cv_t<std::remove_reference_t<std::result_of_t<Transformer(T)>>>;
private:
    optional<bound> _start;
    optional<bound> _end;
    bool _singular;
public:
    wrapping_range(optional<bound> start, optional<bound> end, bool singular = false)  ;
    wrapping_range(T value) 
    ;
    wrapping_range()  ;
private:
    struct start_bound_ref { const optional<bound>& b; };
    struct end_bound_ref { const optional<bound>& b; };
    start_bound_ref start_bound() const ;
    end_bound_ref end_bound() const ;
    template<typename Comparator>
    static bool greater_than_or_equal(end_bound_ref end, start_bound_ref start, Comparator&& cmp) ;
    template<typename Comparator>
    static bool less_than(end_bound_ref end, start_bound_ref start, Comparator&& cmp) ;
    template<typename Comparator>
    static bool less_than_or_equal(start_bound_ref first, start_bound_ref second, Comparator&& cmp) ;
    template<typename Comparator>
    static bool less_than(start_bound_ref first, start_bound_ref second, Comparator&& cmp) ;
    template<typename Comparator>
    static bool greater_than_or_equal(end_bound_ref first, end_bound_ref second, Comparator&& cmp) ;
public:
    template<typename Comparator>
    bool before(const T& point, Comparator&& cmp) const ;
    template<typename Comparator>
    bool after(const T& point, Comparator&& cmp) const ;
    template<typename Comparator>
    bool overlaps(const wrapping_range& other, Comparator&& cmp) const ;
    static wrapping_range make(bound start, bound end) ;
    static wrapping_range make_open_ended_both_sides() ;
    static wrapping_range make_singular(T value) ;
    static wrapping_range make_starting_with(bound b) ;
    static wrapping_range make_ending_with(bound b) ;
    bool is_singular() const ;
    bool is_full() const ;
    void reverse() ;
    const optional<bound>& start() const ;
    const optional<bound>& end() const ;
    template<typename Comparator>
    bool is_wrap_around(Comparator&& cmp) const ;
    std::pair<wrapping_range, wrapping_range> unwrap() const ;
    template<typename Comparator>
    bool contains(const T& point, Comparator&& cmp) const ;
    template<typename Comparator>
    bool contains(const wrapping_range& other, Comparator&& cmp) const ;
    template<typename Comparator>
    std::vector<wrapping_range> subtract(const wrapping_range& other, Comparator&& cmp) const ;
    template<typename Comparator>
    std::pair<wrapping_range<T>, wrapping_range<T>> split(const T& split_point, Comparator&& cmp) const ;
    template<typename Comparator>
    std::optional<wrapping_range<T>> split_after(const T& split_point, Comparator&& cmp) const ;
    template<typename Bound, typename Transformer, typename U = transformed_type<Transformer>>
    static std::optional<typename wrapping_range<U>::bound> transform_bound(Bound&& b, Transformer&& transformer) ;
    template<typename Transformer, typename U = transformed_type<Transformer>>
    wrapping_range<U> transform(Transformer&& transformer) && {
        return wrapping_range<U>(transform_bound(std::move(_start), transformer), transform_bound(std::move(_end), transformer), _singular);
    }
    template<typename Transformer, typename U = transformed_type<Transformer>>
    wrapping_range<U> transform(Transformer&& transformer) const & {
        return wrapping_range<U>(transform_bound(_start, transformer), transform_bound(_end, transformer), _singular);
    }
    template<typename Comparator>
    bool equal(const wrapping_range& other, Comparator&& cmp) const {
        return bool(_start) == bool(other._start)
               && bool(_end) == bool(other._end)
               && (!_start || _start->equal(*other._start, cmp))
               && (!_end || _end->equal(*other._end, cmp))
               && _singular == other._singular;
    }
    bool operator==(const wrapping_range& other) const {
        return (_start == other._start) && (_end == other._end) && (_singular == other._singular);
    }
    template<typename U>
    friend std::ostream& operator<<(std::ostream& out, const wrapping_range<U>& r);
private:
    friend class nonwrapping_range<T>;
};
template<typename U>
std::ostream& operator<<(std::ostream& out, const wrapping_range<U>& r) {
    if (r.is_singular()) {
        return out << "{" << r.start()->value() << "}";
    }
    if (!r.start()) {
        out << "(-inf, ";
    } else {
        if (r.start()->is_inclusive()) {
            out << "[";
        } else {
            out << "(";
        }
        out << r.start()->value() << ", ";
    }
    if (!r.end()) {
        out << "+inf)";
    } else {
        out << r.end()->value();
        if (r.end()->is_inclusive()) {
            out << "]";
        } else {
            out << ")";
        }
    }
    return out;
}
template<typename T>
class nonwrapping_range {
    template <typename U>
    using optional = std::optional<U>;
public:
    using bound = range_bound<T>;
    template <typename Transformer>
    using transformed_type = typename wrapping_range<T>::template transformed_type<Transformer>;
private:
    wrapping_range<T> _range;
public:
    nonwrapping_range(T value)
        : _range(std::move(value))
    { }
    nonwrapping_range() : nonwrapping_range({}, {}) { }
    nonwrapping_range(optional<bound> start, optional<bound> end, bool singular = false)
        : _range(std::move(start), std::move(end), singular)
    { }
    explicit nonwrapping_range(wrapping_range<T>&& r)
        : _range(std::move(r))
    { }
    explicit nonwrapping_range(const wrapping_range<T>& r)
        : _range(r)
    { }
    operator wrapping_range<T>() const & {
        return _range;
    }
    operator wrapping_range<T>() && {
        return std::move(_range);
    }
    template<typename Comparator>
    bool before(const T& point, Comparator&& cmp) const {
        return _range.before(point, std::forward<Comparator>(cmp));
    }
    template<typename Comparator>
    bool after(const T& point, Comparator&& cmp) const {
        return _range.after(point, std::forward<Comparator>(cmp));
    }
    template<typename Comparator>
    bool overlaps(const nonwrapping_range& other, Comparator&& cmp) const {
        if (!start() && !other.start()) {
            return true;
        }
        return wrapping_range<T>::greater_than_or_equal(_range.end_bound(), other._range.start_bound(), cmp)
            && wrapping_range<T>::greater_than_or_equal(other._range.end_bound(), _range.start_bound(), cmp);
    }
    static nonwrapping_range make(bound start, bound end) {
        return nonwrapping_range({std::move(start)}, {std::move(end)});
    }
    static nonwrapping_range make_open_ended_both_sides() {
        return {{}, {}};
    }
    static nonwrapping_range make_singular(T value) {
        return {std::move(value)};
    }
    static nonwrapping_range make_starting_with(bound b) {
        return {{std::move(b)}, {}};
    }
    static nonwrapping_range make_ending_with(bound b) {
        return {{}, {std::move(b)}};
    }
    bool is_singular() const {
        return _range.is_singular();
    }
    bool is_full() const {
        return _range.is_full();
    }
    const optional<bound>& start() const {
        return _range.start();
    }
    const optional<bound>& end() const {
        return _range.end();
    }
    template<typename Comparator>
    bool contains(const T& point, Comparator&& cmp) const {
        return !before(point, cmp) && !after(point, cmp);
    }
    template<typename Comparator>
    bool contains(const nonwrapping_range& other, Comparator&& cmp) const {
        return wrapping_range<T>::less_than_or_equal(_range.start_bound(), other._range.start_bound(), cmp)
                && wrapping_range<T>::greater_than_or_equal(_range.end_bound(), other._range.end_bound(), cmp);
    }
    template<typename Comparator>
    std::vector<nonwrapping_range> subtract(const nonwrapping_range& other, Comparator&& cmp) const {
        auto subtracted = _range.subtract(other._range, std::forward<Comparator>(cmp));
        return boost::copy_range<std::vector<nonwrapping_range>>(subtracted | boost::adaptors::transformed([](auto&& r) {
            return nonwrapping_range(std::move(r));
        }));
    }
    template<typename Comparator>
    std::pair<nonwrapping_range<T>, nonwrapping_range<T>> split(const T& split_point, Comparator&& cmp) const {
        assert(contains(split_point, std::forward<Comparator>(cmp)));
        nonwrapping_range left(start(), bound(split_point));
        nonwrapping_range right(bound(split_point, false), end());
        return std::make_pair(std::move(left), std::move(right));
    }
    template<typename Comparator>
    std::optional<nonwrapping_range> split_after(const T& split_point, Comparator&& cmp) const {
        if (end() && cmp(split_point, end()->value()) >= 0) {
            return std::nullopt;
        } else if (start() && cmp(split_point, start()->value()) < 0) {
            return *this;
        } else {
            return nonwrapping_range(range_bound<T>(split_point, false), end());
        }
    }
    template<typename Comparator>
    std::optional<nonwrapping_range> trim_front(std::optional<bound>&& start, Comparator&& cmp) const {
        return intersection(nonwrapping_range(std::move(start), {}), cmp);
    }
    template<typename Transformer, typename U = transformed_type<Transformer>>
    nonwrapping_range<U> transform(Transformer&& transformer) && {
        return nonwrapping_range<U>(std::move(_range).transform(std::forward<Transformer>(transformer)));
    }
    template<typename Transformer, typename U = transformed_type<Transformer>>
    nonwrapping_range<U> transform(Transformer&& transformer) const & {
        return nonwrapping_range<U>(_range.transform(std::forward<Transformer>(transformer)));
    }
    template<typename Comparator>
    bool equal(const nonwrapping_range& other, Comparator&& cmp) const {
        return _range.equal(other._range, std::forward<Comparator>(cmp));
    }
    bool operator==(const nonwrapping_range& other) const {
        return _range == other._range;
    }
    template<typename Comparator>
    static std::vector<nonwrapping_range> deoverlap(std::vector<nonwrapping_range> ranges, Comparator&& cmp) {
        auto size = ranges.size();
        if (size <= 1) {
            return ranges;
        }
        std::sort(ranges.begin(), ranges.end(), [&](auto&& r1, auto&& r2) {
            return wrapping_range<T>::less_than(r1._range.start_bound(), r2._range.start_bound(), cmp);
        });
        std::vector<nonwrapping_range> deoverlapped_ranges;
        deoverlapped_ranges.reserve(size);
        auto&& current = ranges[0];
        for (auto&& r : ranges | boost::adaptors::sliced(1, ranges.size())) {
            bool includes_end = wrapping_range<T>::greater_than_or_equal(r._range.end_bound(), current._range.start_bound(), cmp)
                                && wrapping_range<T>::greater_than_or_equal(current._range.end_bound(), r._range.end_bound(), cmp);
            if (includes_end) {
                continue; 
            }
            bool includes_start = wrapping_range<T>::greater_than_or_equal(current._range.end_bound(), r._range.start_bound(), cmp);
            if (includes_start) {
                current = nonwrapping_range(std::move(current.start()), std::move(r.end()));
            } else {
                deoverlapped_ranges.emplace_back(std::move(current));
                current = std::move(r);
            }
        }
        deoverlapped_ranges.emplace_back(std::move(current));
        return deoverlapped_ranges;
    }
private:
    struct std_ {};
    struct built_in_ : std_ {};
    template<typename Range, typename LessComparator,
             typename = decltype(std::declval<Range>().lower_bound(std::declval<T>(), std::declval<LessComparator>()))>
    typename std::remove_reference<Range>::type::const_iterator do_lower_bound(const T& value, Range&& r, LessComparator&& cmp, built_in_) const {
        return r.lower_bound(value, std::forward<LessComparator>(cmp));
    }
    template<typename Range, typename LessComparator,
             typename = decltype(std::declval<Range>().upper_bound(std::declval<T>(), std::declval<LessComparator>()))>
    typename std::remove_reference<Range>::type::const_iterator do_upper_bound(const T& value, Range&& r, LessComparator&& cmp, built_in_) const {
        return r.upper_bound(value, std::forward<LessComparator>(cmp));
    }
    template<typename Range, typename LessComparator>
    typename std::remove_reference<Range>::type::const_iterator do_lower_bound(const T& value, Range&& r, LessComparator&& cmp, std_) const {
        return std::lower_bound(r.begin(), r.end(), value, std::forward<LessComparator>(cmp));
    }
    template<typename Range, typename LessComparator>
    typename std::remove_reference<Range>::type::const_iterator do_upper_bound(const T& value, Range&& r, LessComparator&& cmp, std_) const {
        return std::upper_bound(r.begin(), r.end(), value, std::forward<LessComparator>(cmp));
    }
public:
    template<typename Range, typename LessComparator>
    typename std::remove_reference<Range>::type::const_iterator lower_bound(Range&& r, LessComparator&& cmp) const {
        return start()
            ? (start()->is_inclusive()
                ? do_lower_bound(start()->value(), std::forward<Range>(r), std::forward<LessComparator>(cmp), built_in_())
                : do_upper_bound(start()->value(), std::forward<Range>(r), std::forward<LessComparator>(cmp), built_in_()))
            : std::cbegin(r);
    }
    template<typename Range, typename LessComparator>
    typename std::remove_reference<Range>::type::const_iterator upper_bound(Range&& r, LessComparator&& cmp) const {
        return end()
             ? (end()->is_inclusive()
                ? do_upper_bound(end()->value(), std::forward<Range>(r), std::forward<LessComparator>(cmp), built_in_())
                : do_lower_bound(end()->value(), std::forward<Range>(r), std::forward<LessComparator>(cmp), built_in_()))
             : (is_singular()
                ? do_upper_bound(start()->value(), std::forward<Range>(r), std::forward<LessComparator>(cmp), built_in_())
                : std::cend(r));
    }
    template<typename Range, typename LessComparator>
    boost::iterator_range<typename std::remove_reference<Range>::type::const_iterator>
    slice(Range&& range, LessComparator&& cmp) const {
        return boost::make_iterator_range(lower_bound(range, cmp), upper_bound(range, cmp));
    }
    template<typename Comparator>
    std::optional<nonwrapping_range> intersection(const nonwrapping_range& other, Comparator&& cmp) const {
        auto p = std::minmax(_range, other._range, [&cmp] (auto&& a, auto&& b) {
            return wrapping_range<T>::less_than(a.start_bound(), b.start_bound(), cmp);
        });
        if (wrapping_range<T>::greater_than_or_equal(p.first.end_bound(), p.second.start_bound(), cmp)) {
            auto end = std::min(p.first.end_bound(), p.second.end_bound(), [&cmp] (auto&& a, auto&& b) {
                return !wrapping_range<T>::greater_than_or_equal(a, b, cmp);
            });
            return nonwrapping_range(p.second.start(), end.b);
        }
        return {};
    }
    template<typename U>
    friend std::ostream& operator<<(std::ostream& out, const nonwrapping_range<U>& r);
};
template<typename U>
std::ostream& operator<<(std::ostream& out, const nonwrapping_range<U>& r) {
    return out << r._range;
}
template<typename T>
using range = wrapping_range<T>;
GCC6_CONCEPT(
template<template<typename> typename T, typename U>
concept bool Range = std::is_same<T<U>, wrapping_range<U>>::value || std::is_same<T<U>, nonwrapping_range<U>>::value;
)
namespace std {
template<typename T>
struct hash<wrapping_range<T>> {
    using argument_type = wrapping_range<T>;
    using result_type = decltype(std::hash<T>()(std::declval<T>()));
    result_type operator()(argument_type const& s) const {
        auto hash = std::hash<T>();
        auto left = s.start() ? hash(s.start()->value()) : 0;
        auto right = s.end() ? hash(s.end()->value()) : 0;
        return 31 * left + right;
    }
};
template<typename T>
struct hash<nonwrapping_range<T>> {
    using argument_type = nonwrapping_range<T>;
    using result_type = decltype(std::hash<T>()(std::declval<T>()));
    result_type operator()(argument_type const& s) const {
        return hash<wrapping_range<T>>()(s);
    }
};
}
/**
 * Represents the kind of bound in a range tombstone.
 */
enum class bound_kind : uint8_t {
    excl_end = 0,
    incl_start = 1,
    incl_end = 6,
    excl_start = 7,
};
std::ostream& operator<<(std::ostream& out, const bound_kind k);
bound_kind invert_kind(bound_kind k);
int32_t weight(bound_kind k);
class bound_view {
    const static thread_local clustering_key _empty_prefix;
    std::reference_wrapper<const clustering_key_prefix> _prefix;
    bound_kind _kind;
public:
    bound_view(const clustering_key_prefix& prefix, bound_kind kind)
        : _prefix(prefix)
        , _kind(kind)
    { }
    bound_view(const bound_view& other) noexcept = default;
    bound_view& operator=(const bound_view& other) noexcept = default;
    bound_kind kind() const { return _kind; }
    const clustering_key_prefix& prefix() const { return _prefix; }
    struct tri_compare {
        std::reference_wrapper<const schema> _s;
        tri_compare(const schema& s) : _s(s)
        { }
        int operator()(const clustering_key_prefix& p1, int32_t w1, const clustering_key_prefix& p2, int32_t w2) const {
            auto type = _s.get().clustering_key_prefix_type();
            auto res = prefix_equality_tri_compare(type->types().begin(),
                type->begin(p1), type->end(p1),
                type->begin(p2), type->end(p2),
                ::tri_compare);
            if (res) {
                return res;
            }
            auto d1 = p1.size(_s);
            auto d2 = p2.size(_s);
            if (d1 == d2) {
                return w1 - w2;
            }
            return d1 < d2 ? w1 - (w1 <= 0) : -(w2 - (w2 <= 0));
        }
        int operator()(const bound_view b, const clustering_key_prefix& p) const {
            return operator()(b._prefix, weight(b._kind), p, 0);
        }
        int operator()(const clustering_key_prefix& p, const bound_view b) const {
            return operator()(p, 0, b._prefix, weight(b._kind));
        }
        int operator()(const bound_view b1, const bound_view b2) const {
            return operator()(b1._prefix, weight(b1._kind), b2._prefix, weight(b2._kind));
        }
    };
    struct compare {
        tri_compare _cmp;
        compare(const schema& s) : _cmp(s)
        { }
        bool operator()(const clustering_key_prefix& p1, int32_t w1, const clustering_key_prefix& p2, int32_t w2) const {
            return _cmp(p1, w1, p2, w2) < 0;
        }
        bool operator()(const bound_view b, const clustering_key_prefix& p) const {
            return operator()(b._prefix, weight(b._kind), p, 0);
        }
        bool operator()(const clustering_key_prefix& p, const bound_view b) const {
            return operator()(p, 0, b._prefix, weight(b._kind));
        }
        bool operator()(const bound_view b1, const bound_view b2) const {
            return operator()(b1._prefix, weight(b1._kind), b2._prefix, weight(b2._kind));
        }
    };
    bool equal(const schema& s, const bound_view other) const {
        return _kind == other._kind && _prefix.get().equal(s, other._prefix.get());
    }
    bool adjacent(const schema& s, const bound_view other) const {
        return invert_kind(other._kind) == _kind && _prefix.get().equal(s, other._prefix.get());
    }
    static bound_view bottom() {
        return {_empty_prefix, bound_kind::incl_start};
    }
    static bound_view top() {
        return {_empty_prefix, bound_kind::incl_end};
    }
    template<template<typename> typename R>
    GCC6_CONCEPT( requires Range<R, clustering_key_prefix_view> )
    static bound_view from_range_start(const R<clustering_key_prefix>& range) {
        return range.start()
               ? bound_view(range.start()->value(), range.start()->is_inclusive() ? bound_kind::incl_start : bound_kind::excl_start)
               : bottom();
    }
    template<template<typename> typename R>
    GCC6_CONCEPT( requires Range<R, clustering_key_prefix> )
    static bound_view from_range_end(const R<clustering_key_prefix>& range) {
        return range.end()
               ? bound_view(range.end()->value(), range.end()->is_inclusive() ? bound_kind::incl_end : bound_kind::excl_end)
               : top();
    }
    template<template<typename> typename R>
    GCC6_CONCEPT( requires Range<R, clustering_key_prefix> )
    static std::pair<bound_view, bound_view> from_range(const R<clustering_key_prefix>& range) {
        return {from_range_start(range), from_range_end(range)};
    }
    template<template<typename> typename R>
    GCC6_CONCEPT( requires Range<R, clustering_key_prefix_view> )
    static std::optional<typename R<clustering_key_prefix_view>::bound> to_range_bound(const bound_view& bv) {
        if (&bv._prefix.get() == &_empty_prefix) {
            return {};
        }
        bool inclusive = bv._kind != bound_kind::excl_end && bv._kind != bound_kind::excl_start;
        return {typename R<clustering_key_prefix_view>::bound(bv._prefix.get().view(), inclusive)};
    }
    friend std::ostream& operator<<(std::ostream& out, const bound_view& b) {
        return out << "{bound: prefix=" << b._prefix.get() << ", kind=" << b._kind << "}";
    }
};
#include <random>
#include <byteswap.h>
namespace dht {
class token;
enum class token_kind {
    before_all_keys,
    key,
    after_all_keys,
};
class token {
    static inline int64_t normalize(int64_t t) {
        return t == std::numeric_limits<int64_t>::min() ? std::numeric_limits<int64_t>::max() : t;
    }
public:
    using kind = token_kind;
    kind _kind;
    int64_t _data;
    token() : _kind(kind::before_all_keys) {
    }
    token(kind k, int64_t d)
        : _kind(std::move(k))
        , _data(normalize(d)) { }
    token(kind k, const bytes& b) : _kind(std::move(k)) {
        if (b.size() != sizeof(_data)) {
            throw std::runtime_error(fmt::format("Wrong token bytes size: expected {} but got {}", sizeof(_data), b.size()));
        }
        std::copy_n(b.begin(), sizeof(_data), reinterpret_cast<int8_t *>(&_data));
        _data = net::ntoh(_data);
    }
    token(kind k, bytes_view b) : _kind(std::move(k)) {
        if (b.size() != sizeof(_data)) {
            throw std::runtime_error(fmt::format("Wrong token bytes size: expected {} but got {}", sizeof(_data), b.size()));
        }
        std::copy_n(b.begin(), sizeof(_data), reinterpret_cast<int8_t *>(&_data));
        _data = net::ntoh(_data);
    }
    bool is_minimum() const {
        return _kind == kind::before_all_keys;
    }
    bool is_maximum() const {
        return _kind == kind::after_all_keys;
    }
    size_t external_memory_usage() const {
        return 0;
    }
    size_t memory_usage() const {
        return sizeof(token);
    }
    bytes data() const {
        auto t = net::hton(_data);
        bytes b(bytes::initialized_later(), sizeof(_data));
        std::copy_n(reinterpret_cast<int8_t*>(&t), sizeof(_data), b.begin());
        return b;
    }
    /**
     * @return a string representation of this token
     */
    sstring to_sstring() const;
    /**
     * Calculate a token representing the approximate "middle" of the given
     * range.
     *
     * @return The approximate midpoint between left and right.
     */
    static token midpoint(const token& left, const token& right);
    /**
     * @return a randomly generated token
     */
    static token get_random_token();
    /**
     * @return a token from string representation
     */
    static dht::token from_sstring(const sstring& t);
    /**
     * @return a token from its byte representation
     */
    static dht::token from_bytes(bytes_view bytes);
    /**
     * Calculate the deltas between tokens in the ring in order to compare
     *  relative sizes.
     *
     * @param sortedtokens a sorted List of tokens
     * @return the mapping from 'token' to 'percentage of the ring owned by that token'.
     */
    static std::map<token, float> describe_ownership(const std::vector<token>& sorted_tokens);
    static data_type get_token_validator();
    /**
     * Gets the first shard of the minimum token.
     */
    static unsigned shard_of_minimum_token() {
        return 0;  
    }
};
const token& minimum_token();
const token& maximum_token();
int tri_compare(const token& t1, const token& t2);
inline bool operator==(const token& t1, const token& t2) { return tri_compare(t1, t2) == 0; }
inline bool operator<(const token& t1, const token& t2) { return tri_compare(t1, t2) < 0; }
inline bool operator!=(const token& t1, const token& t2) { return std::rel_ops::operator!=(t1, t2); }
inline bool operator>(const token& t1, const token& t2) { return std::rel_ops::operator>(t1, t2); }
inline bool operator<=(const token& t1, const token& t2) { return std::rel_ops::operator<=(t1, t2); }
inline bool operator>=(const token& t1, const token& t2) { return std::rel_ops::operator>=(t1, t2); }
std::ostream& operator<<(std::ostream& out, const token& t);
} 
namespace sstables {
class key_view;
class decorated_key_view;
}
namespace dht {
class decorated_key;
class ring_position;
using partition_range = nonwrapping_range<ring_position>;
using token_range = nonwrapping_range<token>;
using partition_range_vector = std::vector<partition_range>;
using token_range_vector = std::vector<token_range>;
template <typename T>
inline auto get_random_number() {
    static thread_local std::default_random_engine re{std::random_device{}()};
    static thread_local std::uniform_int_distribution<T> dist{};
    return dist(re);
}
class decorated_key {
public:
    dht::token _token;
    partition_key _key;
    decorated_key(dht::token t, partition_key k)
        : _token(std::move(t))
        , _key(std::move(k)) {
    }
    struct less_comparator {
        schema_ptr s;
        less_comparator(schema_ptr s);
        bool operator()(const decorated_key& k1, const decorated_key& k2) const;
        bool operator()(const decorated_key& k1, const ring_position& k2) const;
        bool operator()(const ring_position& k1, const decorated_key& k2) const;
    };
    bool equal(const schema& s, const decorated_key& other) const;
    bool less_compare(const schema& s, const decorated_key& other) const;
    bool less_compare(const schema& s, const ring_position& other) const;
    int tri_compare(const schema& s, const decorated_key& other) const;
    int tri_compare(const schema& s, const ring_position& other) const;
    const dht::token& token() const {
        return _token;
    }
    const partition_key& key() const {
        return _key;
    }
    size_t external_memory_usage() const {
        return _key.external_memory_usage() + _token.external_memory_usage();
    }
    size_t memory_usage() const {
        return sizeof(decorated_key) + external_memory_usage();
    }
};
class decorated_key_equals_comparator {
    const schema& _schema;
public:
    explicit decorated_key_equals_comparator(const schema& schema) : _schema(schema) {}
    bool operator()(const dht::decorated_key& k1, const dht::decorated_key& k2) const {
        return k1.equal(_schema, k2);
    }
};
using decorated_key_opt = std::optional<decorated_key>;
class i_partitioner {
protected:
    unsigned _shard_count;
    unsigned _sharding_ignore_msb_bits;
    std::vector<uint64_t> _shard_start;
public:
    i_partitioner(unsigned shard_count = smp::count, unsigned sharding_ignore_msb_bits = 0);
    virtual ~i_partitioner() {}
    /**
     * Transform key to object representation of the on-disk format.
     *
     * @param key the raw, client-facing key
     * @return decorated version of key
     */
    decorated_key decorate_key(const schema& s, const partition_key& key) {
        return { get_token(s, key), key };
    }
    /**
     * Transform key to object representation of the on-disk format.
     *
     * @param key the raw, client-facing key
     * @return decorated version of key
     */
    decorated_key decorate_key(const schema& s, partition_key&& key) {
        auto token = get_token(s, key);
        return { std::move(token), std::move(key) };
    }
    /**
     * @return a token that can be used to route a given key
     * (This is NOT a method to create a token from its string representation;
     * for that, use tokenFactory.fromString.)
     */
    virtual token get_token(const schema& s, partition_key_view key) const = 0;
    virtual token get_token(const sstables::key_view& key) const = 0;
    /**
     * @return True if the implementing class preserves key order in the tokens
     * it generates.
     */
    virtual bool preserves_order() = 0;
    /**
     * @return name of partitioner.
     */
    virtual const sstring name() const = 0;
    /**
     * Calculates the shard that handles a particular token.
     */
    virtual unsigned shard_of(const token& t) const;
    /**
     * Gets the first token greater than `t` that is in shard `shard`, and is a shard boundary (its first token).
     *
     * If the `spans` parameter is greater than zero, the result is the same as if the function
     * is called `spans` times, each time applied to its return value, but efficiently. This allows
     * selecting ranges that include multiple round trips around the 0..smp::count-1 shard span:
     *
     *     token_for_next_shard(t, shard, spans) == token_for_next_shard(token_for_shard(t, shard, 1), spans - 1)
     *
     * On overflow, maximum_token() is returned.
     */
    virtual token token_for_next_shard(const token& t, shard_id shard, unsigned spans = 1) const;
    /**
     * @return number of shards configured for this partitioner
     */
    unsigned shard_count() const {
        return _shard_count;
    }
    unsigned sharding_ignore_msb() const {
        return _sharding_ignore_msb_bits;
    }
    bool operator==(const i_partitioner& o) const {
        return name() == o.name()
                && sharding_ignore_msb() == o.sharding_ignore_msb();
    }
    bool operator!=(const i_partitioner& o) const {
        return !(*this == o);
    }
};
class ring_position {
public:
    enum class token_bound : int8_t { start = -1, end = 1 };
private:
    friend class ring_position_comparator;
    friend class ring_position_ext;
    dht::token _token;
    token_bound _token_bound{}; 
    std::optional<partition_key> _key;
public:
    static ring_position min() {
        return { minimum_token(), token_bound::start };
    }
    static ring_position max() {
        return { maximum_token(), token_bound::end };
    }
    bool is_min() const {
        return _token.is_minimum();
    }
    bool is_max() const {
        return _token.is_maximum();
    }
    static ring_position starting_at(dht::token token) {
        return { std::move(token), token_bound::start };
    }
    static ring_position ending_at(dht::token token) {
        return { std::move(token), token_bound::end };
    }
    ring_position(dht::token token, token_bound bound)
        : _token(std::move(token))
        , _token_bound(bound)
    { }
    ring_position(dht::token token, partition_key key)
        : _token(std::move(token))
        , _key(std::make_optional(std::move(key)))
    { }
    ring_position(dht::token token, token_bound bound, std::optional<partition_key> key)
        : _token(std::move(token))
        , _token_bound(bound)
        , _key(std::move(key))
    { }
    ring_position(const dht::decorated_key& dk)
        : _token(dk._token)
        , _key(std::make_optional(dk._key))
    { }
    ring_position(dht::decorated_key&& dk)
        : _token(std::move(dk._token))
        , _key(std::make_optional(std::move(dk._key)))
    { }
    const dht::token& token() const {
        return _token;
    }
    token_bound bound() const {
        return _token_bound;
    }
    int relation_to_keys() const {
        return _key ? 0 : static_cast<int>(_token_bound);
    }
    const std::optional<partition_key>& key() const {
        return _key;
    }
    bool has_key() const {
        return bool(_key);
    }
    dht::decorated_key as_decorated_key() const {
        return { _token, *_key };
    }
    bool equal(const schema&, const ring_position&) const;
    int tri_compare(const schema&, const ring_position&) const;
    bool less_compare(const schema&, const ring_position&) const;
    friend std::ostream& operator<<(std::ostream&, const ring_position&);
};
class ring_position_view {
    friend int ring_position_tri_compare(const schema& s, ring_position_view lh, ring_position_view rh);
    friend class ring_position_comparator;
    friend class ring_position_ext;
    const dht::token* _token; 
    const partition_key* _key; 
    int8_t _weight;
public:
    using token_bound = ring_position::token_bound;
    struct after_key_tag {};
    using after_key = bool_class<after_key_tag>;
    static ring_position_view min() {
        return { minimum_token(), nullptr, -1 };
    }
    static ring_position_view max() {
        return { maximum_token(), nullptr, 1 };
    }
    bool is_min() const {
        return _token->is_minimum();
    }
    bool is_max() const {
        return _token->is_maximum();
    }
    static ring_position_view for_range_start(const partition_range& r) {
        return r.start() ? ring_position_view(r.start()->value(), after_key(!r.start()->is_inclusive())) : min();
    }
    static ring_position_view for_range_end(const partition_range& r) {
        return r.end() ? ring_position_view(r.end()->value(), after_key(r.end()->is_inclusive())) : max();
    }
    static ring_position_view for_after_key(const dht::decorated_key& dk) {
        return ring_position_view(dk, after_key::yes);
    }
    static ring_position_view for_after_key(dht::ring_position_view view) {
        return ring_position_view(after_key_tag(), view);
    }
    static ring_position_view starting_at(const dht::token& t) {
        return ring_position_view(t, token_bound::start);
    }
    static ring_position_view ending_at(const dht::token& t) {
        return ring_position_view(t, token_bound::end);
    }
    ring_position_view(const dht::ring_position& pos, after_key after = after_key::no)
        : _token(&pos.token())
        , _key(pos.has_key() ? &*pos.key() : nullptr)
        , _weight(pos.has_key() ? bool(after) : pos.relation_to_keys())
    { }
    ring_position_view(const ring_position_view& pos) = default;
    ring_position_view& operator=(const ring_position_view& other) = default;
    ring_position_view(after_key_tag, const ring_position_view& v)
        : _token(v._token)
        , _key(v._key)
        , _weight(v._key ? 1 : v._weight)
    { }
    ring_position_view(const dht::decorated_key& key, after_key after_key = after_key::no)
        : _token(&key.token())
        , _key(&key.key())
        , _weight(bool(after_key))
    { }
    ring_position_view(const dht::token& token, const partition_key* key, int8_t weight)
        : _token(&token)
        , _key(key)
        , _weight(weight)
    { }
    explicit ring_position_view(const dht::token& token, token_bound bound = token_bound::start)
        : _token(&token)
        , _key(nullptr)
        , _weight(static_cast<std::underlying_type_t<token_bound>>(bound))
    { }
    const dht::token& token() const { return *_token; }
    const partition_key* key() const { return _key; }
    token_bound get_token_bound() const { return token_bound(_weight); }
    after_key is_after_key() const { return after_key(_weight == 1); }
    friend std::ostream& operator<<(std::ostream&, ring_position_view);
};
using ring_position_ext_view = ring_position_view;
class ring_position_ext {
    dht::token _token;
    std::optional<partition_key> _key;
    int8_t _weight;
public:
    using token_bound = ring_position::token_bound;
    struct after_key_tag {};
    using after_key = bool_class<after_key_tag>;
    static ring_position_ext min() {
        return { minimum_token(), std::nullopt, -1 };
    }
    static ring_position_ext max() {
        return { maximum_token(), std::nullopt, 1 };
    }
    bool is_min() const {
        return _token.is_minimum();
    }
    bool is_max() const {
        return _token.is_maximum();
    }
    static ring_position_ext for_range_start(const partition_range& r) {
        return r.start() ? ring_position_ext(r.start()->value(), after_key(!r.start()->is_inclusive())) : min();
    }
    static ring_position_ext for_range_end(const partition_range& r) {
        return r.end() ? ring_position_ext(r.end()->value(), after_key(r.end()->is_inclusive())) : max();
    }
    static ring_position_ext for_after_key(const dht::decorated_key& dk) {
        return ring_position_ext(dk, after_key::yes);
    }
    static ring_position_ext for_after_key(dht::ring_position_ext view) {
        return ring_position_ext(after_key_tag(), view);
    }
    static ring_position_ext starting_at(const dht::token& t) {
        return ring_position_ext(t, token_bound::start);
    }
    static ring_position_ext ending_at(const dht::token& t) {
        return ring_position_ext(t, token_bound::end);
    }
    ring_position_ext(const dht::ring_position& pos, after_key after = after_key::no)
        : _token(pos.token())
        , _key(pos.key())
        , _weight(pos.has_key() ? bool(after) : pos.relation_to_keys())
    { }
    ring_position_ext(const ring_position_ext& pos) = default;
    ring_position_ext& operator=(const ring_position_ext& other) = default;
    ring_position_ext(ring_position_view v)
        : _token(*v._token)
        , _key(v._key ? std::make_optional(*v._key) : std::nullopt)
        , _weight(v._weight)
    { }
    ring_position_ext(after_key_tag, const ring_position_ext& v)
        : _token(v._token)
        , _key(v._key)
        , _weight(v._key ? 1 : v._weight)
    { }
    ring_position_ext(const dht::decorated_key& key, after_key after_key = after_key::no)
        : _token(key.token())
        , _key(key.key())
        , _weight(bool(after_key))
    { }
    ring_position_ext(dht::token token, std::optional<partition_key> key, int8_t weight) noexcept
        : _token(std::move(token))
        , _key(std::move(key))
        , _weight(weight)
    { }
    ring_position_ext(ring_position&& pos) noexcept
        : _token(std::move(pos._token))
        , _key(std::move(pos._key))
        , _weight(pos.relation_to_keys())
    { }
    explicit ring_position_ext(const dht::token& token, token_bound bound = token_bound::start)
        : _token(token)
        , _key(std::nullopt)
        , _weight(static_cast<std::underlying_type_t<token_bound>>(bound))
    { }
    const dht::token& token() const { return _token; }
    const std::optional<partition_key>& key() const { return _key; }
    token_bound get_token_bound() const { return token_bound(_weight); }
    after_key is_after_key() const { return after_key(_weight == 1); }
    operator ring_position_view() const { return { _token, _key ? &*_key : nullptr, _weight }; }
    friend std::ostream& operator<<(std::ostream&, const ring_position_ext&);
};
int ring_position_tri_compare(const schema& s, ring_position_view lh, ring_position_view rh);
struct ring_position_comparator {
    const schema& s;
    ring_position_comparator(const schema& s_) : s(s_) {}
    int operator()(ring_position_view, ring_position_view) const;
    int operator()(ring_position_view, sstables::decorated_key_view) const;
    int operator()(sstables::decorated_key_view, ring_position_view) const;
};
struct ring_position_less_comparator {
    ring_position_comparator tri;
    ring_position_less_comparator(const schema& s) : tri(s) {}
    template<typename T, typename U>
    bool operator()(const T& lh, const U& rh) const {
        return tri(lh, rh) < 0;
    }
};
struct token_comparator {
    int operator()(const token& t1, const token& t2) const;
};
std::ostream& operator<<(std::ostream& out, const token& t);
std::ostream& operator<<(std::ostream& out, const decorated_key& t);
std::ostream& operator<<(std::ostream& out, const i_partitioner& p);
class partition_ranges_view {
    const dht::partition_range* _data = nullptr;
    size_t _size = 0;
public:
    partition_ranges_view() = default;
    partition_ranges_view(const dht::partition_range& range) : _data(&range), _size(1) {}
    partition_ranges_view(const dht::partition_range_vector& ranges) : _data(ranges.data()), _size(ranges.size()) {}
    bool empty() const { return _size == 0; }
    size_t size() const { return _size; }
    const dht::partition_range& front() const { return *_data; }
    const dht::partition_range& back() const { return *(_data + _size - 1); }
    const dht::partition_range* begin() const { return _data; }
    const dht::partition_range* end() const { return _data + _size; }
};
std::ostream& operator<<(std::ostream& out, partition_ranges_view v);
void set_global_partitioner(const sstring& class_name, unsigned ignore_msb = 0);
i_partitioner& global_partitioner();
unsigned shard_of(const schema&, const token&);
inline decorated_key decorate_key(const schema& s, const partition_key& key) {
    return s.get_partitioner().decorate_key(s, key);
}
inline decorated_key decorate_key(const schema& s, partition_key&& key) {
    return s.get_partitioner().decorate_key(s, std::move(key));
}
inline token get_token(const schema& s, partition_key_view key) {
    return s.get_partitioner().get_token(s, key);
}
dht::partition_range to_partition_range(dht::token_range);
dht::partition_range_vector to_partition_ranges(const dht::token_range_vector& ranges);
std::map<unsigned, dht::partition_range_vector>
split_range_to_shards(dht::partition_range pr, const schema& s);
std::map<unsigned, dht::partition_range_vector>
split_ranges_to_shards(const dht::token_range_vector& ranges, const schema& s);
future<utils::chunked_vector<partition_range>> split_range_to_single_shard(const schema& s, const dht::partition_range& pr, shard_id shard);
future<utils::chunked_vector<partition_range>> split_range_to_single_shard(const i_partitioner& partitioner, const schema& s, const dht::partition_range& pr, shard_id shard);
std::unique_ptr<dht::i_partitioner> make_partitioner(sstring name, unsigned shard_count, unsigned sharding_ignore_msb_bits);
extern std::unique_ptr<i_partitioner> default_partitioner;
} 
namespace std {
template<>
struct hash<dht::token> {
    size_t operator()(const dht::token& t) const {
        return bswap_64(t._data);
    }
};
template <>
struct hash<dht::decorated_key> {
    size_t operator()(const dht::decorated_key& k) const {
        auto h_token = hash<dht::token>();
        return h_token(k.token());
    }
};
}
/**
 *
 * Allows to take full advantage of compile-time information when operating
 * on a set of enum values.
 *
 * Examples:
 *
 *   enum class x { A, B, C };
 *   using my_enum = super_enum<x, x::A, x::B, x::C>;
 *   using my_enumset = enum_set<my_enum>;
 *
 *   static_assert(my_enumset::frozen<x::A, x::B>::contains<x::A>(), "it should...");
 *
 *   assert(my_enumset::frozen<x::A, x::B>::contains(my_enumset::prepare<x::A>()));
 *
 *   assert(my_enumset::frozen<x::A, x::B>::contains(x::A));
 *
 */
template<typename EnumType, EnumType... Items>
struct super_enum {
    using enum_type = EnumType;
    template<enum_type... values>
    struct max {
        static constexpr enum_type max_of(enum_type a, enum_type b) {
            return a > b ? a : b;
        }
        template<enum_type first, enum_type second, enum_type... rest>
        static constexpr enum_type get() {
            return max_of(first, get<second, rest...>());
        }
        template<enum_type first>
        static constexpr enum_type get() { return first; }
        static constexpr enum_type value = get<values...>();
    };
    template<enum_type... values>
    struct min {
        static constexpr enum_type min_of(enum_type a, enum_type b) {
            return a < b ? a : b;
        }
        template<enum_type first, enum_type second, enum_type... rest>
        static constexpr enum_type get() {
            return min_of(first, get<second, rest...>());
        }
        template<enum_type first>
        static constexpr enum_type get() { return first; }
        static constexpr enum_type value = get<values...>();
    };
    using sequence_type = typename std::underlying_type<enum_type>::type;
    template <enum_type first, enum_type... rest>
    struct valid_sequence {
        static constexpr bool apply(sequence_type v) noexcept {
            return (v == static_cast<sequence_type>(first)) || valid_sequence<rest...>::apply(v);
        }
    };
    template <enum_type first>
    struct valid_sequence<first> {
        static constexpr bool apply(sequence_type v) noexcept {
            return v == static_cast<sequence_type>(first);
        }
    };
    static constexpr bool is_valid_sequence(sequence_type v) noexcept {
        return valid_sequence<Items...>::apply(v);
    }
    template<enum_type Elem>
    static constexpr sequence_type sequence_for() {
        return static_cast<sequence_type>(Elem);
    }
    static sequence_type sequence_for(enum_type elem) {
        return static_cast<sequence_type>(elem);
    }
    static constexpr sequence_type max_sequence = sequence_for<max<Items...>::value>();
    static constexpr sequence_type min_sequence = sequence_for<min<Items...>::value>();
    static_assert(min_sequence >= 0, "negative enum values unsupported");
};
class bad_enum_set_mask : public std::invalid_argument {
public:
    bad_enum_set_mask() : std::invalid_argument("Bit mask contains invalid enumeration indices.") {
    }
};
template<typename Enum>
class enum_set {
public:
    using mask_type = size_t; 
    using enum_type = typename Enum::enum_type;
private:
    static constexpr int mask_digits = std::numeric_limits<mask_type>::digits;
    using mask_iterator = seastar::bitsets::set_iterator<mask_digits>;
    mask_type _mask;
    constexpr enum_set(mask_type mask) : _mask(mask) {}
    template<enum_type Elem>
    static constexpr unsigned shift_for() {
        return Enum::template sequence_for<Elem>();
    }
    static auto make_iterator(mask_iterator iter) {
        return boost::make_transform_iterator(std::move(iter), [](typename Enum::sequence_type s) {
            return enum_type(s);
        });
    }
public:
    using iterator = std::invoke_result_t<decltype(&enum_set::make_iterator), mask_iterator>;
    constexpr enum_set() : _mask(0) {}
    /**
     * \throws \ref bad_enum_set_mask
     */
    static constexpr enum_set from_mask(mask_type mask) {
        const auto bit_range = seastar::bitsets::for_each_set(std::bitset<mask_digits>(mask));
        if (!std::all_of(bit_range.begin(), bit_range.end(), &Enum::is_valid_sequence)) {
            throw bad_enum_set_mask();
        }
        return enum_set(mask);
    }
    static constexpr mask_type full_mask() {
        return ~(std::numeric_limits<mask_type>::max() << (Enum::max_sequence + 1));
    }
    static constexpr enum_set full() {
        return enum_set(full_mask());
    }
    static inline mask_type mask_for(enum_type e) {
        return mask_type(1) << Enum::sequence_for(e);
    }
    template<enum_type Elem>
    static constexpr mask_type mask_for() {
        return mask_type(1) << shift_for<Elem>();
    }
    struct prepared {
        mask_type mask;
        bool operator==(const prepared& o) const {
            return mask == o.mask;
        }
    };
    static prepared prepare(enum_type e) {
        return {mask_for(e)};
    }
    template<enum_type e>
    static constexpr prepared prepare() {
        return {mask_for<e>()};
    }
    static_assert(std::numeric_limits<mask_type>::max() >= ((size_t)1 << Enum::max_sequence), "mask type too small");
    template<enum_type e>
    bool contains() const {
        return bool(_mask & mask_for<e>());
    }
    bool contains(enum_type e) const {
        return bool(_mask & mask_for(e));
    }
    template<enum_type e>
    void remove() {
        _mask &= ~mask_for<e>();
    }
    void remove(enum_type e) {
        _mask &= ~mask_for(e);
    }
    template<enum_type e>
    void set() {
        _mask |= mask_for<e>();
    }
    template<enum_type e>
    void set_if(bool condition) {
        _mask |= mask_type(condition) << shift_for<e>();
    }
    void set(enum_type e) {
        _mask |= mask_for(e);
    }
    void add(const enum_set& other) {
        _mask |= other._mask;
    }
    explicit operator bool() const {
        return bool(_mask);
    }
    mask_type mask() const {
        return _mask;
    }
    iterator begin() const {
        return make_iterator(mask_iterator(_mask));
    }
    iterator end() const {
        return make_iterator(mask_iterator(0));
    }
    template<enum_type... items>
    struct frozen {
        template<enum_type first>
        static constexpr mask_type make_mask() {
            return mask_for<first>();
        }
        static constexpr mask_type make_mask() {
            return 0;
        }
        template<enum_type first, enum_type second, enum_type... rest>
        static constexpr mask_type make_mask() {
            return mask_for<first>() | make_mask<second, rest...>();
        }
        static constexpr mask_type mask = make_mask<items...>();
        template<enum_type Elem>
        static constexpr bool contains() {
            return mask & mask_for<Elem>();
        }
        static bool contains(enum_type e) {
            return mask & mask_for(e);
        }
        static bool contains(prepared e) {
            return mask & e.mask;
        }
        static constexpr enum_set<Enum> unfreeze() {
            return enum_set<Enum>(mask);
        }
    };
    template<enum_type... items>
    static constexpr enum_set<Enum> of() {
        return frozen<items...>::unfreeze();
    }
};
#include <random>
#include <seastar/core/sharded.hh>
#include <seastar/net/inet_address.hh>
namespace gms {
class inet_address {
private:
    net::inet_address _addr;
public:
    inet_address() = default;
    inet_address(int32_t ip)
        : inet_address(uint32_t(ip)) {
    }
    explicit inet_address(uint32_t ip)
        : _addr(net::ipv4_address(ip)) {
    }
    inet_address(const net::inet_address& addr) : _addr(addr) {}
    inet_address(const socket_address& sa)
        : inet_address(sa.addr())
    {}
    const net::inet_address& addr() const {
        return _addr;
    }
    inet_address(const inet_address&) = default;
    operator const seastar::net::inet_address&() const {
        return _addr;
    }
    inet_address(const sstring& addr) {
        if (addr == "localhost") {
            _addr = net::ipv4_address("127.0.0.1");
        } else {
            _addr = net::inet_address(addr);
        }
    }
    bytes_view bytes() const {
        return bytes_view(reinterpret_cast<const int8_t*>(_addr.data()), _addr.size());
    }
    uint32_t raw_addr() const {
        return addr().as_ipv4_address().ip;
    }
    sstring to_sstring() const {
        return format("{}", *this);
    }
    friend inline bool operator==(const inet_address& x, const inet_address& y) {
        return x._addr == y._addr;
    }
    friend inline bool operator!=(const inet_address& x, const inet_address& y) {
        using namespace std::rel_ops;
        return x._addr != y._addr;
    }
    friend inline bool operator<(const inet_address& x, const inet_address& y) {
        return x.bytes() < y.bytes();
    }
    friend struct std::hash<inet_address>;
    using opt_family = std::optional<net::inet_address::family>;
    static future<inet_address> lookup(sstring, opt_family family = {}, opt_family preferred = {});
};
std::ostream& operator<<(std::ostream& os, const inet_address& x);
}
namespace std {
template<>
struct hash<gms::inet_address> {
    size_t operator()(gms::inet_address a) const { return std::hash<net::inet_address>()(a._addr); }
};
}
namespace tracing {
using elapsed_clock = std::chrono::steady_clock;
extern logging::logger tracing_logger;
class trace_state_ptr;
class tracing;
class backend_registry;
enum class trace_type : uint8_t {
    NONE,
    QUERY,
    REPAIR,
};
extern std::vector<sstring> trace_type_names;
inline const sstring& type_to_string(trace_type t) {
    return trace_type_names.at(static_cast<int>(t));
}
/**
 * Returns a TTL for a given trace type
 * @param t trace type
 *
 * @return TTL
 */
inline std::chrono::seconds ttl_by_type(const trace_type t) {
    switch (t) {
    case trace_type::NONE:
    case trace_type::QUERY:
        return std::chrono::seconds(86400);  
    case trace_type::REPAIR:
        return std::chrono::seconds(604800); 
    default:
        throw std::invalid_argument("unknown trace type: " + std::to_string(int(t)));
    }
}
/**
 * @brief represents an ID of a single tracing span.
 *
 * Currently span ID is a random 64-bit integer.
 */
class span_id {
private:
    uint64_t _id = illegal_id;
public:
    static constexpr uint64_t illegal_id = 0;
public:
    span_id() = default;
    uint64_t get_id() const { return _id; }
    span_id(uint64_t id) : _id(id) {}
    /**
     * @return New span_id with a random legal value
     */
    static span_id make_span_id();
};
std::ostream& operator<<(std::ostream& os, const span_id& id);
enum class trace_state_props {
    write_on_close, primary, log_slow_query, full_tracing
};
using trace_state_props_set = enum_set<super_enum<trace_state_props,
    trace_state_props::write_on_close,
    trace_state_props::primary,
    trace_state_props::log_slow_query,
    trace_state_props::full_tracing>>;
class trace_info {
public:
    utils::UUID session_id;
    trace_type type;
    bool write_on_close;
    trace_state_props_set state_props;
    uint32_t slow_query_threshold_us; 
    uint32_t slow_query_ttl_sec; 
    span_id parent_id;
public:
    trace_info(utils::UUID sid, trace_type t, bool w_o_c, trace_state_props_set s_p, uint32_t slow_query_threshold, uint32_t slow_query_ttl, span_id p_id)
        : session_id(std::move(sid))
        , type(t)
        , write_on_close(w_o_c)
        , state_props(s_p)
        , slow_query_threshold_us(slow_query_threshold)
        , slow_query_ttl_sec(slow_query_ttl)
        , parent_id(std::move(p_id))
    {
        state_props.set_if<trace_state_props::write_on_close>(write_on_close);
    }
};
struct one_session_records;
using records_bulk = std::deque<lw_shared_ptr<one_session_records>>;
struct backend_session_state_base {
    virtual ~backend_session_state_base() {};
};
struct i_tracing_backend_helper {
    using wall_clock = std::chrono::system_clock;
protected:
    tracing& _local_tracing;
public:
    i_tracing_backend_helper(tracing& tr) : _local_tracing(tr) {}
    virtual ~i_tracing_backend_helper() {}
    virtual future<> start() = 0;
    virtual future<> stop() = 0;
    virtual void write_records_bulk(records_bulk& bulk) = 0;
    virtual std::unique_ptr<backend_session_state_base> allocate_session_state() const = 0;
private:
    friend class tracing;
};
struct event_record {
    sstring message;
    elapsed_clock::duration elapsed;
    i_tracing_backend_helper::wall_clock::time_point event_time_point;
    event_record(sstring message_, elapsed_clock::duration elapsed_, i_tracing_backend_helper::wall_clock::time_point event_time_point_)
        : message(std::move(message_))
        , elapsed(elapsed_)
        , event_time_point(event_time_point_) {}
};
struct session_record {
    gms::inet_address client;
    std::map<sstring, sstring> parameters;
    std::set<sstring> tables;
    sstring username;
    sstring request;
    size_t request_size = 0;
    size_t response_size = 0;
    std::chrono::system_clock::time_point started_at;
    trace_type command = trace_type::NONE;
    elapsed_clock::duration elapsed;
    std::chrono::seconds slow_query_record_ttl;
private:
    bool _consumed = false;
public:
    session_record()
        : username("<unauthenticated request>")
        , elapsed(-1) {}
    bool ready() const {
        return elapsed.count() >= 0 && !_consumed;
    }
    void set_consumed() {
        _consumed = true;
    }
};
class one_session_records {
private:
    shared_ptr<tracing> _local_tracing_ptr;
public:
    utils::UUID session_id;
    session_record session_rec;
    std::chrono::seconds ttl;
    std::deque<event_record> events_recs;
    std::unique_ptr<backend_session_state_base> backend_state_ptr;
    bool do_log_slow_query = false;
    uint64_t* budget_ptr;
    span_id parent_id;
    span_id my_span_id;
    one_session_records();
    /**
     * Consume a single record from the per-shard budget.
     */
    void consume_from_budget() {
        ++(*budget_ptr);
    }
    /**
     * Drop all pending records and return the budget.
     */
    void drop_records() {
        (*budget_ptr) -= size();
        events_recs.clear();
        session_rec.set_consumed();
    }
    /**
     * Should be called when a record is scheduled for write.
     * From that point till data_consumed() call all new records will be written
     * in the next write event.
     */
    inline void set_pending_for_write();
    /**
     * Should be called after all data pending to be written in this record has
     * been processed.
     * From that point on new records are cached internally and have to be
     * explicitly committed for write in order to be written during the write event.
     */
    inline void data_consumed();
    bool is_pending_for_write() const {
        return _is_pending_for_write;
    }
    uint64_t size() const {
        return events_recs.size() + session_rec.ready();
    }
private:
    bool _is_pending_for_write = false;
};
class tracing : public seastar::async_sharded_service<tracing> {
public:
    static const gc_clock::duration write_period;
    static constexpr int max_pending_sessions = 1000;
    static constexpr int exp_trace_events_per_session = 10;
    static constexpr int max_pending_trace_records = max_pending_sessions * exp_trace_events_per_session;
    static constexpr int write_event_sessions_threshold = 100;
    static constexpr int write_event_records_threshold = write_event_sessions_threshold * exp_trace_events_per_session;
    static constexpr int log_warning_period = 10000;
    static const std::chrono::microseconds default_slow_query_duraion_threshold;
    static const std::chrono::seconds default_slow_query_record_ttl;
    struct stats {
        uint64_t dropped_sessions = 0;
        uint64_t dropped_records = 0;
        uint64_t trace_records_count = 0;
        uint64_t trace_errors = 0;
    } stats;
private:
    uint64_t _active_sessions = 0;
    uint64_t _cached_records = 0;
    uint64_t _flushing_records = 0;
    uint64_t _pending_for_write_records_count = 0;
    records_bulk _pending_for_write_records_bulk;
    timer<lowres_clock> _write_timer;
    bool _down = true;
    bool _slow_query_logging_enabled = false;
    std::unique_ptr<i_tracing_backend_helper> _tracing_backend_helper_ptr;
    sstring _thread_name;
    const backend_registry& _backend_registry;
    sstring _tracing_backend_helper_class_name;
    seastar::metrics::metric_groups _metrics;
    double _trace_probability = 0.0; 
    uint64_t _normalized_trace_probability = 0;
    std::ranlux48_base _gen;
    std::chrono::microseconds _slow_query_duration_threshold;
    std::chrono::seconds _slow_query_record_ttl;
public:
    uint64_t get_next_rand_uint64() {
        return _gen();
    }
    i_tracing_backend_helper& backend_helper() {
        return *_tracing_backend_helper_ptr;
    }
    const sstring& get_thread_name() const {
        return _thread_name;
    }
    static seastar::sharded<tracing>& tracing_instance() {
        static seastar::sharded<tracing>* tracing_inst = new seastar::sharded<tracing>();
        return *tracing_inst;
    }
    static tracing& get_local_tracing_instance() {
        return tracing_instance().local();
    }
    bool started() const {
        return !_down;
    }
    static future<> create_tracing(const backend_registry& br, sstring tracing_backend_helper_class_name);
    static future<> start_tracing();
    tracing(const backend_registry& br, sstring tracing_backend_helper_class_name);
    future<> start();
    future<> stop();
    future<> shutdown();
    void write_pending_records() {
        if (_pending_for_write_records_bulk.size()) {
            _flushing_records += _pending_for_write_records_count;
            stats.trace_records_count += _pending_for_write_records_count;
            _pending_for_write_records_count = 0;
            _tracing_backend_helper_ptr->write_records_bulk(_pending_for_write_records_bulk);
            _pending_for_write_records_bulk.clear();
        }
    }
    void write_complete(uint64_t nr = 1) {
        if (nr > _flushing_records) {
            throw std::logic_error(seastar::format("completing more records ({:d}) than there are pending ({:d})", nr, _flushing_records));
        }
        _flushing_records -= nr;
    }
    trace_state_ptr create_session(trace_type type, trace_state_props_set props) noexcept;
    trace_state_ptr create_session(const trace_info& secondary_session_info) noexcept;
    void write_maybe() {
        if (_pending_for_write_records_count >= write_event_records_threshold || _pending_for_write_records_bulk.size() >= write_event_sessions_threshold) {
            write_pending_records();
        }
    }
    void end_session() {
        --_active_sessions;
    }
    void write_session_records(lw_shared_ptr<one_session_records> records, bool write_now) {
        if (_down) {
            return;
        }
        try {
            schedule_for_write(std::move(records));
        } catch (...) {
            ++stats.trace_errors;
            return;
        }
        if (write_now) {
            write_pending_records();
        } else {
            write_maybe();
        }
    }
    void set_trace_probability(double p);
    double get_trace_probability() const {
        return _trace_probability;
    }
    bool trace_next_query() {
        return _normalized_trace_probability != 0 && _gen() < _normalized_trace_probability;
    }
    std::unique_ptr<backend_session_state_base> allocate_backend_session_state() const {
        return _tracing_backend_helper_ptr->allocate_session_state();
    }
    bool have_records_budget(uint64_t nr = 1) {
        if (_pending_for_write_records_count + _cached_records + _flushing_records + nr > max_pending_trace_records + write_event_records_threshold) {
            return false;
        }
        return true;
    }
    uint64_t* get_pending_records_ptr() {
        return &_pending_for_write_records_count;
    }
    uint64_t* get_cached_records_ptr() {
        return &_cached_records;
    }
    void schedule_for_write(lw_shared_ptr<one_session_records> records) {
        if (records->is_pending_for_write()) {
            return;
        }
        _pending_for_write_records_bulk.emplace_back(records);
        records->set_pending_for_write();
        auto current_records_num = records->size();
        _cached_records -= current_records_num;
        _pending_for_write_records_count += current_records_num;
    }
    void set_slow_query_enabled(bool enable = true) {
        _slow_query_logging_enabled = enable;
    }
    bool slow_query_tracing_enabled() const {
        return _slow_query_logging_enabled;
    }
    void set_slow_query_threshold(std::chrono::microseconds new_threshold) {
        if (new_threshold.count() > std::numeric_limits<uint32_t>::max()) {
            _slow_query_duration_threshold = std::chrono::microseconds(std::numeric_limits<uint32_t>::max());
            return;
        }
        _slow_query_duration_threshold = new_threshold;
    }
    std::chrono::microseconds slow_query_threshold() const {
        return _slow_query_duration_threshold;
    }
    void set_slow_query_record_ttl(std::chrono::seconds new_ttl) {
        if (new_ttl.count() > std::numeric_limits<int32_t>::max()) {
            _slow_query_record_ttl = std::chrono::seconds(std::numeric_limits<int32_t>::max());
            return;
        }
        _slow_query_record_ttl = new_ttl;
    }
    std::chrono::seconds slow_query_record_ttl() const {
        return _slow_query_record_ttl;
    }
private:
    void write_timer_callback();
    bool may_create_new_session(const std::optional<utils::UUID>& session_id = std::nullopt) {
        if (!have_records_budget(exp_trace_events_per_session) || _active_sessions >= max_pending_sessions + write_event_sessions_threshold) {
            if (session_id) {
                tracing_logger.trace("{}: Too many outstanding tracing records or sessions. Dropping a secondary session", *session_id);
            } else {
                tracing_logger.trace("Too many outstanding tracing records or sessions. Dropping a primary session");
            }
            if (++stats.dropped_sessions % tracing::log_warning_period == 1) {
                tracing_logger.warn("Dropped {} sessions: open_sessions {}, cached_records {} pending_for_write_records {}, flushing_records {}",
                            stats.dropped_sessions, _active_sessions, _cached_records, _pending_for_write_records_count, _flushing_records);
            }
            return false;
        }
        return true;
    }
};
void one_session_records::set_pending_for_write() {
    _is_pending_for_write = true;
    budget_ptr = _local_tracing_ptr->get_pending_records_ptr();
}
void one_session_records::data_consumed() {
    if (session_rec.ready()) {
        session_rec.set_consumed();
    }
    _is_pending_for_write = false;
    budget_ptr = _local_tracing_ptr->get_cached_records_ptr();
}
inline span_id span_id::make_span_id() {
    return 1 + (tracing::get_local_tracing_instance().get_next_rand_uint64() << 1);
}
}
class position_in_partition_view;
namespace query {
using column_id_vector = utils::small_vector<column_id, 8>;
template <typename T>
using range = wrapping_range<T>;
using ring_position = dht::ring_position;
using clustering_range = nonwrapping_range<clustering_key_prefix>;
extern const dht::partition_range full_partition_range;
extern const clustering_range full_clustering_range;
typedef std::vector<clustering_range> clustering_row_ranges;
void trim_clustering_row_ranges_to(const schema& s, clustering_row_ranges& ranges, position_in_partition_view pos, bool reversed = false);
void trim_clustering_row_ranges_to(const schema& s, clustering_row_ranges& ranges, const clustering_key& key, bool reversed = false);
class specific_ranges {
};
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
    using option_set = enum_set<super_enum<option,
        option::send_clustering_key,
        option::send_partition_key,
        option::send_timestamp,
        option::send_expiry,
        option::reversed,
        option::distinct,
        option::collections_as_maps,
        option::send_ttl,
        option::allow_short_read,
        option::with_digest,
        option::bypass_cache,
        option::always_return_static_content>>;
public:
    partition_slice(clustering_row_ranges row_ranges, column_id_vector static_columns,
        column_id_vector regular_columns, option_set options,
        std::unique_ptr<specific_ranges> specific_ranges = nullptr,
        cql_serialization_format = cql_serialization_format::internal(),
        uint32_t partition_row_limit = max_rows);
    partition_slice(clustering_row_ranges ranges, const schema& schema, const column_set& mask, option_set options);
    partition_slice(const partition_slice&);
    partition_slice(partition_slice&&);
    partition_slice& operator=(partition_slice&& other) noexcept;
    const clustering_row_ranges& row_ranges(const schema&, const partition_key&) const;
    void set_range(const schema&, const partition_key&, clustering_row_ranges);
    void clear_range(const schema&, const partition_key&);
    void clear_ranges();    
    clustering_row_ranges get_all_ranges() const;
    const clustering_row_ranges& default_row_ranges() const;
    const std::unique_ptr<specific_ranges>& get_specific_ranges() const;
    const cql_serialization_format& cql_format() const;
    const uint32_t partition_row_limit() const;
    void set_partition_row_limit(uint32_t limit);
    friend std::ostream& operator<<(std::ostream& out, const partition_slice& ps);
    friend std::ostream& operator<<(std::ostream& out, const specific_ranges& ps);
};
constexpr auto max_partitions = std::numeric_limits<uint32_t>::max();
}
lexicographical_relation relation_for_lower_bound(composite_view v);
lexicographical_relation relation_for_upper_bound(composite_view v);
enum class bound_weight : int8_t {
    before_all_prefixed = -1,
    equal = 0,
    after_all_prefixed = 1,
};
bound_weight position_weight(bound_kind k);
enum class partition_region : uint8_t {
    partition_start,
    static_row,
    clustered,
    partition_end,
};
class position_in_partition_view {
    friend class position_in_partition;
public:
    position_in_partition_view(partition_region type, bound_weight weight, const clustering_key_prefix* ck);
    bool is_before_key() const;
    bool is_after_key() const;
private:
    lexicographical_relation relation() const;
public:
    struct partition_start_tag_t { };
    struct end_of_partition_tag_t { };
    struct static_row_tag_t { };
    struct clustering_row_tag_t { };
    struct range_tag_t { };
    using range_tombstone_tag_t = range_tag_t;
    explicit position_in_partition_view(partition_start_tag_t);
    explicit position_in_partition_view(end_of_partition_tag_t);
    explicit position_in_partition_view(static_row_tag_t);
    position_in_partition_view(clustering_row_tag_t, const clustering_key_prefix& ck);
    position_in_partition_view(const clustering_key_prefix& ck);
    position_in_partition_view(range_tag_t, bound_view bv);
    position_in_partition_view(const clustering_key_prefix& ck, bound_weight w);
    static position_in_partition_view for_range_start(const query::clustering_range& r);
    static position_in_partition_view for_range_end(const query::clustering_range& r);
    static position_in_partition_view before_all_clustered_rows();
    static position_in_partition_view after_all_clustered_rows();
    static position_in_partition_view for_static_row();
    static position_in_partition_view for_key(const clustering_key& ck);
    static position_in_partition_view after_key(const clustering_key& ck);
    static position_in_partition_view before_key(const clustering_key& ck);
    partition_region region() const;
    bound_weight get_bound_weight() const;
    bool is_partition_start() const;
    bool is_partition_end() const;
    bool is_static_row() const;
    bool is_clustering_row() const;
    bool has_clustering_key() const;
    bool is_before_all_fragments(const schema& s) const;
    bool is_after_all_clustered_rows(const schema& s) const;
    const clustering_key_prefix& key() const;
    bound_view as_start_bound_view() const;
    bound_view as_end_bound_view() const;
    class printer {
    public:
        printer(const schema& schema, const position_in_partition_view& pipv);
        friend std::ostream& operator<<(std::ostream& os, printer p);
    };
    friend std::ostream& operator<<(std::ostream& os, printer p);
    friend std::ostream& operator<<(std::ostream&, position_in_partition_view);
    friend bool no_clustering_row_between(const schema&, position_in_partition_view, position_in_partition_view);
};
class position_in_partition {
public:
    friend class clustering_interval_set;
    struct partition_start_tag_t { };
    struct end_of_partition_tag_t { };
    struct static_row_tag_t { };
    struct after_static_row_tag_t { };
    struct clustering_row_tag_t { };
    struct after_clustering_row_tag_t { };
    struct before_clustering_row_tag_t { };
    struct range_tag_t { };
    using range_tombstone_tag_t = range_tag_t;
    partition_region get_type() const;
    bound_weight get_bound_weight() const;
    const std::optional<clustering_key_prefix>& get_clustering_key_prefix() const;
    position_in_partition(partition_region type, bound_weight weight, std::optional<clustering_key_prefix> ck);
    explicit position_in_partition(partition_start_tag_t);
    explicit position_in_partition(end_of_partition_tag_t);
    explicit position_in_partition(static_row_tag_t);
    position_in_partition(clustering_row_tag_t, clustering_key_prefix ck);
    position_in_partition(after_clustering_row_tag_t, clustering_key_prefix ck);
    position_in_partition(after_clustering_row_tag_t, position_in_partition_view pos);
    position_in_partition(before_clustering_row_tag_t, clustering_key_prefix ck);
    position_in_partition(range_tag_t, bound_view bv);
    position_in_partition(range_tag_t, bound_kind kind, clustering_key_prefix&& prefix);
    position_in_partition(after_static_row_tag_t);
    explicit position_in_partition(position_in_partition_view view);
    position_in_partition& operator=(position_in_partition_view view);
    static position_in_partition before_all_clustered_rows();
    static position_in_partition after_all_clustered_rows();
    static position_in_partition before_key(clustering_key ck);
    static position_in_partition after_key(clustering_key ck);
    static position_in_partition after_key(position_in_partition_view pos);
    static position_in_partition for_key(clustering_key ck);
    static position_in_partition for_partition_start();
    static position_in_partition for_static_row();
    static position_in_partition min();
    static position_in_partition for_range_start(const query::clustering_range&);
    static position_in_partition for_range_end(const query::clustering_range&);
    partition_region region() const;
    bool is_partition_start() const;
    bool is_partition_end() const;
    bool is_static_row() const;
    bool is_clustering_row() const;
    bool has_clustering_key() const;
    bool is_after_all_clustered_rows(const schema& s) const;
    bool is_before_all_clustered_rows(const schema& s) const;
    template<typename Hasher>
    void feed_hash(Hasher& hasher, const schema& s) const;
    const clustering_key_prefix& key() const;
    operator position_in_partition_view() const;
    class composite_tri_compare {
    public:
        static int rank(partition_region t);
        composite_tri_compare(const schema& s);
        int operator()(position_in_partition_view a, position_in_partition_view b) const;
        int operator()(position_in_partition_view a, composite_view b) const;
        int operator()(composite_view a, position_in_partition_view b) const;
    };
    class composite_less_compare {
    public:
        composite_less_compare(const schema& s);
        template<typename T, typename U>
        bool operator()(const T& a, const U& b) const;
    };
    class tri_compare {
    public:
        tri_compare(const schema& s);
        int operator()(const position_in_partition& a, const position_in_partition& b) const;
        int operator()(const position_in_partition_view& a, const position_in_partition_view& b) const;
        int operator()(const position_in_partition& a, const position_in_partition_view& b) const;
        int operator()(const position_in_partition_view& a, const position_in_partition& b) const;
    };
    class less_compare {
    public:
        less_compare(const schema& s);
        bool operator()(const position_in_partition& a, const position_in_partition& b) const;
        bool operator()(const position_in_partition_view& a, const position_in_partition_view& b) const;
        bool operator()(const position_in_partition& a, const position_in_partition_view& b) const;
        bool operator()(const position_in_partition_view& a, const position_in_partition& b) const;
    };
    class equal_compare {
        template<typename T, typename U>
        bool compare(const T& a, const U& b) const;
    public:
        equal_compare(const schema& s);
        bool operator()(const position_in_partition& a, const position_in_partition& b) const;
        bool operator()(const position_in_partition_view& a, const position_in_partition_view& b) const;
        bool operator()(const position_in_partition_view& a, const position_in_partition& b) const;
        bool operator()(const position_in_partition& a, const position_in_partition_view& b) const;
    };
    friend std::ostream& operator<<(std::ostream&, const position_in_partition&);
};
bool no_clustering_row_between(const schema& s, position_in_partition_view a, position_in_partition_view b);
class position_range {
public:
    static position_range from_range(const query::clustering_range&);
    static position_range for_static_row();
    static position_range full();
    static position_range all_clustered_rows();
    position_range(position_range&&) = default;
    position_range& operator=(position_range&&) = default;
    position_range(const position_range&) = default;
    position_range& operator=(const position_range&) = default;
    position_range(const query::clustering_range&);
    position_range(query::clustering_range&&);
    position_range(position_in_partition start, position_in_partition end);
    const position_in_partition& start() const&;
    position_in_partition&& start() &&;
    const position_in_partition& end() const&;
    position_in_partition&& end() &&;
    bool contains(const schema& s, position_in_partition_view pos) const;
    bool overlaps(const schema& s, position_in_partition_view start, position_in_partition_view end) const;
    friend std::ostream& operator<<(std::ostream&, const position_range&);
};
class clustering_interval_set;
namespace utils {
template<typename T, typename Exception = std::runtime_error>
GCC6_CONCEPT(
    requires FragmentRange<T>
)
class linearizing_input_stream {
    using iterator = typename T::iterator;
    using fragment_type = typename T::fragment_type;
private:
    iterator _it;
    iterator _end;
    fragment_type _current;
    size_t _size;
    std::list<bytes> _linearized_values;
private:
    size_t remove_current_prefix(size_t size) {
        if (size < _current.size()) {
            _current.remove_prefix(size);
            _size -= size;
            return size;
        }
        const auto ret = _current.size();
        _size -= ret;
        ++_it;
        _current = (_it == _end) ? fragment_type{} : *_it;
        return ret;
    }
    void check_size(size_t size) const {
        if (size > _size) {
            seastar::throw_with_backtrace<Exception>(
                    fmt::format("linearizing_input_stream::check_size() - not enough bytes (requested {:d}, got {:d})", size, _size));
        }
    }
    std::pair<bytes_view, bool> do_read(size_t size) {
        check_size(size);
        if (size <= _current.size()) {
            bytes_view ret(_current.begin(), size);
            remove_current_prefix(size);
            return {ret, false};
        }
        auto out = _linearized_values.emplace_back(bytes::initialized_later{}, size).begin();
        while (size) {
            out = std::copy_n(_current.begin(), std::min(size, _current.size()), out);
            size -= remove_current_prefix(size);
        }
        return {_linearized_values.back(), true};
    }
public:
    explicit linearizing_input_stream(const T& fr)
        : _it(fr.begin())
        , _end(fr.end())
        , _current(*_it)
        , _size(fr.size_bytes()) {
    }
    linearizing_input_stream(const linearizing_input_stream&) = delete;
    size_t size() const {
        return _size;
    }
    bool empty() const {
        return _size == 0;
    }
    bytes_view read(size_t size) {
        return do_read(size).first;
    }
    template <typename Type>
    GCC6_CONCEPT(
        requires std::is_trivial_v<Type>
    )
    Type read_trivial() {
        auto [bv, linearized] = do_read(sizeof(Type));
        auto ret = net::ntoh(*reinterpret_cast<const net::packed<Type>*>(bv.begin()));
        if (linearized) {
            _linearized_values.pop_back();
        }
        return ret;
    }
    void skip(size_t size) {
        check_size(size);
        while (size) {
            size -= remove_current_prefix(size);
        }
    }
};
} 
class abstract_type;
class bytes_ostream;
class compaction_garbage_collector;
class row_tombstone;
class collection_mutation;
struct collection_mutation_description {
    tombstone tomb;
    utils::chunked_vector<std::pair<bytes, atomic_cell>> cells;
    bool compact_and_expire(column_id id, row_tombstone tomb, gc_clock::time_point query_time,
        can_gc_fn&, gc_clock::time_point gc_before, compaction_garbage_collector* collector = nullptr);
    collection_mutation serialize(const abstract_type&) const;
};
struct collection_mutation_view_description {
    tombstone tomb;
    utils::chunked_vector<std::pair<bytes_view, atomic_cell_view>> cells;
    collection_mutation_description materialize(const abstract_type&) const;
    collection_mutation serialize(const abstract_type&) const;
};
using collection_mutation_input_stream = struct {};
collection_mutation_view_description deserialize_collection_mutation(const abstract_type&, collection_mutation_input_stream&);
class collection_mutation_view {
public:
    bool is_empty() const;
    bool is_any_live(const abstract_type&, tombstone t = tombstone(), gc_clock::time_point tp = gc_clock::time_point::min()) const;
    api::timestamp_type last_update(const abstract_type&) const;
    class printer {
        const abstract_type& _type;
        const collection_mutation_view& _cmv;
    public:
        printer(const abstract_type& type, const collection_mutation_view& cmv)
                : _type(type), _cmv(cmv) {}
        friend std::ostream& operator<<(std::ostream& os, const printer& cmvp);
    };
};
class collection_mutation {
public:
    collection_mutation() {}
    collection_mutation(const abstract_type&, collection_mutation_view);
    collection_mutation(const abstract_type& type, const bytes_ostream& data);
    operator collection_mutation_view() const;
};
collection_mutation merge(const abstract_type&, collection_mutation_view, collection_mutation_view);
collection_mutation difference(const abstract_type&, collection_mutation_view, collection_mutation_view);
bytes serialize_for_cql(const abstract_type&, collection_mutation_view, cql_serialization_format);
class atomic_cell_or_collection final {
public:
    atomic_cell_or_collection() = default;
    atomic_cell_or_collection(atomic_cell_or_collection&&) = default;
    atomic_cell_or_collection(const atomic_cell_or_collection&) = delete;
    atomic_cell_or_collection& operator=(atomic_cell_or_collection&&) = default;
    atomic_cell_or_collection& operator=(const atomic_cell_or_collection&) = delete;
    atomic_cell_or_collection(atomic_cell ac);
    atomic_cell_or_collection(const abstract_type& at, atomic_cell_view acv);
    static atomic_cell_or_collection from_atomic_cell(atomic_cell data);
    atomic_cell_view as_atomic_cell(const column_definition& cdef) const;
    atomic_cell_ref as_atomic_cell_ref(const column_definition& cdef);
    atomic_cell_mutable_view as_mutable_atomic_cell(const column_definition& cdef);
    atomic_cell_or_collection(collection_mutation cm);
    atomic_cell_or_collection copy(const abstract_type&) const;
    explicit operator bool() const;
    static constexpr bool can_use_mutable_view();
    void swap(atomic_cell_or_collection& other) noexcept;
    static atomic_cell_or_collection from_collection_mutation(collection_mutation data);
    collection_mutation_view as_collection_mutation() const;
    bytes_view serialize() const;
    bool equals(const abstract_type& type, const atomic_cell_or_collection& other) const;
    size_t external_memory_usage(const abstract_type&) const;
    class printer {
    public:
        printer(const column_definition& cdef, const atomic_cell_or_collection& cell);
        printer(const printer&) = delete;
        printer(printer&&) = delete;
        friend std::ostream& operator<<(std::ostream&, const printer&);
    };
    friend std::ostream& operator<<(std::ostream&, const printer&);
};
namespace std {
void swap(atomic_cell_or_collection& a, atomic_cell_or_collection& b) noexcept;
}
namespace query {
enum class digest_algorithm : uint8_t {
    none = 0,  
    MD5 = 1,
    xxHash = 2,
};
}
namespace query {
class result_memory_accounter;
class result_memory_limiter {
    const size_t _maximum_total_result_memory;
    semaphore _memory_limiter;
public:
    static constexpr size_t minimum_result_size = 4 * 1024;
    static constexpr size_t maximum_result_size = 1 * 1024 * 1024;
public:
    explicit result_memory_limiter(size_t maximum_total_result_memory)
        : _maximum_total_result_memory(maximum_total_result_memory)
        , _memory_limiter(_maximum_total_result_memory)
    { }
    result_memory_limiter(const result_memory_limiter&) = delete;
    result_memory_limiter(result_memory_limiter&&) = delete;
    ssize_t total_used_memory() const {
        return _maximum_total_result_memory - _memory_limiter.available_units();
    }
    future<result_memory_accounter> new_mutation_read(size_t max_result_size);
    future<result_memory_accounter> new_data_read(size_t max_result_size);
    future<result_memory_accounter> new_digest_read(size_t max_result_size);
    stop_iteration check() const {
        return stop_iteration(_memory_limiter.current() <= 0);
    }
    stop_iteration update_and_check(size_t n) {
        _memory_limiter.consume(n);
        return check();
    }
    void release(size_t n) noexcept {
        _memory_limiter.signal(n);
    }
    semaphore& sem() noexcept { return _memory_limiter; }
};
class result_memory_tracker {
    semaphore_units<> _units;
    size_t _used_memory;
private:
    static thread_local semaphore _dummy;
public:
    result_memory_tracker() noexcept : _units(_dummy, 0), _used_memory(0) { }
    result_memory_tracker(semaphore& sem, size_t blocked, size_t used) noexcept
        : _units(sem, blocked), _used_memory(used) { }
    size_t used_memory() const { return _used_memory; }
};
class result_memory_accounter {
    result_memory_limiter* _limiter = nullptr;
    size_t _blocked_bytes = 0;
    size_t _used_memory = 0;
    size_t _total_used_memory = 0;
    size_t _maximum_result_size = 0;
    stop_iteration _stop_on_global_limit;
private:
    struct mutation_query_tag { };
    explicit result_memory_accounter(mutation_query_tag, result_memory_limiter& limiter, size_t max_size) noexcept
        : _limiter(&limiter)
        , _blocked_bytes(result_memory_limiter::minimum_result_size)
        , _maximum_result_size(max_size)
        , _stop_on_global_limit(true)
    { }
    struct data_query_tag { };
    explicit result_memory_accounter(data_query_tag, result_memory_limiter& limiter, size_t max_size) noexcept
        : _limiter(&limiter)
        , _blocked_bytes(result_memory_limiter::minimum_result_size)
        , _maximum_result_size(max_size)
    { }
    struct digest_query_tag { };
    explicit result_memory_accounter(digest_query_tag, result_memory_limiter&, size_t max_size) noexcept
        : _blocked_bytes(0)
        , _maximum_result_size(max_size)
    { }
    friend class result_memory_limiter;
public:
    result_memory_accounter() = default;
    result_memory_accounter(result_memory_accounter&& other) noexcept
        : _limiter(std::exchange(other._limiter, nullptr))
        , _blocked_bytes(other._blocked_bytes)
        , _used_memory(other._used_memory)
        , _total_used_memory(other._total_used_memory)
        , _maximum_result_size(other._maximum_result_size)
        , _stop_on_global_limit(other._stop_on_global_limit)
    { }
    result_memory_accounter& operator=(result_memory_accounter&& other) noexcept {
        if (this != &other) {
            this->~result_memory_accounter();
            new (this) result_memory_accounter(std::move(other));
        }
        return *this;
    }
    ~result_memory_accounter() {
        if (_limiter) {
            _limiter->release(_blocked_bytes);
        }
    }
    size_t used_memory() const { return _used_memory; }
    stop_iteration update_and_check(size_t n) {
        _used_memory += n;
        _total_used_memory += n;
        auto stop = stop_iteration(_total_used_memory > _maximum_result_size);
        if (_limiter && _used_memory > _blocked_bytes) {
            auto to_block = std::min(_used_memory - _blocked_bytes, n);
            _blocked_bytes += to_block;
            stop = (_limiter->update_and_check(to_block) && _stop_on_global_limit) || stop;
        }
        return stop;
    }
    stop_iteration check() const {
        stop_iteration stop { _total_used_memory > result_memory_limiter::maximum_result_size };
        if (!stop && _used_memory >= _blocked_bytes && _limiter) {
            return _limiter->check() && _stop_on_global_limit;
        }
        return stop;
    }
    void update(size_t n) {
        update_and_check(n);
    }
    result_memory_tracker done() && {
        if (!_limiter) {
            return { };
        }
        auto& sem = std::exchange(_limiter, nullptr)->sem();
        return result_memory_tracker(sem, _blocked_bytes, _used_memory);
    }
};
inline future<result_memory_accounter> result_memory_limiter::new_mutation_read(size_t max_size) {
    return _memory_limiter.wait(minimum_result_size).then([this, max_size] {
        return result_memory_accounter(result_memory_accounter::mutation_query_tag(), *this, max_size);
    });
}
inline future<result_memory_accounter> result_memory_limiter::new_data_read(size_t max_size) {
    return _memory_limiter.wait(minimum_result_size).then([this, max_size] {
        return result_memory_accounter(result_memory_accounter::data_query_tag(), *this, max_size);
    });
}
inline future<result_memory_accounter> result_memory_limiter::new_digest_read(size_t max_size) {
    return make_ready_future<result_memory_accounter>(result_memory_accounter(result_memory_accounter::digest_query_tag(), *this, max_size));
}
enum class result_request {
    only_result,
    only_digest,
    result_and_digest,
};
struct result_options {
    result_request request = result_request::only_result;
    digest_algorithm digest_algo = query::digest_algorithm::none;
    static result_options only_result() {
        return result_options{};
    }
    static result_options only_digest(digest_algorithm da) {
        return {result_request::only_digest, da};
    }
};
class result_digest {
public:
    using type = std::array<uint8_t, 16>;
private:
    type _digest;
public:
    result_digest() = default;
    result_digest(type&& digest) : _digest(std::move(digest)) {}
    const type& get() const { return _digest; }
    bool operator==(const result_digest& rh) const {
        return _digest == rh._digest;
    }
    bool operator!=(const result_digest& rh) const {
        return _digest != rh._digest;
    }
};
struct short_read_tag { };
using short_read = bool_class<short_read_tag>;
class result {
    bytes_ostream _w;
    std::optional<result_digest> _digest;
    std::optional<uint32_t> _row_count;
    api::timestamp_type _last_modified = api::missing_timestamp;
    short_read _short_read;
    query::result_memory_tracker _memory_tracker;
    std::optional<uint32_t> _partition_count;
public:
    class builder;
    class partition_writer;
    friend class result_merger;
    result();
    result(bytes_ostream&& w, short_read sr, std::optional<uint32_t> c, std::optional<uint32_t> pc,
           result_memory_tracker memory_tracker = { })
        : _w(std::move(w))
        , _row_count(c)
        , _short_read(sr)
        , _memory_tracker(std::move(memory_tracker))
        , _partition_count(pc)
    {
        w.reduce_chunk_count();
    }
    result(bytes_ostream&& w, std::optional<result_digest> d, api::timestamp_type last_modified,
           short_read sr, std::optional<uint32_t> c, std::optional<uint32_t> pc, result_memory_tracker memory_tracker = { })
        : _w(std::move(w))
        , _digest(d)
        , _row_count(c)
        , _last_modified(last_modified)
        , _short_read(sr)
        , _memory_tracker(std::move(memory_tracker))
        , _partition_count(pc)
    {
        w.reduce_chunk_count();
    }
    result(result&&) = default;
    result(const result&) = default;
    result& operator=(result&&) = default;
    result& operator=(const result&) = default;
    const bytes_ostream& buf() const {
        return _w;
    }
    const std::optional<result_digest>& digest() const {
        return _digest;
    }
    const std::optional<uint32_t>& row_count() const {
        return _row_count;
    }
    const api::timestamp_type last_modified() const {
        return _last_modified;
    }
    short_read is_short_read() const {
        return _short_read;
    }
    const std::optional<uint32_t>& partition_count() const {
        return _partition_count;
    }
    void ensure_counts();
    struct printer {
        schema_ptr s;
        const query::partition_slice& slice;
        const query::result& res;
    };
    sstring pretty_print(schema_ptr, const query::partition_slice&) const;
    printer pretty_printer(schema_ptr, const query::partition_slice&) const;
};
std::ostream& operator<<(std::ostream& os, const query::result::printer&);
}
#include <boost/intrusive/set.hpp>
namespace bi = boost::intrusive;
/**
 * Represents a ranged deletion operation. Can be empty.
 */
class range_tombstone final {
    bi::set_member_hook<bi::link_mode<bi::auto_unlink>> _link;
public:
    clustering_key_prefix start;
    bound_kind start_kind;
    clustering_key_prefix end;
    bound_kind end_kind;
    tombstone tomb;
    range_tombstone(clustering_key_prefix start, bound_kind start_kind, clustering_key_prefix end, bound_kind end_kind, tombstone tomb)
            : start(std::move(start))
            , start_kind(start_kind)
            , end(std::move(end))
            , end_kind(end_kind)
            , tomb(std::move(tomb))
    { }
    range_tombstone(bound_view start, bound_view end, tombstone tomb)
            : range_tombstone(start.prefix(), start.kind(), end.prefix(), end.kind(), std::move(tomb))
    { }
    range_tombstone(position_in_partition_view start, position_in_partition_view end, tombstone tomb)
            : range_tombstone(start.as_start_bound_view(), end.as_end_bound_view(), tomb)
    {}
    range_tombstone(clustering_key_prefix&& start, clustering_key_prefix&& end, tombstone tomb)
            : range_tombstone(std::move(start), bound_kind::incl_start, std::move(end), bound_kind::incl_end, std::move(tomb))
    { }
    range_tombstone(clustering_key_prefix&& start, tombstone tomb, bound_kind start_kind, clustering_key_prefix&& end, bound_kind end_kind)
            : range_tombstone(std::move(start), start_kind, std::move(end), end_kind, std::move(tomb))
    { }
    range_tombstone(range_tombstone&& rt) noexcept
            : range_tombstone(std::move(rt.start), rt.start_kind, std::move(rt.end), rt.end_kind, std::move(rt.tomb)) {
        update_node(rt._link);
    }
    struct without_link { };
    range_tombstone(range_tombstone&& rt, without_link) noexcept
            : range_tombstone(std::move(rt.start), rt.start_kind, std::move(rt.end), rt.end_kind, std::move(rt.tomb)) {
    }
    range_tombstone(const range_tombstone& rt)
            : range_tombstone(rt.start, rt.start_kind, rt.end, rt.end_kind, rt.tomb)
    { }
    range_tombstone& operator=(range_tombstone&& rt) noexcept {
        update_node(rt._link);
        move_assign(std::move(rt));
        return *this;
    }
    range_tombstone& operator=(const range_tombstone& rt) {
        start = rt.start;
        start_kind = rt.start_kind;
        end = rt.end;
        end_kind = rt.end_kind;
        tomb = rt.tomb;
        return *this;
    }
    const bound_view start_bound() const {
        return bound_view(start, start_kind);
    }
    const bound_view end_bound() const {
        return bound_view(end, end_kind);
    }
    position_in_partition_view position() const;
    position_in_partition_view end_position() const;
    bool empty() const {
        return !bool(tomb);
    }
    explicit operator bool() const {
        return bool(tomb);
    }
    bool equal(const schema& s, const range_tombstone& other) const {
        return tomb == other.tomb && start_bound().equal(s, other.start_bound()) && end_bound().equal(s, other.end_bound());
    }
    struct compare {
        bound_view::compare _c;
        compare(const schema& s) : _c(s) {}
        bool operator()(const range_tombstone& rt1, const range_tombstone& rt2) const {
            return _c(rt1.start_bound(), rt2.start_bound());
        }
    };
    friend void swap(range_tombstone& rt1, range_tombstone& rt2) {
        range_tombstone tmp(std::move(rt2), without_link());
        rt2.move_assign(std::move(rt1));
        rt1.move_assign(std::move(tmp));
    }
    friend std::ostream& operator<<(std::ostream& out, const range_tombstone& rt);
    using container_type = bi::set<range_tombstone,
            bi::member_hook<range_tombstone, bi::set_member_hook<bi::link_mode<bi::auto_unlink>>, &range_tombstone::_link>,
            bi::compare<range_tombstone::compare>,
            bi::constant_time_size<false>>;
    static bool is_single_clustering_row_tombstone(const schema& s, const clustering_key_prefix& start,
        bound_kind start_kind, const clustering_key_prefix& end, bound_kind end_kind)
    {
        return start.is_full(s) && start_kind == bound_kind::incl_start
            && end_kind == bound_kind::incl_end && start.equal(s, end);
    }
    std::optional<range_tombstone> apply(const schema& s, range_tombstone&& src);
    bool trim_front(const schema& s, position_in_partition_view pos) {
        position_in_partition::less_compare less(s);
        if (!less(pos, end_position())) {
            return false;
        }
        if (less(position(), pos)) {
            set_start(s, pos);
        }
        return true;
    }
    void set_start(const schema& s, position_in_partition_view pos) {
        bound_view new_start = pos.as_start_bound_view();
        start = new_start.prefix();
        start_kind = new_start.kind();
    }
    size_t external_memory_usage(const schema&) const {
        return start.external_memory_usage() + end.external_memory_usage();
    }
    size_t memory_usage(const schema& s) const {
        return sizeof(range_tombstone) + external_memory_usage(s);
    }
private:
    void move_assign(range_tombstone&& rt) {
        start = std::move(rt.start);
        start_kind = rt.start_kind;
        end = std::move(rt.end);
        end_kind = rt.end_kind;
        tomb = std::move(rt.tomb);
    }
    void update_node(bi::set_member_hook<bi::link_mode<bi::auto_unlink>>& other_link) {
        if (other_link.is_linked()) {
            container_type::node_algorithms::replace_node(other_link.this_ptr(), _link.this_ptr());
            container_type::node_algorithms::init(other_link.this_ptr());
        }
    }
};
template<>
struct appending_hash<range_tombstone>  {
    template<typename Hasher>
    void operator()(Hasher& h, const range_tombstone& value, const schema& s) const {
        feed_hash(h, value.start, s);
        if (!value.start.equal(s, value.end) || value.start_kind != bound_kind::incl_start || value.end_kind != bound_kind::incl_end) {
            feed_hash(h, value.start_kind);
            feed_hash(h, value.end, s);
            feed_hash(h, value.end_kind);
        }
        feed_hash(h, value.tomb);
    }
};
class range_tombstone_accumulator {
    bound_view::compare _cmp;
    tombstone _partition_tombstone;
    std::deque<range_tombstone> _range_tombstones;
    tombstone _current_tombstone;
    bool _reversed;
private:
    void update_current_tombstone();
    void drop_unneeded_tombstones(const clustering_key_prefix& ck, int w = 0);
public:
    range_tombstone_accumulator(const schema& s, bool reversed)
        : _cmp(s), _reversed(reversed) { }
    void set_partition_tombstone(tombstone t) {
        _partition_tombstone = t;
        update_current_tombstone();
    }
    tombstone get_partition_tombstone() const {
        return _partition_tombstone;
    }
    tombstone current_tombstone() const {
        return _current_tombstone;
    }
    tombstone tombstone_for_row(const clustering_key_prefix& ck) {
        drop_unneeded_tombstones(ck);
        return _current_tombstone;
    }
    const std::deque<range_tombstone>& range_tombstones_for_row(const clustering_key_prefix& ck) {
        drop_unneeded_tombstones(ck);
        return _range_tombstones;
    }
    std::deque<range_tombstone> range_tombstones() && {
        return std::move(_range_tombstones);
    }
    void apply(range_tombstone rt);
    void clear();
};
class row_marker;
class row_tombstone;
using is_continuous = bool_class<class continuous_tag>;
class dummy_tag {};
using is_dummy = bool_class<dummy_tag>;
class mutation_partition_visitor {
public:
    virtual void accept_partition_tombstone(tombstone) = 0;
    virtual void accept_static_cell(column_id, atomic_cell_view) = 0;
    virtual void accept_static_cell(column_id, collection_mutation_view) = 0;
    virtual void accept_row_tombstone(const range_tombstone&) = 0;
    virtual void accept_row(position_in_partition_view key, const row_tombstone& deleted_at, const row_marker& rm,
        is_dummy = is_dummy::no, is_continuous = is_continuous::yes) = 0;
    virtual void accept_row_cell(column_id id, atomic_cell_view) = 0;
    virtual void accept_row_cell(column_id id, collection_mutation_view) = 0;
};
namespace utils {
using input_stream = seastar::memory_input_stream<bytes_ostream::fragment_iterator>;
}
namespace ser {
class mutation_partition_view;
}
class partition_builder;
class converting_mutation_partition_applier;
GCC6_CONCEPT(
template<typename T>
concept bool MutationViewVisitor = requires (T& visitor, tombstone t, atomic_cell ac,
                                             collection_mutation_view cmv, range_tombstone rt,
                                             position_in_partition_view pipv, row_tombstone row_tomb,
                                             row_marker rm) {
    visitor.accept_partition_tombstone(t);
    visitor.accept_static_cell(column_id(), std::move(ac));
    visitor.accept_static_cell(column_id(), cmv);
    visitor.accept_row_tombstone(rt);
    visitor.accept_row(pipv, row_tomb, rm,
            is_dummy::no, is_continuous::yes);
    visitor.accept_row_cell(column_id(), std::move(ac));
    visitor.accept_row_cell(column_id(), cmv);
};
)
class mutation_partition_view_virtual_visitor {
public:
    virtual ~mutation_partition_view_virtual_visitor();
    virtual void accept_partition_tombstone(tombstone t) = 0;
    virtual void accept_static_cell(column_id, atomic_cell ac) = 0;
    virtual void accept_static_cell(column_id, collection_mutation_view cmv) = 0;
    virtual void accept_row_tombstone(range_tombstone rt) = 0;
    virtual void accept_row(position_in_partition_view pipv, row_tombstone rt, row_marker rm, is_dummy, is_continuous) = 0;
    virtual void accept_row_cell(column_id, atomic_cell ac) = 0;
    virtual void accept_row_cell(column_id, collection_mutation_view cmv) = 0;
};
class mutation_partition_view {
    utils::input_stream _in;
private:
    mutation_partition_view(utils::input_stream v)
        : _in(v)
    { }
    template<typename Visitor>
    GCC6_CONCEPT(requires MutationViewVisitor<Visitor>)
    void do_accept(const column_mapping&, Visitor& visitor) const;
public:
    static mutation_partition_view from_stream(utils::input_stream v) {
        return { v };
    }
    static mutation_partition_view from_view(ser::mutation_partition_view v);
    void accept(const schema& schema, partition_builder& visitor) const;
    void accept(const column_mapping&, converting_mutation_partition_applier& visitor) const;
    void accept(const column_mapping&, mutation_partition_view_virtual_visitor& mpvvv) const;
    std::optional<clustering_key> first_row_key() const;
    std::optional<clustering_key> last_row_key() const;
};
template<typename T, unsigned InternalSize = 0, typename SizeType = size_t>
class managed_vector {
    static_assert(std::is_nothrow_move_constructible<T>::value,
        "objects stored in managed_vector need to be nothrow move-constructible");
public:
    using value_type = T;
    using size_type = SizeType;
    using iterator = T*;
    using const_iterator = const T*;
private:
    struct external {
        managed_vector* _backref;
        T _data[0];
        external(external&& other) noexcept : _backref(other._backref) {
            for (unsigned i = 0; i < _backref->size(); i++) {
                new (_data + i) T(std::move(other._data[i]));
                other._data[i].~T();
            }
            _backref->_data = _data;
        }
        size_t storage_size() const {
            return sizeof(*this) + sizeof(T[_backref->_capacity]);
        }
        friend size_t size_for_allocation_strategy(const external& obj) {
            return obj.storage_size();
        }
    };
    union maybe_constructed {
        maybe_constructed() { }
        ~maybe_constructed() { }
        T object;
    };
private:
    std::array<maybe_constructed, InternalSize> _internal;
    size_type _size = 0;
    size_type _capacity = InternalSize;
    T* _data = reinterpret_cast<T*>(_internal.data());
    friend class external;
private:
    bool is_external() const {
        return _data != reinterpret_cast<const T*>(_internal.data());
    }
    external* get_external() {
        auto ptr = reinterpret_cast<char*>(_data) - offsetof(external, _data);
        return reinterpret_cast<external*>(ptr);
    }
    void maybe_grow(size_type new_size) {
        if (new_size <= _capacity) {
            return;
        }
        auto new_capacity = std::max({ _capacity + std::min(_capacity, size_type(1024)), new_size, size_type(InternalSize + 8) });
        reserve(new_capacity);
    }
    void clear_and_release() noexcept {
        clear();
        if (is_external()) {
            current_allocator().free(get_external(), get_external()->storage_size());
        }
    }
public:
    managed_vector() = default;
    managed_vector(const managed_vector& other) {
        reserve(other._size);
        try {
            for (const auto& v : other) {
                push_back(v);
            }
        } catch (...) {
            clear_and_release();
            throw;
        }
    }
    managed_vector(managed_vector&& other) noexcept : _size(other._size), _capacity(other._capacity) {
        if (other.is_external()) {
            _data = other._data;
            other._data = reinterpret_cast<T*>(other._internal.data());
            get_external()->_backref = this;
        } else {
            for (unsigned i = 0; i < _size; i++) {
                new (_data + i) T(std::move(other._data[i]));
                other._data[i].~T();
            }
        }
        other._size = 0;
        other._capacity = InternalSize;
    }
    managed_vector& operator=(const managed_vector& other) {
        if (this != &other) {
            managed_vector tmp(other);
            this->~managed_vector();
            new (this) managed_vector(std::move(tmp));
        }
        return *this;
    }
    managed_vector& operator=(managed_vector&& other) noexcept {
        if (this != &other) {
            this->~managed_vector();
            new (this) managed_vector(std::move(other));
        }
        return *this;
    }
    ~managed_vector() {
        clear_and_release();
    }
    T& at(size_type pos) {
        if (pos >= _size) {
            throw std::out_of_range("out of range");
        }
        return operator[](pos);
    }
    const T& at(size_type pos) const {
        if (pos >= _size) {
            throw std::out_of_range("out of range");
        }
        return operator[](pos);
    }
    T& operator[](size_type pos) noexcept {
        return _data[pos];
    }
    const T& operator[](size_type pos) const noexcept {
        return _data[pos];
    }
    T& front() noexcept { return *_data; }
    const T& front() const noexcept { return *_data;  }
    T& back() noexcept { return _data[_size - 1]; }
    const T& back() const noexcept { return _data[_size - 1]; }
    T* data() noexcept { return _data; }
    const T* data() const noexcept { return _data; }
    iterator begin() noexcept { return _data; }
    const_iterator begin() const noexcept { return _data; }
    const_iterator cbegin() const noexcept { return _data; }
    iterator end() noexcept { return _data + _size; }
    const_iterator end() const noexcept { return _data + _size; }
    const_iterator cend() const noexcept { return _data + _size; }
    bool empty() const noexcept { return !_size; }
    size_type size() const noexcept { return _size; }
    size_type capacity() const noexcept { return _capacity; }
    void clear() {
        while (_size) {
            pop_back();
        }
    }
    void reserve(size_type new_capacity) {
        if (new_capacity <= _capacity) {
            return;
        }
        auto ptr = current_allocator().alloc(&get_standard_migrator<external>(),
            sizeof(external) + sizeof(T) * new_capacity, alignof(external));
        auto ext = static_cast<external*>(ptr);
        ext->_backref = this;
        T* data_ptr = ext->_data;
        for (unsigned i = 0; i < _size; i++) {
            new (data_ptr + i) T(std::move(_data[i]));
            _data[i].~T();
        }
        if (is_external()) {
            current_allocator().free(get_external(), get_external()->storage_size());
        }
        _data = data_ptr;
        _capacity = new_capacity;
    }
    iterator erase(iterator it) {
        std::move(it + 1, end(), it);
        _data[_size - 1].~T();
        _size--;
        return it;
    }
    void push_back(const T& value) {
        emplace_back(value);
    }
    void push_back(T&& value) {
        emplace_back(std::move(value));
    }
    template<typename... Args>
    T& emplace_back(Args&&... args) {
        maybe_grow(_size + 1);
        T* elem = new (_data + _size) T(std::forward<Args>(args)...);
        _size++;
        return *elem;
    }
    void pop_back() {
        _data[_size - 1].~T();
        _size--;
    }
    void resize(size_type new_size) {
        maybe_grow(new_size);
        while (_size > new_size) {
            pop_back();
        }
        while (_size < new_size) {
            emplace_back();
        }
    }
    void resize(size_type new_size, const T& value) {
        maybe_grow(new_size);
        while (_size > new_size) {
            pop_back();
        }
        while (_size < new_size) {
            push_back(value);
        }
    }
    size_t used_space_external_memory_usage() const {
        if (is_external()) {
            return sizeof(external) + _size * sizeof(T);
        }
        return 0;
    }
};
class is_preemptible_tag;
using is_preemptible = bool_class<is_preemptible_tag>;
class range_tombstone_list final {
    using range_tombstones_type = range_tombstone::container_type;
    class insert_undo_op {
        const range_tombstone& _new_rt;
    public:
        insert_undo_op(const range_tombstone& new_rt)
                : _new_rt(new_rt) { }
        void undo(const schema& s, range_tombstone_list& rt_list) noexcept;
    };
    class erase_undo_op {
        alloc_strategy_unique_ptr<range_tombstone> _rt;
    public:
        erase_undo_op(range_tombstone& rt)
                : _rt(&rt) { }
        void undo(const schema& s, range_tombstone_list& rt_list) noexcept;
    };
    class update_undo_op {
        range_tombstone _old_rt;
        const range_tombstone& _new_rt;
    public:
        update_undo_op(range_tombstone&& old_rt, const range_tombstone& new_rt)
                : _old_rt(std::move(old_rt)), _new_rt(new_rt) { }
        void undo(const schema& s, range_tombstone_list& rt_list) noexcept;
    };
    class reverter {
    private:
        using op = std::variant<erase_undo_op, insert_undo_op, update_undo_op>;
        std::vector<op> _ops;
        const schema& _s;
    protected:
        range_tombstone_list& _dst;
    public:
        reverter(const schema& s, range_tombstone_list& dst)
                : _s(s)
                , _dst(dst) { }
        virtual ~reverter() {
            revert();
        }
        reverter(reverter&&) = default;
        reverter& operator=(reverter&&) = default;
        reverter(const reverter&) = delete;
        reverter& operator=(reverter&) = delete;
        virtual range_tombstones_type::iterator insert(range_tombstones_type::iterator it, range_tombstone& new_rt);
        virtual range_tombstones_type::iterator erase(range_tombstones_type::iterator it);
        virtual void update(range_tombstones_type::iterator it, range_tombstone&& new_rt);
        void revert() noexcept;
        void cancel() noexcept {
            _ops.clear();
        }
    };
    class nop_reverter : public reverter {
    public:
        nop_reverter(const schema& s, range_tombstone_list& rt_list)
                : reverter(s, rt_list) { }
        virtual range_tombstones_type::iterator insert(range_tombstones_type::iterator it, range_tombstone& new_rt) override;
        virtual range_tombstones_type::iterator erase(range_tombstones_type::iterator it) override;
        virtual void update(range_tombstones_type::iterator it, range_tombstone&& new_rt) override;
    };
private:
    range_tombstones_type _tombstones;
public:
    using iterator = range_tombstones_type::iterator;
    using const_iterator = range_tombstones_type::const_iterator;
    struct copy_comparator_only { };
    range_tombstone_list(const schema& s)
        : _tombstones(range_tombstone::compare(s))
    { }
    range_tombstone_list(const range_tombstone_list& x, copy_comparator_only)
        : _tombstones(x._tombstones.key_comp())
    { }
    range_tombstone_list(const range_tombstone_list&);
    range_tombstone_list& operator=(range_tombstone_list&) = delete;
    range_tombstone_list(range_tombstone_list&&) = default;
    range_tombstone_list& operator=(range_tombstone_list&&) = default;
    ~range_tombstone_list();
    size_t size() const {
        return _tombstones.size();
    }
    bool empty() const {
        return _tombstones.empty();
    }
    range_tombstones_type& tombstones() {
        return _tombstones;
    }
    auto begin() {
        return _tombstones.begin();
    }
    auto begin() const {
        return _tombstones.begin();
    }
    auto end() {
        return _tombstones.end();
    }
    auto end() const {
        return _tombstones.end();
    }
    void apply(const schema& s, const bound_view& start_bound, const bound_view& end_bound, tombstone tomb) {
        apply(s, start_bound.prefix(), start_bound.kind(), end_bound.prefix(), end_bound.kind(), std::move(tomb));
    }
    void apply(const schema& s, const range_tombstone& rt) {
        apply(s, rt.start, rt.start_kind, rt.end, rt.end_kind, rt.tomb);
    }
    void apply(const schema& s, range_tombstone&& rt) {
        apply(s, std::move(rt.start), rt.start_kind, std::move(rt.end), rt.end_kind, std::move(rt.tomb));
    }
    void apply(const schema& s, clustering_key_prefix start, bound_kind start_kind,
               clustering_key_prefix end, bound_kind end_kind, tombstone tomb) {
        nop_reverter rev(s, *this);
        apply_reversibly(s, std::move(start), start_kind, std::move(end), end_kind, std::move(tomb), rev);
    }
    void apply_monotonically(const schema& s, const range_tombstone& rt);
    void apply_monotonically(const schema& s, const range_tombstone_list& list);
    stop_iteration apply_monotonically(const schema& s, range_tombstone_list&& list, is_preemptible = is_preemptible::no);
public:
    tombstone search_tombstone_covering(const schema& s, const clustering_key_prefix& key) const;
    boost::iterator_range<const_iterator> slice(const schema& s, const query::clustering_range&) const;
    boost::iterator_range<const_iterator> slice(const schema& s, position_in_partition_view start, position_in_partition_view end) const;
    iterator erase(const_iterator, const_iterator);
    void trim(const schema& s, const query::clustering_row_ranges&);
    range_tombstone_list difference(const schema& s, const range_tombstone_list& rt_list) const;
    template <typename Pred>
    void erase_where(Pred filter) {
        static_assert(std::is_same<bool, std::result_of_t<Pred(const range_tombstone&)>>::value,
                      "bad Pred signature");
        auto it = begin();
        while (it != end()) {
            if (filter(*it)) {
                it = _tombstones.erase_and_dispose(it, current_deleter<range_tombstone>());
            } else {
                ++it;
            }
        }
    }
    void clear() {
        _tombstones.clear_and_dispose(current_deleter<range_tombstone>());
    }
    stop_iteration clear_gently() noexcept;
    void apply(const schema& s, const range_tombstone_list& rt_list);
    reverter apply_reversibly(const schema& s, range_tombstone_list& rt_list);
    friend std::ostream& operator<<(std::ostream& out, const range_tombstone_list&);
    bool equal(const schema&, const range_tombstone_list&) const;
    size_t external_memory_usage(const schema& s) const {
        size_t result = 0;
        for (auto& rtb : _tombstones) {
            result += rtb.memory_usage(s);
        }
        return result;
    }
private:
    void apply_reversibly(const schema& s, clustering_key_prefix start, bound_kind start_kind,
                          clustering_key_prefix end, bound_kind end_kind, tombstone tomb, reverter& rev);
    void insert_from(const schema& s, range_tombstones_type::iterator it, clustering_key_prefix start,
                     bound_kind start_kind, clustering_key_prefix end, bound_kind end_kind, tombstone tomb, reverter& rev);
    range_tombstones_type::iterator find(const schema& s, const range_tombstone& rt);
};
namespace query {
class clustering_key_filter_ranges {
public:
    clustering_key_filter_ranges(const clustering_row_ranges& ranges);
    struct reversed { };
    clustering_key_filter_ranges(reversed, const clustering_row_ranges& ranges);
    clustering_key_filter_ranges(clustering_key_filter_ranges&& other) noexcept;
    clustering_key_filter_ranges& operator=(clustering_key_filter_ranges&& other) noexcept;
    static clustering_key_filter_ranges get_ranges(const schema& schema, const query::partition_slice& slice, const partition_key& key);
};
}
template<typename T>
class managed;
template<typename T>
struct managed_ref {
    managed<T>* _ptr;
    managed_ref() : _ptr(nullptr) {}
    managed_ref(const managed_ref&) = delete;
    managed_ref(managed_ref&& other) noexcept
        : _ptr(other._ptr)
    {
        other._ptr = nullptr;
        if (_ptr) {
            _ptr->_backref = &_ptr;
        }
    }
    ~managed_ref() {
        if (_ptr) {
            current_allocator().destroy(_ptr);
        }
    }
    managed_ref& operator=(managed_ref&& o) {
        this->~managed_ref();
        new (this) managed_ref(std::move(o));
        return *this;
    }
    T* get() {
        return _ptr ? &_ptr->_value : nullptr;
    }
    const T* get() const {
        return _ptr ? &_ptr->_value : nullptr;
    }
    T& operator*() {
        return _ptr->_value;
    }
    const T& operator*() const {
        return _ptr->_value;
    }
    T* operator->() {
        return &_ptr->_value;
    }
    const T* operator->() const {
        return &_ptr->_value;
    }
    explicit operator bool() const {
        return _ptr != nullptr;
    }
    size_t external_memory_usage() const {
        return _ptr ? current_allocator().object_memory_size_in_allocator(_ptr) : 0;
    }
};
template<typename T>
class managed {
    managed<T>** _backref;
    T _value;
    template<typename T_>
    friend struct managed_ref;
public:
    static_assert(std::is_nothrow_move_constructible<T>::value, "Throwing move constructor not supported");
    managed(managed<T>** backref, T&& v) noexcept
        : _backref(backref)
        , _value(std::move(v))
    {
        *_backref = this;
    }
    managed(managed&& other) noexcept
        : _backref(other._backref)
        , _value(std::move(other._value))
    {
        *_backref = this;
    }
};
template<typename T, typename... Args>
managed_ref<T>
make_managed(Args&&... args) {
    managed_ref<T> ref;
    current_allocator().construct<managed<T>>(&ref._ptr, T(std::forward<Args>(args)...));
    return ref;
}
class mutation_fragment;
class clustering_row;
struct cell_hash {
    using size_type = uint64_t;
    static constexpr size_type no_hash = 0;
    size_type hash = no_hash;
    explicit operator bool() const noexcept;
};
using cell_hash_opt = seastar::optimized_optional<cell_hash>;
struct cell_and_hash {
    mutable cell_hash_opt hash;
    cell_and_hash() = default;
    cell_and_hash(cell_and_hash&&) noexcept = default;
    cell_and_hash& operator=(cell_and_hash&&) noexcept = default;
};
class compaction_garbage_collector;
class row {
    class cell_entry {
        friend class row;
    public:
        cell_entry(column_id id);
        cell_entry(cell_entry&&) noexcept;
        column_id id() const;
        const cell_hash_opt& hash() const;
        struct compare {
            bool operator()(const cell_entry& e1, const cell_entry& e2) const ;
            bool operator()(column_id id1, const cell_entry& e2) const;
            bool operator()(const cell_entry& e1, column_id id2) const;
        };
    };
    using size_type = std::make_unsigned_t<column_id>;
public:
    static constexpr size_t max_vector_size = 32;
    static constexpr size_t internal_count = 5;
private:
public:
    row();
    ~row();
    row(const schema&, column_kind, const row&);
    row(row&& other) noexcept;
    row& operator=(row&& other) noexcept;
    size_t size() const;
    bool empty() const;
    void reserve(column_id);
private:
    template<typename Func>
    void remove_if(Func&& func);
private:
    template<typename Func>
    auto with_both_ranges(const row& other, Func&& func) const;
    void vector_to_set();
    template<typename Func>
    void consume_with(Func&&);
public:
    template<typename Func>
    void for_each_cell(Func&& func);
    template<typename Func>
    void for_each_cell(Func&& func) const;
    template<typename Func>
    void for_each_cell_until(Func&& func) const;
    void apply(const column_definition& column, const atomic_cell_or_collection& cell, cell_hash_opt hash = cell_hash_opt());
    void apply(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());
    void apply_monotonically(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());
    void append_cell(column_id id, atomic_cell_or_collection cell);
    void apply(const schema&, column_kind, const row& src);
    void apply(const schema&, column_kind, row&& src);
    void apply_monotonically(const schema&, column_kind, row&& src);
    bool compact_and_expire(
            const schema& s,
            column_kind kind,
            row_tombstone tomb,
            gc_clock::time_point query_time,
            can_gc_fn&,
            gc_clock::time_point gc_before,
            const row_marker& marker,
            compaction_garbage_collector* collector = nullptr);
    bool compact_and_expire(
            const schema& s,
            column_kind kind,
            row_tombstone tomb,
            gc_clock::time_point query_time,
            can_gc_fn&,
            gc_clock::time_point gc_before,
            compaction_garbage_collector* collector = nullptr);
    row difference(const schema&, column_kind, const row& other) const;
    bool equal(column_kind kind, const schema& this_schema, const row& other, const schema& other_schema) const;
    size_t external_memory_usage(const schema&, column_kind) const;
    cell_hash_opt cell_hash_for(column_id id) const;
    void prepare_hash(const schema& s, column_kind kind) const;
    void clear_hash() const;
    bool is_live(const schema&, column_kind kind, tombstone tomb = tombstone(), gc_clock::time_point now = gc_clock::time_point::min()) const;
    class printer {
    public:
        printer(const schema& s, column_kind k, const row& r);
        printer(const printer&) = delete;
        printer(printer&&) = delete;
        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
};
class lazy_row {
public:
    lazy_row() = default;
    explicit lazy_row(row&& r);
    lazy_row(const schema& s, column_kind kind, const lazy_row& r);
    lazy_row(const schema& s, column_kind kind, const row& r);
    row& maybe_create();
    const row& get_existing() const &;
    row& get_existing() &;
    row&& get_existing() &&;
    const row& get() const;
    size_t size() const;
    bool empty() const;
    void reserve(column_id nr);
    const atomic_cell_or_collection& cell_at(column_id id) const;
    const atomic_cell_or_collection* find_cell(column_id id) const;
    const cell_and_hash* find_cell_and_hash(column_id id) const;
    template<typename Func>
    void for_each_cell(Func&& func);
    template<typename Func>
    void for_each_cell(Func&& func) const;
    template<typename Func>
    void for_each_cell_until(Func&& func) const;
    void apply(const column_definition& column, const atomic_cell_or_collection& cell, cell_hash_opt hash = cell_hash_opt());
    void apply(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());
    void apply_monotonically(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());
    void append_cell(column_id id, atomic_cell_or_collection cell);
    void apply(const schema& s, column_kind kind, const row& src);
    void apply(const schema& s, column_kind kind, const lazy_row& src);
    void apply(const schema& s, column_kind kind, row&& src);
    void apply_monotonically(const schema& s, column_kind kind, row&& src);
    void apply_monotonically(const schema& s, column_kind kind, lazy_row&& src);
    bool compact_and_expire(
            const schema& s,
            column_kind kind,
            row_tombstone tomb,
            gc_clock::time_point query_time,
            can_gc_fn& can_gc,
            gc_clock::time_point gc_before,
            const row_marker& marker,
            compaction_garbage_collector* collector = nullptr);
    bool compact_and_expire(
            const schema& s,
            column_kind kind,
            row_tombstone tomb,
            gc_clock::time_point query_time,
            can_gc_fn& can_gc,
            gc_clock::time_point gc_before,
            compaction_garbage_collector* collector = nullptr);
    lazy_row difference(const schema& s, column_kind kind, const lazy_row& other) const;
    bool equal(column_kind kind, const schema& this_schema, const lazy_row& other, const schema& other_schema) const;
    size_t external_memory_usage(const schema& s, column_kind kind) const;
    cell_hash_opt cell_hash_for(column_id id) const;
    void prepare_hash(const schema& s, column_kind kind) const;
    void clear_hash() const;
    bool is_live(const schema& s, column_kind kind, tombstone tomb = tombstone(), gc_clock::time_point now = gc_clock::time_point::min()) const;
    class printer {
    public:
        printer(const schema& s, column_kind k, const lazy_row& r);
        printer(const printer&) = delete;
        printer(printer&&) = delete;
        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
};
class row_marker;
int compare_row_marker_for_merge(const row_marker& left, const row_marker& right) noexcept;
class row_marker {
    static constexpr gc_clock::duration no_ttl { 0 };
    static constexpr gc_clock::duration dead { -1 };
    static constexpr gc_clock::time_point no_expiry { gc_clock::duration(0) };
    api::timestamp_type _timestamp = api::missing_timestamp;
    gc_clock::duration _ttl = no_ttl;
    gc_clock::time_point _expiry = no_expiry;
public:
    row_marker() = default;
    explicit row_marker(api::timestamp_type created_at) : _timestamp(created_at) { }
    row_marker(api::timestamp_type created_at, gc_clock::duration ttl, gc_clock::time_point expiry)
        : _timestamp(created_at), _ttl(ttl), _expiry(expiry)
    { }
    explicit row_marker(tombstone deleted_at)
        : _timestamp(deleted_at.timestamp), _ttl(dead), _expiry(deleted_at.deletion_time)
    { }
    bool is_missing() const {
        return _timestamp == api::missing_timestamp;
    }
    bool is_live() const {
        return !is_missing() && _ttl != dead;
    }
    bool is_live(tombstone t, gc_clock::time_point now) const {
        if (is_missing() || _ttl == dead) {
            return false;
        }
        if (_ttl != no_ttl && _expiry <= now) {
            return false;
        }
        return _timestamp > t.timestamp;
    }
    bool is_dead(gc_clock::time_point now) const {
        if (_ttl == dead) {
            return true;
        }
        return _ttl != no_ttl && _expiry <= now;
    }
    bool is_expiring() const {
        return _ttl != no_ttl;
    }
    gc_clock::duration ttl() const {
        return _ttl;
    }
    gc_clock::time_point expiry() const {
        return _expiry;
    }
    gc_clock::time_point deletion_time() const {
        return _ttl == dead ? _expiry : _expiry - _ttl;
    }
    api::timestamp_type timestamp() const {
        return _timestamp;
    }
    void apply(const row_marker& rm) {
        if (compare_row_marker_for_merge(*this, rm) < 0) {
            *this = rm;
        }
    }
    bool compact_and_expire(tombstone tomb, gc_clock::time_point now,
            can_gc_fn& can_gc, gc_clock::time_point gc_before, compaction_garbage_collector* collector = nullptr);
    bool operator==(const row_marker& other) const {
        if (_timestamp != other._timestamp) {
            return false;
        }
        if (is_missing()) {
            return true;
        }
        if (_ttl != other._ttl) {
            return false;
        }
        return _ttl == no_ttl || _expiry == other._expiry;
    }
    bool operator!=(const row_marker& other) const {
        return !(*this == other);
    }
    template<typename Hasher>
    void feed_hash(Hasher& h) const {
        ::feed_hash(h, _timestamp);
        if (!is_missing()) {
            ::feed_hash(h, _ttl);
            if (_ttl != no_ttl) {
                ::feed_hash(h, _expiry);
            }
        }
    }
    friend std::ostream& operator<<(std::ostream& os, const row_marker& rm);
};
class clustering_row;
class shadowable_tombstone : public with_relational_operators<shadowable_tombstone> {
    tombstone _tomb;
public:
    explicit shadowable_tombstone(api::timestamp_type timestamp, gc_clock::time_point deletion_time)
            : _tomb(timestamp, deletion_time) {
    }
    explicit shadowable_tombstone(tombstone tomb = tombstone())
            : _tomb(std::move(tomb)) {
    }
    int compare(const shadowable_tombstone& t) const {
        return _tomb.compare(t._tomb);
    }
    explicit operator bool() const {
        return bool(_tomb);
    }
    const tombstone& tomb() const {
        return _tomb;
    }
    bool is_shadowed_by(const row_marker& marker) const {
        return marker.is_live() && marker.timestamp() > _tomb.timestamp;
    }
    void maybe_shadow(tombstone t, row_marker marker) noexcept {
        if (is_shadowed_by(marker)) {
            _tomb = std::move(t);
        }
    }
    void apply(tombstone t) noexcept {
        _tomb.apply(t);
    }
    void apply(shadowable_tombstone t) noexcept {
        _tomb.apply(t._tomb);
    }
    friend std::ostream& operator<<(std::ostream& out, const shadowable_tombstone& t) {
        if (t) {
            return out << "{shadowable tombstone: timestamp=" << t.tomb().timestamp
                   << ", deletion_time=" << t.tomb().deletion_time.time_since_epoch().count()
                   << "}";
        } else {
            return out << "{shadowable tombstone: none}";
        }
    }
};
class row_tombstone : public with_relational_operators<row_tombstone> {
    tombstone _regular;
    shadowable_tombstone _shadowable; 
public:
    explicit row_tombstone(tombstone regular, shadowable_tombstone shadowable)
            : _regular(std::move(regular))
            , _shadowable(std::move(shadowable)) {
    }
    explicit row_tombstone(tombstone regular)
            : row_tombstone(regular, shadowable_tombstone(regular)) {
    }
    row_tombstone() = default;
    int compare(const row_tombstone& t) const {
        return _shadowable.compare(t._shadowable);
    }
    explicit operator bool() const {
        return bool(_shadowable);
    }
    const tombstone& tomb() const {
        return _shadowable.tomb();
    }
    const gc_clock::time_point max_deletion_time() const {
        return std::max(_regular.deletion_time, _shadowable.tomb().deletion_time);
    }
    const tombstone& regular() const {
        return _regular;
    }
    const shadowable_tombstone& shadowable() const {
        return _shadowable;
    }
    bool is_shadowable() const {
        return _shadowable.tomb() > _regular;
    }
    void maybe_shadow(const row_marker& marker) noexcept {
        _shadowable.maybe_shadow(_regular, marker);
    }
    void apply(tombstone regular) noexcept {
        _shadowable.apply(regular);
        _regular.apply(regular);
    }
    void apply(shadowable_tombstone shadowable, row_marker marker) noexcept {
        _shadowable.apply(shadowable.tomb());
        _shadowable.maybe_shadow(_regular, marker);
    }
    void apply(row_tombstone t, row_marker marker) noexcept {
        _regular.apply(t._regular);
        _shadowable.apply(t._shadowable);
        _shadowable.maybe_shadow(_regular, marker);
    }
    friend std::ostream& operator<<(std::ostream& out, const row_tombstone& t) {
        if (t) {
            return out << "{row_tombstone: " << t._regular << (t.is_shadowable() ? t._shadowable : shadowable_tombstone()) << "}";
        } else {
            return out << "{row_tombstone: none}";
        }
    }
};
class deletable_row final {
    row_tombstone _deleted_at;
    row_marker _marker;
    row _cells;
public:
    deletable_row() {}
    explicit deletable_row(clustering_row&&);
    deletable_row(const schema& s, const deletable_row& other)
        : _deleted_at(other._deleted_at)
        , _marker(other._marker)
        , _cells(s, column_kind::regular_column, other._cells)
    { }
    deletable_row(const schema& s, row_tombstone tomb, const row_marker& marker, const row& cells)
        : _deleted_at(tomb), _marker(marker), _cells(s, column_kind::regular_column, cells)
    {}
    void apply(const schema&, clustering_row);
    void apply(tombstone deleted_at) {
        _deleted_at.apply(deleted_at);
    }
    void apply(shadowable_tombstone deleted_at) {
        _deleted_at.apply(deleted_at, _marker);
    }
    void apply(row_tombstone deleted_at) {
        _deleted_at.apply(deleted_at, _marker);
    }
    void apply(const row_marker& rm) {
        _marker.apply(rm);
        _deleted_at.maybe_shadow(_marker);
    }
    void remove_tombstone() {
        _deleted_at = {};
    }
    void apply(const schema& s, deletable_row&& src);
    void apply_monotonically(const schema& s, deletable_row&& src);
public:
    row_tombstone deleted_at() const { return _deleted_at; }
    api::timestamp_type created_at() const { return _marker.timestamp(); }
    row_marker& marker() { return _marker; }
    const row_marker& marker() const { return _marker; }
    const row& cells() const { return _cells; }
    row& cells() { return _cells; }
    bool equal(column_kind, const schema& s, const deletable_row& other, const schema& other_schema) const;
    bool is_live(const schema& s, tombstone base_tombstone = tombstone(), gc_clock::time_point query_time = gc_clock::time_point::min()) const;
    bool empty() const { return !_deleted_at && _marker.is_missing() && !_cells.size(); }
    deletable_row difference(const schema&, column_kind, const deletable_row& other) const;
    class printer {
        const schema& _schema;
        const deletable_row& _deletable_row;
    public:
        printer(const schema& s, const deletable_row& r) : _schema(s), _deletable_row(r) { }
        printer(const printer&) = delete;
        printer(printer&&) = delete;
        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
};
class cache_tracker;
class rows_entry {
    friend class cache_tracker;
    friend class size_calculator;
    struct flags {};
    friend class mutation_partition;
public:
    struct last_dummy_tag {};
    explicit rows_entry(clustering_key&& key);
    explicit rows_entry(const clustering_key& key);
    rows_entry(const schema& s, position_in_partition_view pos, is_dummy dummy, is_continuous continuous);
    rows_entry(const schema& s, last_dummy_tag, is_continuous continuous);
    rows_entry(const clustering_key& key, deletable_row&& row);
    rows_entry(const schema& s, const clustering_key& key, const deletable_row& row);
    rows_entry(const schema& s, const clustering_key& key, row_tombstone tomb, const row_marker& marker, const row& row);
    rows_entry(rows_entry&& o) noexcept;
    rows_entry(const schema& s, const rows_entry& e);
    clustering_key& key();
    const clustering_key& key() const;
    deletable_row& row();
    const deletable_row& row() const;
    position_in_partition_view position() const;
    is_continuous continuous() const;
    void set_continuous(bool value);
    void set_continuous(is_continuous value);
    is_dummy dummy() const;
    bool is_last_dummy() const;
    void set_dummy(bool value);
    void set_dummy(is_dummy value);
    void apply(row_tombstone t);
    void apply_monotonically(const schema& s, rows_entry&& e);
    bool empty() const;
    struct tri_compare {
        explicit tri_compare(const schema& s);
        int operator()(const rows_entry& e1, const rows_entry& e2) const;
        int operator()(const clustering_key& key, const rows_entry& e) const;
        int operator()(const rows_entry& e, const clustering_key& key) const;
        int operator()(const rows_entry& e, position_in_partition_view p) const;
        int operator()(position_in_partition_view p, const rows_entry& e) const;
        int operator()(position_in_partition_view p1, position_in_partition_view p2) const;
    };
    struct compare {
        explicit compare(const schema& s);
        bool operator()(const rows_entry& e1, const rows_entry& e2) const;
        bool operator()(const clustering_key& key, const rows_entry& e) const;
        bool operator()(const rows_entry& e, const clustering_key& key) const;
        bool operator()(const clustering_key_view& key, const rows_entry& e) const;
        bool operator()(const rows_entry& e, const clustering_key_view& key) const;
        bool operator()(const rows_entry& e, position_in_partition_view p) const;
        bool operator()(position_in_partition_view p, const rows_entry& e) const;
        bool operator()(position_in_partition_view p1, position_in_partition_view p2) const;
    };
    bool equal(const schema& s, const rows_entry& other) const;
    bool equal(const schema& s, const rows_entry& other, const schema& other_schema) const;
    size_t memory_usage(const schema&) const;
    void on_evicted(cache_tracker&) noexcept;
    class printer {
    public:
        printer(const schema& s, const rows_entry& r);
        printer(const printer&) = delete;
        printer(printer&&) = delete;
        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
};
struct mutation_application_stats {
};
class mutation_partition final {
public:
    friend class rows_entry;
    friend class size_calculator;
    friend class converting_mutation_partition_applier;
public:
    struct copy_comparators_only {};
    struct incomplete_tag {};
    mutation_partition(incomplete_tag, const schema& s, tombstone);
    static mutation_partition make_incomplete(const schema& s, tombstone t = {});
    mutation_partition(schema_ptr s);
    mutation_partition(mutation_partition& other, copy_comparators_only);
    mutation_partition(mutation_partition&&) = default;
    mutation_partition(const schema& s, const mutation_partition&);
    mutation_partition(const mutation_partition&, const schema&, query::clustering_key_filter_ranges);
    mutation_partition(mutation_partition&&, const schema&, query::clustering_key_filter_ranges);
    ~mutation_partition();
    mutation_partition& operator=(mutation_partition&& x) noexcept;
    bool equal(const schema&, const mutation_partition&) const;
    bool equal(const schema& this_schema, const mutation_partition& p, const schema& p_schema) const;
    bool equal_continuity(const schema&, const mutation_partition&) const;
    template<typename Hasher>
    void feed_hash(Hasher& h, const schema& s) const;
    class printer {
    public:
        printer(const schema& s, const mutation_partition& mp);
        printer(const printer&) = delete;
        printer(printer&&) = delete;
        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
public:
    void ensure_last_dummy(const schema&);
    bool static_row_continuous() const;
    void set_static_row_continuous(bool value);
    bool is_fully_continuous() const;
    void make_fully_continuous();
    void set_continuity(const schema&, const position_range& pr, is_continuous);
    clustering_interval_set get_continuity(const schema&, is_continuous = is_continuous::yes) const;
    bool fully_continuous(const schema&, const position_range&);
    bool fully_discontinuous(const schema&, const position_range&);
    bool check_continuity(const schema&, const position_range&, is_continuous) const;
    stop_iteration clear_gently(cache_tracker*) noexcept;
    void apply(const schema& s, const mutation_fragment&);
    void apply(tombstone t);
    void apply_delete(const schema& schema, const clustering_key_prefix& prefix, tombstone t);
    void apply_delete(const schema& schema, range_tombstone rt);
    void apply_delete(const schema& schema, clustering_key_prefix&& prefix, tombstone t);
    void apply_delete(const schema& schema, clustering_key_prefix_view prefix, tombstone t);
    void apply_insert(const schema& s, clustering_key_view, api::timestamp_type created_at);
    void apply_insert(const schema& s, clustering_key_view, api::timestamp_type created_at,
                      gc_clock::duration ttl, gc_clock::time_point expiry);
    void apply_row_tombstone(const schema& schema, clustering_key_prefix prefix, tombstone t);
    void apply_row_tombstone(const schema& schema, range_tombstone rt);
    void apply(const schema& this_schema, const mutation_partition& p, const schema& p_schema,
            mutation_application_stats& app_stats);
    void apply(const schema& s, mutation_partition&& p, mutation_application_stats& app_stats);
    void apply(const schema& this_schema, mutation_partition_view p, const schema& p_schema,
            mutation_application_stats& app_stats);
    stop_iteration apply_monotonically(const schema& s, mutation_partition&& p, cache_tracker*,
            mutation_application_stats& app_stats, is_preemptible = is_preemptible::no);
    stop_iteration apply_monotonically(const schema& s, mutation_partition&& p, const schema& p_schema,
            mutation_application_stats& app_stats, is_preemptible = is_preemptible::no);
    void apply_weak(const schema& s, const mutation_partition& p, const schema& p_schema,
            mutation_application_stats& app_stats);
    void apply_weak(const schema& s, mutation_partition&&,
            mutation_application_stats& app_stats);
    void apply_weak(const schema& s, mutation_partition_view p, const schema& p_schema,
            mutation_application_stats& app_stats);
    void upgrade(const schema& old_schema, const schema& new_schema);
private:
    void insert_row(const schema& s, const clustering_key& key, deletable_row&& row);
    void insert_row(const schema& s, const clustering_key& key, const deletable_row& row);
    uint32_t do_compact(const schema& s,
        gc_clock::time_point now,
        const std::vector<query::clustering_range>& row_ranges,
        bool always_return_static_content,
        bool reverse,
        uint32_t row_limit,
        can_gc_fn&);
    template<bool reversed, typename Func>
    void trim_rows(const schema& s,
        const std::vector<query::clustering_range>& row_ranges,
        Func&& func);
public:
    uint32_t compact_for_query(const schema& s, gc_clock::time_point query_time,
        const std::vector<query::clustering_range>& row_ranges, bool always_return_static_content,
        bool reversed, uint32_t row_limit);
    void compact_for_compaction(const schema& s, can_gc_fn&,
        gc_clock::time_point compaction_time);
    mutation_partition difference(schema_ptr s, const mutation_partition& other) const;
    mutation_partition sliced(const schema& s, const query::clustering_row_ranges&) const;
    bool empty() const;
public:
    deletable_row& clustered_row(const schema& s, const clustering_key& key);
    deletable_row& clustered_row(const schema& s, clustering_key&& key);
    deletable_row& clustered_row(const schema& s, clustering_key_view key);
    deletable_row& clustered_row(const schema& s, position_in_partition_view pos, is_dummy, is_continuous);
public:
    tombstone partition_tombstone() const;
    lazy_row& static_row();
    const lazy_row& static_row() const;
    const range_tombstone_list& row_tombstones() const;
    range_tombstone_list& row_tombstones();
    const row* find_row(const schema& s, const clustering_key& key) const;
    tombstone range_tombstone_for_row(const schema& schema, const clustering_key& key) const;
    row_tombstone tombstone_for_row(const schema& schema, const clustering_key& key) const;
    row_tombstone tombstone_for_row(const schema& schema, const rows_entry& e) const;
    void query_compacted(query::result::partition_writer& pw, const schema& s, uint32_t row_limit) const;
    void accept(const schema&, mutation_partition_visitor&) const;
    size_t live_row_count(const schema&,
        gc_clock::time_point query_time = gc_clock::time_point::min()) const;
    bool is_static_row_live(const schema&,
        gc_clock::time_point query_time = gc_clock::time_point::min()) const;
    size_t row_count() const;
    size_t external_memory_usage(const schema&) const;
private:
    template<typename Func>
    void for_each_row(const schema& schema, const query::clustering_range& row_range, bool reversed, Func&& func) const;
    friend class counter_write_query_result_builder;
    void check_schema(const schema& s) const;
};
namespace db {
using timeout_clock = seastar::lowres_clock;
using timeout_semaphore = seastar::basic_semaphore<seastar::default_timeout_exception_factory, timeout_clock>;
using timeout_semaphore_units = seastar::semaphore_units<seastar::default_timeout_exception_factory, timeout_clock>;
static constexpr timeout_clock::time_point no_timeout = timeout_clock::time_point::max();
}
class clustering_row {
public:
    explicit clustering_row(clustering_key_prefix ck);
    clustering_row(clustering_key_prefix ck, row_tombstone t, row_marker marker, row cells);
    clustering_row(const schema& s, const clustering_row& other);
    clustering_row(const schema& s, const rows_entry& re);
    clustering_row(rows_entry&& re);
    clustering_key_prefix& key();
    const clustering_key_prefix& key() const;
    void remove_tombstone();
    row_tombstone tomb() const;
    const row_marker& marker() const;
    row_marker& marker();
    const row& cells() const;
    row& cells();
    bool empty() const;
    bool is_live(const schema& s, tombstone base_tombstone = tombstone(), gc_clock::time_point now = gc_clock::time_point::min()) const;
    void apply(const schema& s, clustering_row&& cr);
    void apply(const schema& s, const clustering_row& cr);
    void set_cell(const column_definition& def, atomic_cell_or_collection&& value);
    void apply(row_marker rm);
    void apply(tombstone t);
    void apply(shadowable_tombstone t);
    void apply(const schema& s, const rows_entry& r);
    position_in_partition_view position() const;
    size_t external_memory_usage(const schema& s) const;
    size_t memory_usage(const schema& s) const;
    bool equal(const schema& s, const clustering_row& other) const;
    class printer {
    public:
        printer(const schema& s, const clustering_row& r);
        printer(const printer&) = delete;
        printer(printer&&) = delete;
        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
};
class static_row {
    row _cells;
public:
    static_row() = default;
    static_row(const schema& s, const static_row& other) : static_row(s, other._cells) { }
    explicit static_row(const schema& s, const row& r) : _cells(s, column_kind::static_column, r) { }
    explicit static_row(row&& r) : _cells(std::move(r)) { }
    row& cells() { return _cells; }
    const row& cells() const { return _cells; }
    bool empty() const {
        return _cells.empty();
    }
    bool is_live(const schema& s, gc_clock::time_point now = gc_clock::time_point::min()) const {
        return _cells.is_live(s, column_kind::static_column, tombstone(), now);
    }
    void apply(const schema& s, const row& r) {
        _cells.apply(s, column_kind::static_column, r);
    }
    void apply(const schema& s, static_row&& sr) {
        _cells.apply(s, column_kind::static_column, std::move(sr._cells));
    }
    void set_cell(const column_definition& def, atomic_cell_or_collection&& value) {
        _cells.apply(def, std::move(value));
    }
    position_in_partition_view position() const;
    size_t external_memory_usage(const schema& s) const {
        return _cells.external_memory_usage(s, column_kind::static_column);
    }
    size_t memory_usage(const schema& s) const {
        return sizeof(static_row) + external_memory_usage(s);
    }
    bool equal(const schema& s, const static_row& other) const {
        return _cells.equal(column_kind::static_column, s, other._cells, s);
    }
    class printer {
        const schema& _schema;
        const static_row& _static_row;
    public:
        printer(const schema& s, const static_row& r) : _schema(s), _static_row(r) { }
        printer(const printer&) = delete;
        printer(printer&&) = delete;
        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
};
class partition_start final {
    dht::decorated_key _key;
    tombstone _partition_tombstone;
public:
    partition_start(dht::decorated_key pk, tombstone pt)
        : _key(std::move(pk))
        , _partition_tombstone(std::move(pt))
    { }
    dht::decorated_key& key() { return _key; }
    const dht::decorated_key& key() const { return _key; }
    const tombstone& partition_tombstone() const { return _partition_tombstone; }
    tombstone& partition_tombstone() { return _partition_tombstone; }
    position_in_partition_view position() const;
    size_t external_memory_usage(const schema&) const {
        return _key.external_memory_usage();
    }
    size_t memory_usage(const schema& s) const {
        return sizeof(partition_start) + external_memory_usage(s);
    }
    bool equal(const schema& s, const partition_start& other) const {
        return _key.equal(s, other._key) && _partition_tombstone == other._partition_tombstone;
    }
    friend std::ostream& operator<<(std::ostream& is, const partition_start& row);
};
class partition_end final {
public:
    position_in_partition_view position() const;
    size_t external_memory_usage(const schema&) const {
        return 0;
    }
    size_t memory_usage(const schema& s) const {
        return sizeof(partition_end) + external_memory_usage(s);
    }
    bool equal(const schema& s, const partition_end& other) const {
        return true;
    }
    friend std::ostream& operator<<(std::ostream& is, const partition_end& row);
};
GCC6_CONCEPT(
template<typename T, typename ReturnType>
concept bool MutationFragmentConsumer() {
    return requires(T t, static_row sr, clustering_row cr, range_tombstone rt, partition_start ph, partition_end pe) {
        { t.consume(std::move(sr)) } -> ReturnType;
        { t.consume(std::move(cr)) } -> ReturnType;
        { t.consume(std::move(rt)) } -> ReturnType;
        { t.consume(std::move(ph)) } -> ReturnType;
        { t.consume(std::move(pe)) } -> ReturnType;
    };
}
)
GCC6_CONCEPT(
template<typename T, typename ReturnType>
concept bool FragmentConsumerReturning() {
    return requires(T t, static_row sr, clustering_row cr, range_tombstone rt, tombstone tomb) {
        { t.consume(std::move(sr)) } -> ReturnType;
        { t.consume(std::move(cr)) } -> ReturnType;
        { t.consume(std::move(rt)) } -> ReturnType;
    };
}
)
GCC6_CONCEPT(
template<typename T>
concept bool FragmentConsumer() {
    return FragmentConsumerReturning<T, stop_iteration >() || FragmentConsumerReturning<T, future<stop_iteration>>();
}
)
GCC6_CONCEPT(
template<typename T>
concept bool StreamedMutationConsumer() {
    return FragmentConsumer<T>() && requires(T t, static_row sr, clustering_row cr, range_tombstone rt, tombstone tomb) {
        t.consume(tomb);
        t.consume_end_of_stream();
    };
}
)
GCC6_CONCEPT(
template<typename T, typename ReturnType>
concept bool MutationFragmentVisitor() {
    return requires(T t, const static_row& sr, const clustering_row& cr, const range_tombstone& rt, const partition_start& ph, const partition_end& eop) {
        { t(sr) } -> ReturnType;
        { t(cr) } -> ReturnType;
        { t(rt) } -> ReturnType;
        { t(ph) } -> ReturnType;
        { t(eop) } -> ReturnType;
    };
}
)
class mutation_fragment {
public:
    enum class kind {
        static_row,
        clustering_row,
        range_tombstone,
        partition_start,
        partition_end,
    };
private:
    struct data {
        data() { }
        ~data() { }
        std::optional<size_t> _size_in_bytes;
        union {
            static_row _static_row;
            clustering_row _clustering_row;
            range_tombstone _range_tombstone;
            partition_start _partition_start;
            partition_end _partition_end;
        };
    };
private:
    kind _kind;
    std::unique_ptr<data> _data;
    mutation_fragment() = default;
    explicit operator bool() const noexcept { return bool(_data); }
    void destroy_data() noexcept;
    friend class optimized_optional<mutation_fragment>;
    friend class position_in_partition;
public:
    struct clustering_row_tag_t { };
    template<typename... Args>
    mutation_fragment(clustering_row_tag_t, Args&&... args)
        : _kind(kind::clustering_row)
        , _data(std::make_unique<data>())
    {
        new (&_data->_clustering_row) clustering_row(std::forward<Args>(args)...);
    }
    mutation_fragment(static_row&& r);
    mutation_fragment(clustering_row&& r);
    mutation_fragment(range_tombstone&& r);
    mutation_fragment(partition_start&& r);
    mutation_fragment(partition_end&& r);
    mutation_fragment(const schema& s, const mutation_fragment& o)
        : _kind(o._kind), _data(std::make_unique<data>()) {
        switch(_kind) {
            case kind::static_row:
                new (&_data->_static_row) static_row(s, o._data->_static_row);
                break;
            case kind::clustering_row:
                new (&_data->_clustering_row) clustering_row(s, o._data->_clustering_row);
                break;
            case kind::range_tombstone:
                new (&_data->_range_tombstone) range_tombstone(o._data->_range_tombstone);
                break;
            case kind::partition_start:
                new (&_data->_partition_start) partition_start(o._data->_partition_start);
                break;
            case kind::partition_end:
                new (&_data->_partition_end) partition_end(o._data->_partition_end);
                break;
        }
    }
    mutation_fragment(mutation_fragment&& other) = default;
    mutation_fragment& operator=(mutation_fragment&& other) noexcept {
        if (this != &other) {
            this->~mutation_fragment();
            new (this) mutation_fragment(std::move(other));
        }
        return *this;
    }
    [[gnu::always_inline]]
    ~mutation_fragment() {
        if (_data) {
            destroy_data();
        }
    }
    position_in_partition_view position() const;
    position_range range() const;
    bool relevant_for_range(const schema& s, position_in_partition_view pos) const;
    bool relevant_for_range_assuming_after(const schema& s, position_in_partition_view pos) const;
    bool has_key() const { return is_clustering_row() || is_range_tombstone(); }
    const clustering_key_prefix& key() const;
    kind mutation_fragment_kind() const { return _kind; }
    bool is_static_row() const { return _kind == kind::static_row; }
    bool is_clustering_row() const { return _kind == kind::clustering_row; }
    bool is_range_tombstone() const { return _kind == kind::range_tombstone; }
    bool is_partition_start() const { return _kind == kind::partition_start; }
    bool is_end_of_partition() const { return _kind == kind::partition_end; }
    static_row& as_mutable_static_row() {
        _data->_size_in_bytes = std::nullopt;
        return _data->_static_row;
    }
    clustering_row& as_mutable_clustering_row() {
        _data->_size_in_bytes = std::nullopt;
        return _data->_clustering_row;
    }
    range_tombstone& as_mutable_range_tombstone() {
        _data->_size_in_bytes = std::nullopt;
        return _data->_range_tombstone;
    }
    partition_start& as_mutable_partition_start() {
        _data->_size_in_bytes = std::nullopt;
        return _data->_partition_start;
    }
    partition_end& as_mutable_end_of_partition();
    static_row&& as_static_row() &&;
    clustering_row&& as_clustering_row() &&;
    range_tombstone&& as_range_tombstone() &&;
    partition_start&& as_partition_start() &&;
    partition_end&& as_end_of_partition() &&;
    const static_row& as_static_row() const &;
    const clustering_row& as_clustering_row() const &;
    const range_tombstone& as_range_tombstone() const &;
    const partition_start& as_partition_start() const &;
    const partition_end& as_end_of_partition() const &;
    void apply(const schema& s, mutation_fragment&& mf);
    template<typename Consumer>
    GCC6_CONCEPT(
        requires MutationFragmentConsumer<Consumer, decltype(std::declval<Consumer>().consume(std::declval<range_tombstone>()))>()
    )
    decltype(auto) consume(Consumer& consumer) && {
        switch (_kind) {
        case kind::static_row:
            return consumer.consume(std::move(_data->_static_row));
        case kind::clustering_row:
            return consumer.consume(std::move(_data->_clustering_row));
        case kind::range_tombstone:
            return consumer.consume(std::move(_data->_range_tombstone));
        case kind::partition_start:
            return consumer.consume(std::move(_data->_partition_start));
        case kind::partition_end:
            return consumer.consume(std::move(_data->_partition_end));
        }
        abort();
    }
    template<typename Visitor>
    GCC6_CONCEPT(
        requires MutationFragmentVisitor<Visitor, decltype(std::declval<Visitor>()(std::declval<static_row&>()))>()
    )
    decltype(auto) visit(Visitor&& visitor) const {
        switch (_kind) {
        case kind::static_row:
            return visitor(as_static_row());
        case kind::clustering_row:
            return visitor(as_clustering_row());
        case kind::range_tombstone:
            return visitor(as_range_tombstone());
        case kind::partition_start:
            return visitor(as_partition_start());
        case kind::partition_end:
            return visitor(as_end_of_partition());
        }
        abort();
    }
    size_t memory_usage(const schema& s) const;
    bool equal(const schema& s, const mutation_fragment& other) const;
    bool mergeable_with(const mutation_fragment& mf) const;
    class printer {
        const schema& _schema;
        const mutation_fragment& _mutation_fragment;
    public:
        printer(const schema& s, const mutation_fragment& mf);
        printer(const printer&) = delete;
        printer(printer&&) = delete;
        friend std::ostream& operator<<(std::ostream& os, const printer& p);
    };
    friend std::ostream& operator<<(std::ostream& os, const printer& p);
};
std::ostream& operator<<(std::ostream&, partition_region);
std::ostream& operator<<(std::ostream&, mutation_fragment::kind);
using mutation_fragment_opt = optimized_optional<mutation_fragment>;
namespace streamed_mutation {
    class forwarding_tag;
    using forwarding = bool_class<forwarding_tag>;
}
class range_tombstone_stream {
    const schema& _schema;
    position_in_partition::less_compare _cmp;
    range_tombstone_list _list;
private:
    mutation_fragment_opt do_get_next();
public:
    range_tombstone_stream(const schema& s) : _schema(s), _cmp(s), _list(s) { }
    mutation_fragment_opt get_next(const rows_entry&);
    mutation_fragment_opt get_next(const mutation_fragment&);
    mutation_fragment_opt get_next(position_in_partition_view upper_bound);
    mutation_fragment_opt get_next();
    void forward_to(position_in_partition_view);
    void apply(range_tombstone&& rt) {
        _list.apply(_schema, std::move(rt));
    }
    void apply(const range_tombstone_list& list) {
        _list.apply(_schema, list);
    }
    void apply(const range_tombstone_list&, const query::clustering_range&, bool trim_front = false);
    void reset();
    bool empty() const;
    friend std::ostream& operator<<(std::ostream& out, const range_tombstone_stream&);
};
GCC6_CONCEPT(
    template<typename F>
    concept bool StreamedMutationTranformer() {
        return requires(F f, mutation_fragment mf, schema_ptr s) {
            { f(std::move(mf)) } -> mutation_fragment;
            { f(s) } -> schema_ptr;
        };
    }
)
class mutation final {
    mutation() = default;
    explicit operator bool();
    friend class optimized_optional<mutation>;
public:
    mutation(schema_ptr schema, dht::decorated_key key);
    mutation(schema_ptr schema, partition_key key_);
    mutation(schema_ptr schema, dht::decorated_key key, const mutation_partition& mp);
    mutation(schema_ptr schema, dht::decorated_key key, mutation_partition&& mp);
    mutation(const mutation& m);
    mutation(mutation&&) = default;
    mutation& operator=(mutation&& x) = default;
    mutation& operator=(const mutation& m);
    void set_static_cell(const column_definition& def, atomic_cell_or_collection&& value);
    void set_static_cell(const bytes& name, const data_value& value, api::timestamp_type timestamp, ttl_opt ttl = {});
    void set_clustered_cell(const clustering_key& key, const bytes& name, const data_value& value, api::timestamp_type timestamp, ttl_opt ttl = {});
    void set_clustered_cell(const clustering_key& key, const column_definition& def, atomic_cell_or_collection&& value);
    void set_cell(const clustering_key_prefix& prefix, const bytes& name, const data_value& value, api::timestamp_type timestamp, ttl_opt ttl = {});
    void set_cell(const clustering_key_prefix& prefix, const column_definition& def, atomic_cell_or_collection&& value);
    void upgrade(const schema_ptr&);
    const partition_key& key() const;
    const dht::decorated_key& decorated_key() const;
    dht::ring_position ring_position() const;
    const dht::token& token() const;
    const schema_ptr& schema() const;
    const mutation_partition& partition() const;
    mutation_partition& partition();
    const utils::UUID& column_family_id() const;
    bool operator==(const mutation&) const;
    bool operator!=(const mutation&) const;
public:
    query::result query(const query::partition_slice&,
        query::result_options opts = query::result_options::only_result(),
        gc_clock::time_point now = gc_clock::now(),
        uint32_t row_limit = query::max_rows) &&;
    query::result query(const query::partition_slice&,
        query::result_options opts = query::result_options::only_result(),
        gc_clock::time_point now = gc_clock::now(),
        uint32_t row_limit = query::max_rows) const&;
    void query(query::result::builder& builder,
        const query::partition_slice& slice,
        gc_clock::time_point now = gc_clock::now(),
        uint32_t row_limit = query::max_rows) &&;
    size_t live_row_count(gc_clock::time_point query_time = gc_clock::time_point::min()) const;
    void apply(mutation&&);
    void apply(const mutation&);
    void apply(const mutation_fragment&);
    mutation operator+(const mutation& other) const;
    mutation& operator+=(const mutation& other);
    mutation& operator+=(mutation&& other);
    mutation sliced(const query::clustering_row_ranges&) const;
private:
    friend std::ostream& operator<<(std::ostream& os, const mutation& m);
};
struct mutation_equals_by_key {
    bool operator()(const mutation& m1, const mutation& m2) const;
};
struct mutation_hash_by_key {
    size_t operator()(const mutation& m) const;
};
struct mutation_decorated_key_less_comparator {
    bool operator()(const mutation& m1, const mutation& m2) const;
};
using mutation_opt = optimized_optional<mutation>;
template<>
struct appending_hash<mutation> {
    template<typename Hasher>
    void operator()(Hasher& h, const mutation& m) const;
};
void apply(mutation_opt& dst, mutation&& src);
void apply(mutation_opt& dst, mutation_opt&& src);
boost::iterator_range<std::vector<mutation>::const_iterator> slice(
    const std::vector<mutation>& partitions,
    const dht::partition_range&);
class flat_mutation_reader;
future<mutation_opt> read_mutation_from_flat_mutation_reader(flat_mutation_reader& reader, db::timeout_clock::time_point timeout);
class cell_locker;
class cell_locker_stats;
class locked_cell;
class frozen_mutation;
class reconcilable_result;
namespace service {
class storage_proxy;
class migration_notifier;
class migration_manager;
}
namespace netw {
class messaging_service;
}
namespace gms {
class feature_service;
}
namespace sstables {
class sstable;
class entry_descriptor;
class compaction_descriptor;
class compaction_completion_desc;
class foreign_sstable_open_info;
class sstables_manager;
}
class compaction_manager;
namespace ser {
template<typename T>
class serializer;
}
namespace db {
class commitlog;
class config;
class extensions;
class rp_handle;
class data_listeners;
class large_data_handler;
}
class mutation_reordered_with_truncate_exception : public std::exception {};
using shared_memtable = lw_shared_ptr<memtable>;
class memtable_list;
class memtable_list {
};
class table;
using column_family = table;
struct table_stats;
using column_family_stats = table_stats;
class database_sstable_write_monitor;
class table {
public:
    future<std::vector<locked_cell>> lock_counter_cells(const mutation& m, db::timeout_clock::time_point timeout);
};
class user_types_metadata;
class keyspace_metadata final {
};
class keyspace {
public:
};
class database {
private:
    future<mutation> do_apply_counter_update(column_family& cf, const frozen_mutation& fm, schema_ptr m_schema, db::timeout_clock::time_point timeout,
                                             tracing::trace_state_ptr trace_state);
public:
};
namespace cql3{
class query_options;
struct raw_value_view;
namespace statements {
class prepared_statement;
}
}
namespace tracing {
class trace_state_ptr final {
public:
    trace_state_ptr();
    trace_state_ptr(nullptr_t);
};
}
using seastar::future;
class mutation_source;
GCC6_CONCEPT(
    template<typename Consumer>
    concept bool FlatMutationReaderConsumer() {
        return requires(Consumer c, mutation_fragment mf) {
            { c(std::move(mf)) } -> stop_iteration;
        };
    }
)
GCC6_CONCEPT(
    template<typename T>
    concept bool FlattenedConsumer() {
        return StreamedMutationConsumer<T>() && requires(T obj, const dht::decorated_key& dk) {
            obj.consume_new_partition(dk);
            obj.consume_end_of_partition();
        };
    }
    template<typename T>
    concept bool FlattenedConsumerFilter = requires(T filter, const dht::decorated_key& dk, const mutation_fragment& mf) {
        { filter(dk) } -> bool;
        { filter(mf) } -> bool;
        { filter.on_end_of_stream() } -> void;
    };
)
/*
 * Allows iteration on mutations using mutation_fragments.
 * It iterates over mutations one by one and for each mutation
 * it returns:
 *      1. partition_start mutation_fragment
 *      2. static_row mutation_fragment if one exists
 *      3. mutation_fragments for all clustering rows and range tombstones
 *         in clustering key order
 *      4. partition_end mutation_fragment
 * The best way to consume those mutation_fragments is to call
 * flat_mutation_reader::consume with a consumer that receives the fragments.
 */
class flat_mutation_reader final {
public:
    class impl {
    };
private:
    std::unique_ptr<impl> _impl;
    flat_mutation_reader() = default;
    explicit operator bool() const noexcept;
    friend class optimized_optional<flat_mutation_reader>;
    void do_upgrade_schema(const schema_ptr&);
public:
    class partition_range_forwarding_tag;
    using partition_range_forwarding = bool_class<partition_range_forwarding_tag>;
    flat_mutation_reader(std::unique_ptr<impl> impl) noexcept;
    future<mutation_fragment_opt> operator()(db::timeout_clock::time_point timeout);
    void next_partition();
    future<> fill_buffer(db::timeout_clock::time_point timeout);
    future<> fast_forward_to(const dht::partition_range& pr, db::timeout_clock::time_point timeout);
    future<> fast_forward_to(position_range cr, db::timeout_clock::time_point timeout);
    bool is_end_of_stream() const;
    bool is_buffer_empty() const;
    bool is_buffer_full() const;
    mutation_fragment pop_mutation_fragment();
    void unpop_mutation_fragment(mutation_fragment mf);
    const schema_ptr& schema() const;
    void set_max_buffer_size(size_t size);
    future<mutation_fragment*> peek(db::timeout_clock::time_point timeout);
    const mutation_fragment& peek_buffer() const;
    size_t buffer_size() const;
    circular_buffer<mutation_fragment> detach_buffer();
    void move_buffer_content_to(impl& other);
    void upgrade_schema(const schema_ptr& s);
};
using flat_mutation_reader_opt = optimized_optional<flat_mutation_reader>;
template<typename Impl, typename... Args>
flat_mutation_reader make_flat_mutation_reader(Args &&... args) {
    return flat_mutation_reader(std::make_unique<Impl>(std::forward<Args>(args)...));
}
template<typename T>
GCC6_CONCEPT(
    requires StreamedMutationTranformer<T>()
)
flat_mutation_reader transform(flat_mutation_reader r, T t);
inline flat_mutation_reader& to_reference(flat_mutation_reader& r) { return r; }
inline const flat_mutation_reader& to_reference(const flat_mutation_reader& r) { return r; }
flat_mutation_reader make_delegating_reader(flat_mutation_reader&);
flat_mutation_reader make_forwardable(flat_mutation_reader m);
flat_mutation_reader make_nonforwardable(flat_mutation_reader, bool);
flat_mutation_reader make_empty_flat_reader(schema_ptr s);
flat_mutation_reader flat_mutation_reader_from_mutations(std::vector<mutation>, const dht::partition_range& pr = query::full_partition_range, streamed_mutation::forwarding fwd = streamed_mutation::forwarding::no);
inline flat_mutation_reader flat_mutation_reader_from_mutations(std::vector<mutation> ms, streamed_mutation::forwarding fwd) {
    return flat_mutation_reader_from_mutations(std::move(ms), query::full_partition_range, fwd);
}
flat_mutation_reader
flat_mutation_reader_from_mutations(std::vector<mutation> ms,
                                    const query::partition_slice& slice,
                                    streamed_mutation::forwarding fwd = streamed_mutation::forwarding::no);
flat_mutation_reader
flat_mutation_reader_from_mutations(std::vector<mutation> ms,
                                    const dht::partition_range& pr,
                                    const query::partition_slice& slice,
                                    streamed_mutation::forwarding fwd = streamed_mutation::forwarding::no);
flat_mutation_reader
make_flat_multi_range_reader(schema_ptr s, mutation_source source, const dht::partition_range_vector& ranges,
                             const query::partition_slice& slice, const io_priority_class& pc = default_priority_class(),
                             tracing::trace_state_ptr trace_state = nullptr,
                             flat_mutation_reader::partition_range_forwarding fwd_mr = flat_mutation_reader::partition_range_forwarding::yes);
flat_mutation_reader
make_flat_multi_range_reader(
        schema_ptr s,
        mutation_source source,
        std::function<std::optional<dht::partition_range>()> generator,
        const query::partition_slice& slice,
        const io_priority_class& pc = default_priority_class(),
        tracing::trace_state_ptr trace_state = nullptr,
        flat_mutation_reader::partition_range_forwarding fwd_mr = flat_mutation_reader::partition_range_forwarding::yes);
flat_mutation_reader
make_flat_mutation_reader_from_fragments(schema_ptr, std::deque<mutation_fragment>);
flat_mutation_reader
make_flat_mutation_reader_from_fragments(schema_ptr, std::deque<mutation_fragment>, const dht::partition_range& pr);
flat_mutation_reader
make_flat_mutation_reader_from_fragments(schema_ptr, std::deque<mutation_fragment>, const dht::partition_range& pr, const query::partition_slice& slice);
template <typename Consumer>
inline
future<> consume_partitions(flat_mutation_reader& reader, Consumer consumer, db::timeout_clock::time_point timeout) {
    static_assert(std::is_same<future<stop_iteration>, futurize_t<std::result_of_t<Consumer(mutation&&)>>>::value, "bad Consumer signature");
    using futurator = futurize<std::result_of_t<Consumer(mutation&&)>>;
    return do_with(std::move(consumer), [&reader, timeout] (Consumer& c) -> future<> {
        return repeat([&reader, &c, timeout] () {
            return read_mutation_from_flat_mutation_reader(reader, timeout).then([&c] (mutation_opt&& mo) -> future<stop_iteration> {
                if (!mo) {
                    return make_ready_future<stop_iteration>(stop_iteration::yes);
                }
                return futurator::apply(c, std::move(*mo));
            });
        });
    });
}
flat_mutation_reader
make_generating_reader(schema_ptr s, std::function<future<mutation_fragment_opt> ()> get_next_fragment);
flat_mutation_reader
make_reversing_reader(flat_mutation_reader& original, size_t max_memory_consumption);
class mutation;
namespace ser {
class mutation_view;
}
class frozen_mutation final {
private:
    partition_key deserialize_key() const;
    ser::mutation_view mutation_view() const;
public:
    frozen_mutation(const mutation& m);
    explicit frozen_mutation(bytes_ostream&& b);
    frozen_mutation(bytes_ostream&& b, partition_key key);
    frozen_mutation(frozen_mutation&& m) = default;
    frozen_mutation(const frozen_mutation& m) = default;
    frozen_mutation& operator=(frozen_mutation&&) = default;
    frozen_mutation& operator=(const frozen_mutation&) = default;
    const bytes_ostream& representation() const;
    utils::UUID column_family_id() const;
    utils::UUID schema_version() const; 
    partition_key_view key(const schema& s) const;
    dht::decorated_key decorated_key(const schema& s) const;
    mutation_partition_view partition() const;
    mutation unfreeze(schema_ptr s) const;
    struct printer;
    printer pretty_printer(schema_ptr) const;
};
frozen_mutation freeze(const mutation& m);
struct frozen_mutation_and_schema {
    frozen_mutation fm;
    schema_ptr s;
};
class streamed_mutation_freezer;
static constexpr size_t default_frozen_fragment_size = 128 * 1024;
using frozen_mutation_consumer_fn = std::function<future<stop_iteration>(frozen_mutation, bool)>;
future<> fragment_and_freeze(flat_mutation_reader mr, frozen_mutation_consumer_fn c,
                             size_t fragment_size = default_frozen_fragment_size);
class frozen_mutation_fragment {
};
frozen_mutation_fragment freeze(const schema& s, const mutation_fragment& mf);
struct reader_resources {
    int count = 0;
    ssize_t memory = 0;
    reader_resources() = default;
    reader_resources(int count, ssize_t memory)
        : count(count)
        , memory(memory) {
    }
    bool operator>=(const reader_resources& other) const {
        return count >= other.count && memory >= other.memory;
    }
    reader_resources& operator-=(const reader_resources& other) {
        count -= other.count;
        memory -= other.memory;
        return *this;
    }
    reader_resources& operator+=(const reader_resources& other) {
        count += other.count;
        memory += other.memory;
        return *this;
    }
    explicit operator bool() const {
        return count >= 0 && memory >= 0;
    }
};
class reader_concurrency_semaphore;
class reader_permit {
    struct impl {
        reader_concurrency_semaphore& semaphore;
        reader_resources base_cost;
        impl(reader_concurrency_semaphore& semaphore, reader_resources base_cost);
        ~impl();
    };
    friend reader_permit no_reader_permit();
public:
    class memory_units {
        reader_concurrency_semaphore* _semaphore = nullptr;
        size_t _memory = 0;
        friend class reader_permit;
    private:
        memory_units(reader_concurrency_semaphore* semaphore, ssize_t memory) noexcept;
    public:
        memory_units(const memory_units&) = delete;
        memory_units(memory_units&&) noexcept;
        ~memory_units();
        memory_units& operator=(const memory_units&) = delete;
        memory_units& operator=(memory_units&&) noexcept;
        void reset(size_t memory = 0);
        operator size_t() const {
            return _memory;
        }
    };
private:
    lw_shared_ptr<impl> _impl;
private:
    reader_permit() = default;
public:
    reader_permit(reader_concurrency_semaphore& semaphore, reader_resources base_cost);
    bool operator==(const reader_permit& o) const {
        return _impl == o._impl;
    }
    operator bool() const {
        return bool(_impl);
    }
    memory_units get_memory_units(size_t memory = 0);
    void release();
};
reader_permit no_reader_permit();
template <typename Char>
temporary_buffer<Char> make_tracked_temporary_buffer(temporary_buffer<Char> buf, reader_permit& permit) {
    return temporary_buffer<Char>(buf.get_write(), buf.size(),
            make_deleter(buf.release(), [units = permit.get_memory_units(buf.size())] () mutable { units.reset(); }));
}
file make_tracked_file(file f, reader_permit p);
using namespace seastar;
class reader_concurrency_semaphore {
};
namespace mutation_reader {
    using forwarding = flat_mutation_reader::partition_range_forwarding;
}
enum class partition_presence_checker_result {
    definitely_doesnt_exist,
    maybe_exists
};
using partition_presence_checker = std::function<partition_presence_checker_result (const dht::decorated_key& key)>;
partition_presence_checker make_default_partition_presence_checker();
class mutation_source {
    using partition_range = const dht::partition_range&;
    using io_priority = const io_priority_class&;
    using flat_reader_factory_type = std::function<flat_mutation_reader(schema_ptr,
                                                                        reader_permit,
                                                                        partition_range,
                                                                        const query::partition_slice&,
                                                                        io_priority,
                                                                        tracing::trace_state_ptr,
                                                                        streamed_mutation::forwarding,
                                                                        mutation_reader::forwarding)>;
public:
    mutation_source() = default;
    explicit operator bool() const;
    friend class optimized_optional<mutation_source>;
public:
    mutation_source(flat_reader_factory_type fn, std::function<partition_presence_checker()> pcf = [] { return make_default_partition_presence_checker(); });
    mutation_source(std::function<flat_mutation_reader(schema_ptr, reader_permit, partition_range, const query::partition_slice&, io_priority,
                tracing::trace_state_ptr, streamed_mutation::forwarding)> fn);
    mutation_source(std::function<flat_mutation_reader(schema_ptr, reader_permit, partition_range, const query::partition_slice&, io_priority)> fn);
    mutation_source(std::function<flat_mutation_reader(schema_ptr, reader_permit, partition_range, const query::partition_slice&)> fn);
    mutation_source(std::function<flat_mutation_reader(schema_ptr, reader_permit, partition_range range)> fn);
    mutation_source(const mutation_source& other) = default;
    mutation_source& operator=(const mutation_source& other) = default;
    mutation_source(mutation_source&&) = default;
    mutation_source& operator=(mutation_source&&) = default;
    flat_mutation_reader
    make_reader(
        schema_ptr s,
        reader_permit permit,
        partition_range range,
        const query::partition_slice& slice,
        io_priority pc = default_priority_class(),
        tracing::trace_state_ptr trace_state = nullptr,
        streamed_mutation::forwarding fwd = streamed_mutation::forwarding::no,
        mutation_reader::forwarding fwd_mr = mutation_reader::forwarding::yes) const;
    flat_mutation_reader
    make_reader(
        schema_ptr s,
        reader_permit permit = no_reader_permit(),
        partition_range range = query::full_partition_range) const;
    partition_presence_checker make_partition_presence_checker();
};
using mutation_source_opt = optimized_optional<mutation_source>;
class reconcilable_result;
class frozen_reconcilable_result;
struct partition {
};
future<mutation_opt> counter_write_query(schema_ptr, const mutation_source&,
                                         const dht::decorated_key& dk,
                                         const query::partition_slice& slice,
                                         tracing::trace_state_ptr trace_ptr);
using namespace std::chrono_literals;
using namespace db;
class locked_cell {
};
future<mutation> database::do_apply_counter_update(column_family& cf, const frozen_mutation& fm, schema_ptr m_schema,
                                                   db::timeout_clock::time_point timeout,tracing::trace_state_ptr trace_state) {
    auto m = fm.unfreeze(m_schema);
    query::column_id_vector static_columns;
    query::clustering_row_ranges cr_ranges;
    query::column_id_vector regular_columns;
    auto slice = query::partition_slice(std::move(cr_ranges), std::move(static_columns),
        std::move(regular_columns), { }, { }, cql_serialization_format::internal(), query::max_rows);
    return do_with(std::move(slice), std::move(m), std::vector<locked_cell>(),
                   [this, &cf, timeout] (const query::partition_slice& slice, mutation& m, std::vector<locked_cell>& locks) mutable {
        return cf.lock_counter_cells(m, timeout).then([&, timeout, this] (std::vector<locked_cell> lcs) mutable {
            locks = std::move(lcs);
            return counter_write_query(schema_ptr(), mutation_source(), m.decorated_key(), slice, nullptr)
                    .then([this, &cf, &m, timeout] (auto mopt) {
                return std::move(m);
            });
        });
    });
}
