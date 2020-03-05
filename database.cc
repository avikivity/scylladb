/*
 * Copyright (C) 2014 ScyllaDB
 */

/*
 * This file is part of Scylla.
 *
 * Scylla is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Scylla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Scylla.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>

extern std::atomic<int64_t> clocks_offset;


#include <chrono>
#include <optional>
#include <seastar/core/byteorder.hh>
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


//
// This hashing differs from std::hash<> in that it decouples knowledge about
// type structure from the way the hash value is calculated:
//  * appending_hash<T> instantiation knows about what data should be included in the hash for type T.
//  * Hasher object knows how to combine the data into the final hash.
//
// The appending_hash<T> should always feed some data into the hasher, regardless of the state the object is in,
// in order for the hash to be highly sensitive for value changes. For example, vector<optional<T>> should
// ideally feed different values for empty vector and a vector with a single empty optional.
//
// appending_hash<T> is machine-independent.
//

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

#include <chrono>
#include <optional>

class gc_clock final {
public:
    using base = seastar::lowres_system_clock;
    using rep = int64_t;
    using period = std::ratio<1, 1>; // seconds
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<gc_clock, duration>;

    static constexpr auto is_steady = base::is_steady;

    static time_point now();

    static int32_t as_int32(duration d);
    static int32_t as_int32(time_point tp);
};

using expiry_opt = std::optional<gc_clock::time_point>;
using ttl_opt = std::optional<gc_clock::duration>;

// 20 years in seconds
static constexpr gc_clock::duration max_ttl = gc_clock::duration{20 * 365 * 24 * 60 * 60};

std::ostream& operator<<(std::ostream& os, gc_clock::time_point tp);




#include <chrono>
#include <cstdint>
#include <ratio>

// the database clock follows Java - 1ms granularity, 64-bit counter, 1970 epoch

class db_clock final {
public:
    using base = std::chrono::system_clock;
    using rep = int64_t;
    using period = std::ratio<1, 1000>; // milliseconds
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<db_clock, duration>;

    static constexpr bool is_steady = base::is_steady;
    static time_point now() {
        return time_point();
    }
};

gc_clock::time_point to_gc_clock(db_clock::time_point tp);
/* For debugging and log messages. */
std::ostream& operator<<(std::ostream&, db_clock::time_point);


#include <chrono>
#include <functional>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string.h>
#include <seastar/core/future.hh>
#include <limits>
#include <cstddef>

#include <seastar/core/shared_ptr.hh>

using column_count_type = uint32_t;

// Column ID, unique within column_kind
using column_id = column_count_type;

class schema;
class schema_extension;

using schema_ptr = seastar::lw_shared_ptr<const schema>;


#include <cstdint>
#include <limits>
#include <chrono>
#include <string>

namespace api {

using timestamp_type = int64_t;
timestamp_type constexpr missing_timestamp = std::numeric_limits<timestamp_type>::min();
timestamp_type constexpr min_timestamp = std::numeric_limits<timestamp_type>::min() + 1;
timestamp_type constexpr max_timestamp = std::numeric_limits<timestamp_type>::max();

// Used for generating server-side mutation timestamps.
// Same epoch as Java's System.currentTimeMillis() for compatibility.
// Satisfies requirements of Clock.
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


#include <functional>

#include <seastar/util/gcc6-concepts.hh>
#include <type_traits>

GCC6_CONCEPT(
template<typename T>
concept bool HasTriCompare =
    requires(const T& t) {
        { t.compare(t) } -> int;
    } && std::is_same<std::result_of_t<decltype(&T::compare)(T, T)>, int>::value; //FIXME: #1449
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

    // See reversibly_mergeable.hh
    void apply_reversibly(tombstone& t) noexcept;

    // See reversibly_mergeable.hh
    void revert(tombstone& t) noexcept;
    tombstone operator+(const tombstone& t);

    friend std::ostream& operator<<(std::ostream& out, const tombstone& t);
};

template<>
struct appending_hash<tombstone> {
    template<typename Hasher>
    void operator()(Hasher& h, const tombstone& t) const;
};

// Determines whether tombstone may be GC-ed.
using can_gc_fn = std::function<bool(tombstone)>;

static can_gc_fn always_gc = [] (tombstone) { return true; };

#include <iosfwd>

#include <seastar/core/bitset-iter.hh>
#include <seastar/util/optimized_optional.hh>

#include <seastar/core/sstring.hh>
#include <optional>
#include <iosfwd>
#include <functional>

#include <string_view>
#include <seastar/core/sstring.hh>

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

// Must be in std:: namespace, or ADL fails
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

#include <optional>
#include <iosfwd>

#include <seastar/core/sstring.hh>
#include <seastar/core/shared_ptr.hh>
#include <stdint.h>
#include <cassert>
#include <array>
#include <iosfwd>

#include <seastar/core/sstring.hh>
#include <seastar/core/print.hh>
#include <seastar/net/byteorder.hh>

#include <stdint.h>

#include <seastar/util/gcc6-concepts.hh>
#include <seastar/core/sstring.hh>
#include <seastar/net/byteorder.hh>
#include <iosfwd>
#include <iterator>


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

// The following serializer is compatible with Java's writeUTF().
// In our C++ implementation, we assume the string is already UTF-8
// encoded. Unfortunately, Java's implementation is a bit different from
// UTF-8 for encoding characters above 16 bits in unicode (see
// http://docs.oracle.com/javase/7/docs/api/java/io/DataInput.html#modified-utf-8)
// For now we'll just assume those aren't in the string...
// TODO: fix the compatibility with Java even in this case.
template <typename CharOutputIterator>
GCC6_CONCEPT(requires requires (CharOutputIterator it) {
    *it++ = 'a';
})
inline
void serialize_string(CharOutputIterator& out, const sstring& s) {
    // Java specifies that nulls in the string need to be replaced by the
    // two bytes 0xC0, 0x80. Let's not bother with such transformation
    // now, but just verify wasn't needed.
    for (char c : s) {
        if (c == '\0') {
            throw UTFDataFormatException();
        }
    }
    if (s.size() > std::numeric_limits<uint16_t>::max()) {
        // Java specifies the string length is written as uint16_t, so we
        // can't serialize longer strings.
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
    // TODO: like above, need to change UTF-8 when above 16-bit.
    auto len = strlen(s);
    if (len > std::numeric_limits<uint16_t>::max()) {
        // Java specifies the string length is written as uint16_t, so we
        // can't serialize longer strings.
        throw UTFDataFormatException();
    }
    serialize_int16(out, len);
    out = std::copy_n(s, len, out);
}

inline
size_t serialize_string_size(const sstring& s) {;
    // As above, this code is missing the case of modified utf-8
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
        //if (version() != 1) {
        //     throw new UnsupportedOperationException("Not a time-based UUID");
        //}
        assert(is_timestamp());

        return ((most_sig_bits & 0xFFF) << 48) |
               (((most_sig_bits >> 16) & 0xFFFF) << 32) |
               (((uint64_t)most_sig_bits) >> 32);

    }

    // This matches Java's UUID.toString() actual implementation. Note that
    // that method's documentation suggest something completely different!
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

#include <seastar/net/byteorder.hh>
#include <seastar/util/log.hh>

namespace logging {

//
// Seastar changed the names of some of these types. Maintain the old names here to avoid too much churn.
//

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

#include <seastar/net/byteorder.hh>
#include <cstdint>
#include <iosfwd>
#include <seastar/util/gcc6-concepts.hh>
#include <seastar/util/variant_utils.hh>

#include <array>
#include <type_traits>

#include <seastar/util/gcc6-concepts.hh>

namespace meta {

// Wrappers that allows returning a list of types. All helpers defined in this
// file accept both unpacked and packed lists of types.
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

// Returns the index of the first type in the list of types list of types Ts for
// which Predicate<T::value is true.
template<template<class> typename Predicate, typename... Ts>
constexpr size_t find_if = internal::do_find_if<Predicate, Ts...>::value;

// Returns the index of the first occurrence of type T in the list of types Ts.
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

// Returns the Nth type in the provided list of types.
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

// Returns a list containing N first elements of the provided list of types.
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

// Executes the provided function for each element in the provided list of
// types. For each type T the Function is called with an argument of type T*.
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

// Returns the size of a list of types.
template<typename... Ts>
constexpr size_t size = internal::get_size<Ts...>::value;

template<template <class> typename Predicate, typename... Ts>
static constexpr bool all_of = std::conjunction_v<Predicate<Ts>...>;

}

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/for_each.hpp>

#include <seastar/util/gcc6-concepts.hh>

enum class mutable_view { no, yes, };

GCC6_CONCEPT(

/// Fragmented buffer
///
/// Concept `FragmentedBuffer` is satisfied by any class that is a range of
/// fragments and provides a method `size_bytes()` which returns the total
/// size of the buffer. The interfaces accepting `FragmentedBuffer` will attempt
/// to avoid unnecessary linearisation.
template<typename T>
concept bool FragmentRange = requires (T range) {
    typename T::fragment_type;
    requires std::is_same_v<typename T::fragment_type, bytes_view>
        || std::is_same_v<typename T::fragment_type, bytes_mutable_view>;
    { *range.begin() } -> typename T::fragment_type;
    { *range.end() } -> typename T::fragment_type;
    { range.size_bytes() } -> size_t;
    { range.empty() } -> bool; // returns true iff size_bytes() == 0.
};

)

template<typename T, typename = void>
struct is_fragment_range : std::false_type { };

template<typename T>
struct is_fragment_range<T, std::void_t<typename T::fragment_type>> : std::true_type { };

template<typename T>
static constexpr bool is_fragment_range_v = is_fragment_range<T>::value;

/// A non-mutable view of a FragmentRange
///
/// Provide a trivially copyable and movable, non-mutable view on a
/// fragment range. This allows uniform ownership semantics across
/// multi-fragment ranges and the single fragment and empty fragment
/// adaptors below, i.e. it allows treating all fragment ranges
/// uniformly as views.
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


#include <vector>

#include <seastar/core/iostream.hh>
#include <seastar/core/print.hh>
#include <seastar/core/temporary_buffer.hh>
#include <seastar/core/simple-stream.hh>


#include <boost/range/iterator_range.hpp>

#include <seastar/core/unaligned.hh>
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
        // FIXME: group fragment pointers to reduce pointer chasing when packetizing
        std::unique_ptr<chunk> next;
        ~chunk() {
            auto p = std::move(next);
            while (p) {
                // Avoid recursion when freeing chunks
                auto p_next = std::move(p->next);
                p = std::move(p_next);
            }
        }
        size_type offset; // Also means "size" after chunk is closed
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
        bytes_view operator*() const {
            return { _current->data, _current->offset };
        }
        bytes_view operator->() const {
            return *(*this);
        }
        fragment_iterator& operator++() {
            _current = _current->next.get();
            return *this;
        }
        fragment_iterator operator++(int) {
            fragment_iterator tmp(*this);
            ++(*this);
            return tmp;
        }
        bool operator==(const fragment_iterator& other) const {
            return _current == other._current;
        }
        bool operator!=(const fragment_iterator& other) const {
            return _current != other._current;
        }
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
        explicit output_iterator(bytes_ostream& os) : _ostream(&os) { }

    public:
        reference operator*() const { return *_ostream->write_place_holder(1); }
        output_iterator& operator++() { return *this; }
        output_iterator operator++(int) { return *this; }
    };
private:
    inline size_type current_space_left() const {
        if (!_current) {
            return 0;
        }
        return _current->size - _current->offset;
    }
    // Figure out next chunk size.
    //   - must be enough for data_size
    //   - must be at least _initial_chunk_size
    //   - try to double each time to prevent too many allocations
    //   - do not exceed max_chunk_size
    size_type next_alloc_size(size_t data_size) const {
        auto next_size = _current
                ? _current->size * 2
                : _initial_chunk_size;
        next_size = std::min(next_size, max_chunk_size());
        // FIXME: check for overflow?
        return std::max<size_type>(next_size, data_size + sizeof(chunk));
    }
    // Makes room for a contiguous region of given size.
    // The region is accounted for as already written.
    // size must not be zero.
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
        // makes the place_holder looks like a stream
        seastar::simple_output_stream get_stream() {
            return seastar::simple_output_stream(reinterpret_cast<char*>(ptr), sizeof(T));
        }
    };

    // Returns a place holder for a value to be written later.
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

    // Writes given sequence of bytes
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

    // Call only when is_linearized()
    bytes_view view() const {
        assert(is_linearized());
        if (!_current) {
            return bytes_view();
        }

        return bytes_view(_current->data, _size);
    }

    // Makes the underlying storage contiguous and returns a view to it.
    // Invalidates all previously created placeholders.
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

    // Returns the amount of bytes written so far
    size_type size() const {
        return _size;
    }

    // For the FragmentRange concept
    size_type size_bytes() const {
        return _size;
    }

    bool empty() const {
        return _size == 0;
    }

    void reserve(size_t size) {
        // FIXME: implement
    }

    void append(const bytes_ostream& o) {
        for (auto&& bv : o.fragments()) {
            write(bv);
        }
    }

    // Removes n bytes from the end of the bytes_ostream.
    // Beware of O(n) algorithm.
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

    // begin() and end() form an input range to bytes_view representing fragments.
    // Any modification of this instance invalidates iterators.
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

    position pos() const {
        return { _current, _current ? _current->offset : 0 };
    }

    // Returns the amount of bytes written since given position.
    // "pos" must be valid.
    size_type written_since(position pos) {
        chunk* c = pos._chunk;
        if (!c) {
            return _size;
        }
        size_type total = c->offset - pos._offset;
        c = c->next.get();
        while (c) {
            total += c->offset;
            c = c->next.get();
        }
        return total;
    }

    // Rollbacks all data written after "pos".
    // Invalidates all placeholders and positions created after "pos".
    void retract(position pos) {
        if (!pos._chunk) {
            *this = {};
            return;
        }
        _size -= written_since(pos);
        _current = pos._chunk;
        _current->next = nullptr;
        _current->offset = pos._offset;
    }

    void reduce_chunk_count() {
        // FIXME: This is a simplified version. It linearizes the whole buffer
        // if its size is below max_chunk_size. We probably could also gain
        // some read performance by doing "real" reduction, i.e. merging
        // all chunks until all but the last one is max_chunk_size.
        if (size() < max_chunk_size()) {
            linearize();
        }
    }

    bool operator==(const bytes_ostream& other) const {
        auto as = fragments().begin();
        auto as_end = fragments().end();
        auto bs = other.fragments().begin();
        auto bs_end = other.fragments().end();

        auto a = *as++;
        auto b = *bs++;
        while (!a.empty() || !b.empty()) {
            auto now = std::min(a.size(), b.size());
            if (!std::equal(a.begin(), a.begin() + now, b.begin(), b.begin() + now)) {
                return false;
            }
            a.remove_prefix(now);
            if (a.empty() && as != as_end) {
                a = *as++;
            }
            b.remove_prefix(now);
            if (b.empty() && bs != bs_end) {
                b = *bs++;
            }
        }
        return true;
    }

    bool operator!=(const bytes_ostream& other) const {
        return !(*this == other);
    }

    // Makes this instance empty.
    //
    // The first buffer is not deallocated, so callers may rely on the
    // fact that if they write less than the initial chunk size between
    // the clear() calls then writes will not involve any memory allocations,
    // except for the first write made on this instance.
    void clear() {
        if (_begin) {
            _begin->offset = 0;
            _size = 0;
            _current = _begin.get();
            _begin->next.reset();
        }
    }
};


/// Fragmented buffer consisting of multiple temporary_buffer<char>
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

    // Linear complexity, invalidates views and istreams
    void remove_prefix(size_t n) noexcept;

    // Linear complexity, invalidates views and istreams
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

#include <vector>
#include <unordered_set>
#include <list>
#include <array>
#include <seastar/core/sstring.hh>
#include <unordered_map>
#include <optional>

namespace ser {

/// A fragmented view of an opaque buffer in a stream of serialised data
///
/// This class allows reading large, fragmented blobs serialised by the IDL
/// infrastructure without linearising or copying them. The view remains valid
/// as long as the underlying IDL-serialised buffer is alive.
///
/// Satisfies FragmentRange concept.
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

/// View of an atomic cell
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
    // Can be called on live and dead cells
    api::timestamp_type timestamp() const;
    void set_timestamp(api::timestamp_type ts);
    // Can be called on live cells only
    size_t value_size() const;
    bool is_value_fragmented() const;
    // Can be called on live counter update cells only
    int64_t counter_update_value() const;
    // Can be called only when is_dead(gc_clock::time_point)
    gc_clock::time_point deletion_time() const;
    // Can be called only when is_live_and_has_ttl()
    gc_clock::time_point expiry() const;
    // Can be called only when is_live_and_has_ttl()
    gc_clock::duration ttl() const;
    // Can be called on live and dead cells
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

#include <iosfwd>

using cql_protocol_version_type = uint8_t;

// Abstraction of transport protocol-dependent serialization format
// Protocols v1, v2 used 16 bits for collection sizes, while v3 and
// above use 32 bits.  But letting every bit of the code know what
// transport protocol we're using (and in some cases, we aren't using
// any transport -- it's for internal storage) is bad, so abstract it
// away here.

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

#include <seastar/core/sstring.hh>
#include <vector>
#include <sstream>
#include <unordered_set>
#include <set>
#include <optional>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>
#include <algorithm>
#include <initializer_list>
#include <memory>
#include <stdexcept>

namespace utils {

/// A vector with small buffer optimisation
///
/// small_vector is a variation of std::vector<> that reserves a configurable
/// amount of storage internally, without the need for memory allocation.
/// This can bring measurable gains if the expected number of elements is
/// small. The drawback is that moving such small_vector is more expensive
/// and invalidates iterators as well as references which disqualifies it in
/// some cases.
///
/// All member functions of small_vector provide strong exception guarantees.
///
/// It is unspecified when small_vector is going to use internal storage, except
/// for the obvious case when size() > N. In other situations user must not
/// attempt to guess if data is stored internally or externally. The same applies
/// to capacity(). Apart from the obvious fact that capacity() >= size() the user
/// must not assume anything else. In particular it may not always hold that
/// capacity() >= N.
///
/// Unless otherwise specified (e.g. move ctor and assignment) small_vector
/// provides guarantees at least as strong as those of std::vector<>.
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

    // Use union instead of std::aligned_storage so that debuggers can see
    // the contained objects without needing any pretty printers.
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

    // May invalidate iterators and references.
    small_vector(small_vector&& other) noexcept {
        if (other.uses_internal_storage()) {
            _begin = _internal.storage;
            _capacity_end = _begin + N;
            if constexpr (std::is_trivially_copyable_v<T>) {
                // Compilers really like loops with the number of iterations known at
                // the compile time, the usually emit less code which can be more aggressively
                // optimised. Since we can assume that N is small it is most likely better
                // to just copy everything, regardless of how many elements are actually in
                // the vector.
                std::memcpy(_internal.storage, other._internal.storage, N * sizeof(T));
                _end = _begin + other.size();
            } else {
                _end = _begin;

                // What we would really like here is std::uninintialized_move_and_destroy.
                // It is beneficial to do move and destruction in a single pass since the compiler
                // may be able to merge those operations (e.g. the destruction of a move-from
                // std::unique_ptr is a no-op).
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

    // May invalidate iterators and references.
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

                // Better to use single pass than std::uninitialize_move + std::destroy.
                // See comment in move ctor for details.
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

#include <boost/range/algorithm/equal.hpp>
#include <boost/algorithm/clamp.hpp>
#include <boost/version.hpp>
#include <memory>
#include <type_traits>
#include <iterator>
#include <utility>
#include <algorithm>
#include <stdexcept>

namespace utils {

struct chunked_vector_free_deleter {
    void operator()(void* x) const { ::free(x); }
};

template <typename T, size_t max_contiguous_allocation = 128*1024>
class chunked_vector {
    static_assert(std::is_nothrow_move_constructible<T>::value, "T must be nothrow move constructible");
    using chunk_ptr = std::unique_ptr<T[], chunked_vector_free_deleter>;
    // Each chunk holds max_chunk_capacity() items, except possibly the last
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
        iterator_type(const iterator_type<std::remove_const_t<ValueType>>& x) : _chunks(x._chunks), _i(x._i) {} // needed for iterator->const_iterator conversion
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

#include <seastar/core/sstring.hh>

#include <cstdint>
#include <string_view>
#include <ostream>
#include <stdexcept>

// Wrapper for a value with a type-tag for differentiating instances.
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

//
// A duration of time.
//
// Three counters represent the time: the number of months, of days, and of nanoseconds. This is necessary because
// the number hours in a day can vary during daylight savings and because the number of days in a month vary.
//
// As a consequence of this representation, there can exist no total ordering relation on durations. To see why,
// consider a duration `1mo5s` (1 month and 5 seconds). In a month with 30 days, this represents a smaller duration of
// time than in a month with 31 days.
//
// The primary use of this type is to manipulate absolute time-stamps with relative offsets. For example,
// `"Jan. 31 2005 at 23:15" + 3mo5d`.
//
class cql_duration final {
public:
    using common_counter_type = int64_t;

    static_assert(
            (sizeof(common_counter_type) >= sizeof(months_counter::value_type)) &&
            (sizeof(common_counter_type) >= sizeof(days_counter::value_type)) &&
            (sizeof(common_counter_type) >= sizeof(nanoseconds_counter::value_type)),
            "The common counter type is smaller than one of the component counter types.");

    // A zero-valued duration.
    constexpr cql_duration() noexcept = default;

    // Construct a duration with explicit values for its three counters.
    constexpr cql_duration(months_counter m, days_counter d, nanoseconds_counter n) noexcept :
            months(m),
            days(d),
            nanoseconds(n) {}

    //
    // Parse a duration string.
    //
    // Three formats for durations are supported:
    //
    // 1. "Standard" format. This consists of one or more pairs of a count and a unit specifier. Examples are "23d1mo"
    //    and "5h23m10s". Components of the total duration must be written in decreasing order. That is, "5h2y" is
    //    an invalid duration string.
    //
    //    The allowed units are:
    //      - "y": years
    //      - "mo": months
    //      - "w": weeks
    //      - "d": days
    //      - "h": hours
    //      - "m": minutes
    //      - "s": seconds
    //      - "ms": milliseconds
    //      - "us" or "µs": microseconds
    //      - "ns": nanoseconds
    //
    //    Units are case-insensitive.
    //
    // 2. ISO-8601 format. "P[n]Y[n]M[n]DT[n]H[n]M[n]S" or "P[n]W". All specifiers are optional. Examples are
    //    "P23Y1M" or "P10W".
    //
    // 3. ISO-8601 alternate format. "P[YYYY]-[MM]-[DD]T[hh]:[mm]:[ss]". All specifiers are mandatory. An example is
    //    "P2000-10-14T07:22:30".
    //
    // For all formats, a negative duration is indicated by beginning the string with the '-' symbol. For example,
    // "-2y10ns".
    //
    // Throws `cql_duration_error` in the event of a parsing error.
    //
    explicit cql_duration(std::string_view s);

    months_counter::value_type months{0};
    days_counter::value_type days{0};
    nanoseconds_counter::value_type nanoseconds{0};
};

//
// Pretty-print a duration using the standard format.
//
// Durations are simplified during printing so that `duration(24, 0, 0)` is printed as "2y".
//
std::ostream& operator<<(std::ostream& os, const cql_duration& d);

// See above.
seastar::sstring to_string(const cql_duration&);

//
// Note that equality comparison is based on exact counter matches. It is not valid to expect equivalency across
// counters like months and days. See the documentation for `duration` for more.
//

bool operator==(const cql_duration&, const cql_duration&) noexcept;
bool operator!=(const cql_duration&, const cql_duration&) noexcept;

#include <stdexcept>

class marshal_exception : public std::exception {
    sstring _why;
public:
    marshal_exception() = delete;
    marshal_exception(sstring why) : _why(sstring("marshaling error: ") + why) {}
    virtual const char* what() const noexcept override { return _why.c_str(); }
};

#include <seastar/net/ip.hh>
#include <seastar/net/inet_address.hh>
#include <seastar/util/backtrace.hh>

#include <seastar/core/sstring.hh>
#include <seastar/core/on_internal_error.hh>

#include <functional>
#include <system_error>

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

// Specifies position in a lexicographically ordered sequence
// relative to some value.
//
// For example, if used with a value "bc" with lexicographical ordering on strings,
// each enum value represents the following positions in an example sequence:
//
//   aa
//   aaa
//   b
//   ba
// --> before_all_prefixed
//   bc
// --> before_all_strictly_prefixed
//   bca
//   bcd
// --> after_all_prefixed
//   bd
//   bda
//   c
//   ca
//
enum class lexicographical_relation : int8_t {
    before_all_prefixed,
    before_all_strictly_prefixed,
    after_all_prefixed
};

// A trichotomic comparator for prefix equality total ordering.
// In this ordering, two sequences are equal iff any of them is a prefix
// of the another. Otherwise, lexicographical ordering determines the order.
//
// 'comp' is an abstract_type-aware trichotomic comparator, which takes the
// type as first argument.
//
template <typename TypesIterator, typename InputIt1, typename InputIt2, typename Compare>
int prefix_equality_tri_compare(TypesIterator types, InputIt1 first1, InputIt1 last1,
        InputIt2 first2, InputIt2 last2, Compare comp);

// Returns true iff the second sequence is a prefix of the first sequence
// Equality is an abstract_type-aware equality checker which takes the type as first argument.
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

// Cassandra has a notion of empty values even for scalars (i.e. int).  This is
// distinct from NULL which means deleted or never set.  It is serialized
// as a zero-length byte array (whereas NULL is serialized as a negative-length
// byte array).
template <typename T>
class emptyable {
    // We don't use optional<>, to avoid lots of ifs during the copy and move constructors
    static_assert(std::is_default_constructible<T>::value, "must be default constructible");
public:
    // default-constructor defaults to a non-empty value, since empty is the
    // exception rather than the rule
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

// Checks whether T::empty() const exists and returns bool
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
    void* _value;  // FIXME: use "small value optimization" for small types
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
    // common conversions from C++ types to database types
    // note: somewhat dangerous, consider a factory function instead
    explicit data_value(bytes);

    data_value(sstring&&);
    data_value(std::string_view);
    // We need the following overloads just to avoid ambiguity because
    // seastar::net::inet_address is implicitly constructible from a
    // const sstring&.
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
    bool is_null() const {   // may return false negatives for strings etc.
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

// Unsafe to access across shards unless otherwise noted.
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
    // returns a callable that can be called with two byte_views, and calls this->less() on them.
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

    // For types that contain (or are equal to) the given user type (e.g., a set of elements of this type),
    // updates them with the new version of the type ('updated'). For other types does nothing.
    std::optional<data_type> update_user_type(const shared_ptr<const user_type_impl> updated) const;

    bool references_duration() const;
    std::optional<uint32_t> value_length_if_fixed() const;
public:
    bytes decompose(const data_value& value) const;
    // Safe to call across shards
    const sstring& name() const;

    /**
     * When returns true then equal values have the same byte representation and if byte
     * representation is different, the values are not equal.
     *
     * When returns false, nothing can be inferred.
     */
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
    // Lists and sets are similar: they are both represented as std::vector<data_value>
    // @sa listlike_collection_type_impl
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
    // native_value_* methods are virualized versions of native_type's
    // sizeof/alignof/copy-ctor/move-ctor etc.
    void* native_value_clone(const void* from) const;
    const std::type_info& native_typeid() const;
    // abstract_type is a friend of data_value, but derived classes are not.
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

// CRTP: implements translation between a native_type (C++ type) to abstract_type
// AbstractType is parametrized because we want a
//    abstract_type -> collection_type_impl -> map_type
// type hierarchy, and native_type is only known at the last step.
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

// Remember to update type_codec in transport/server.cc and cql3/cql3_type.cc
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

// FIXME: make more explicit
bytes
to_bytes(const char* x);

// FIXME: make more explicit
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

// FIXME: make more explicit
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

// This follows java.util.Comparator
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


#include <iosfwd>
#include <algorithm>
#include <vector>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <seastar/util/backtrace.hh>
#include <iosfwd>
#include <seastar/core/print.hh>
#include <seastar/core/sstring.hh>
#include <seastar/core/enum.hh>

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
    WRAP_AROUND, // Support for handling wrap around ranges in queries on database level and below
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
    // Retruns true iff given prefix has no missing components
    bool is_full(bytes_view v) const {
        assert(AllowPrefixes == allow_prefixes::yes);
        return std::distance(begin(v), end(v)) == (ssize_t)_types.size();
    }
    bool is_empty(bytes_view v) const {
        return begin(v) == end(v);
    }
    void validate(bytes_view v) {
        // FIXME: implement
        warn(unimplemented::cause::VALIDATION);
    }
    bool equal(bytes_view v1, bytes_view v2) {
        if (_byte_order_equal) {
            return compare_unsigned(v1, v2) == 0;
        }
        // FIXME: call equal() on each component
        return compare(v1, v2) == 0;
    }
};

using compound_prefix = compound_type<allow_prefixes::yes>;

#include <optional>
#include <unordered_map>
#include <boost/range/iterator_range.hpp>
#include <boost/range/join.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/dynamic_bitset.hpp>

#include <seastar/core/shared_ptr.hh>
#include <seastar/util/backtrace.hh>

namespace dht {

class i_partitioner;

}

using column_count_type = uint32_t;

// Column ID, unique within column_kind
using column_id = column_count_type;

// Column ID unique within a schema. Enum class to avoid
// mixing wtih column id.
enum class ordinal_column_id: column_count_type {};

std::ostream& operator<<(std::ostream& os, ordinal_column_id id);

// Maintains a set of columns used in a query. The columns are
// identified by ordinal_id.
//
// @sa column_definition::ordinal_id.
class column_set {
public:
    using bitset = boost::dynamic_bitset<uint64_t>;
    using size_type = bitset::size_type;

    // column_count_type is more narrow than size_type, but truncating a size_type max value does
    // give column_count_type max value. This is used to avoid extra branching in
    // find_first()/find_next().
    static_assert(static_cast<column_count_type>(boost::dynamic_bitset<uint64_t>::npos) == ~static_cast<column_count_type>(0));
    static constexpr ordinal_column_id npos = static_cast<ordinal_column_id>(bitset::npos);

    explicit column_set(column_count_type num_bits = 0)
        : _mask(num_bits)
    {
    }

    void resize(column_count_type num_bits) {
        _mask.resize(num_bits);
    }

    // Set the appropriate bit for column id.
    void set(ordinal_column_id id) {
        column_count_type bit = static_cast<column_count_type>(id);
        _mask.set(bit);
    }
    // Test the mask for use of a given column id.
    bool test(ordinal_column_id id) const {
        column_count_type bit = static_cast<column_count_type>(id);
        return _mask.test(bit);
    }
    // @sa boost::dynamic_bistet docs
    size_type count() const { return _mask.count(); }
    ordinal_column_id find_first() const {
        return static_cast<ordinal_column_id>(_mask.find_first());
    }
    ordinal_column_id find_next(ordinal_column_id pos) const {
        return static_cast<ordinal_column_id>(_mask.find_next(static_cast<column_count_type>(pos)));
    }
    // Logical or
    void union_with(const column_set& with) {
        _mask |= with._mask;
    }

private:
    bitset _mask;
};

// Cluster-wide identifier of schema version of particular table.
//
// The version changes the value not only on structural changes but also
// temporal. For example, schemas with the same set of columns but created at
// different times should have different versions. This allows nodes to detect
// if the version they see was already synchronized with or not even if it has
// the same structure as the past versions.
//
// Schema changes merged in any order should result in the same final version.
//
// When table_schema_version changes, schema_tables::calculate_schema_digest() should
// also change when schema mutations are applied.
using table_schema_version = utils::UUID;

class schema;
class schema_registry_entry;
class schema_builder;

// Useful functions to manipulate the schema's comparator field
namespace cell_comparator {
sstring to_sstring(const schema& s);
bool check_compound(sstring comparator);
void read_collections(schema_builder& builder, sstring comparator);
}

namespace db {
class extensions;
}
// make sure these match the order we like columns back from schema
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

/*
 * Sub-schema for thrift aspects. Should be kept isolated (and starved)
 */
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

// Unsafe to access across shards.
// Safe to copy across shards.
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

// Encapsulates information needed for converting mutations between different schema versions.
//
// Unsafe to access across shards.
// Safe to copy across shards.
class column_mapping {
private:
    // Contains _n_static definitions for static columns followed by definitions for regular columns,
    // both ordered by consecutive column_ids.
    // Primary key column sets are not mutable so we don't need to map them.
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

// Represents a column set which is compactible with Cassandra 3.x.
//
// This layout differs from the layout Scylla uses in schema/schema_builder for static compact tables.
// For such tables, Scylla expects all columns to be of regular type and no clustering columns,
// whereas in v3 those columns are static and there is a clustering column with type matching the
// cell name comparator and a regular column with type matching the default validator.
// See issues #2555 and #1474.
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
    // More complex fields are derived from these inside rebuild().
    // Contains only fields which can be safely default-copied.
    struct raw_schema {
        raw_schema(utils::UUID id);
        utils::UUID _id;
        sstring _ks_name;
        sstring _cf_name;
        // regular columns are sorted by name
        // static columns are sorted by name, but present only when there's any clustering column
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
        // FIXME: SizeTiered doesn't really work yet. Being it marked here only means that this is the strategy
        // we will use by default - when we have the choice.
        bool _compaction_enabled = true;
        table_schema_version _version;
        std::unordered_map<sstring, dropped_column> _dropped_columns;
        std::map<bytes, data_type> _collections;
        std::unordered_map<sstring, index_metadata> _indices_by_name;
        // The flag is not stored in the schema mutation and does not affects schema digest.
        // It is set locally on a system tables that should be extra durable
        bool _wait_for_sync = false; // true if all writes using this schema have to be synced immediately by commitlog
    };
    raw_schema _raw;
    thrift_schema _thrift;
    v3_columns _v3_columns;
    mutable schema_registry_entry* _registry_entry = nullptr;
    std::unique_ptr<::view_info> _view_info;

    const std::array<column_count_type, 3> _offsets;

    inline column_count_type column_offset(column_kind k) const {
        return k == column_kind::partition_key ? 0 : _offsets[column_count_type(k) - 1];
    }

    std::unordered_map<bytes, const column_definition*> _columns_by_name;
    lw_shared_ptr<compound_type<allow_prefixes::no>> _partition_key_type;
    lw_shared_ptr<compound_type<allow_prefixes::yes>> _clustering_key_type;
    column_mapping _column_mapping;
    shared_ptr<query::partition_slice> _full_slice;
    column_count_type _clustering_key_size;
    column_count_type _regular_column_count;
    column_count_type _static_column_count;

    extensions_map& extensions() {
        return _raw._extensions;
    }

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
    // deprecated, use schema_builder.
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
    table_schema_version version() const {
        return _raw._version;
    }
    double bloom_filter_fp_chance() const {
        return _raw._bloom_filter_fp_chance;
    }
    sstring thrift_key_validator() const;
    const extensions_map& extensions() const {
        return _raw._extensions;
    }
    bool is_dense() const {
        return _raw._is_dense;
    }

    bool is_compound() const {
        return _raw._is_compound;
    }

    bool is_cql3_table() const {
        return !is_super() && !is_dense() && is_compound();
    }
    bool is_compact_table() const {
        return !is_cql3_table();
    }
    bool is_static_compact_table() const {
        return !is_super() && !is_dense() && !is_compound();
    }

    thrift_schema& thrift() {
        return _thrift;
    }
    const thrift_schema& thrift() const {
        return _thrift;
    }
    const utils::UUID& id() const {
        return _raw._id;
    }
    const sstring& comment() const {
        return _raw._comment;
    }
    bool is_counter() const {
        return _raw._is_counter;
    }

    const cf_type type() const {
        return _raw._type;
    }

    bool is_super() const {
        return _raw._type == cf_type::super;
    }

    gc_clock::duration gc_grace_seconds() const {
        auto seconds = std::chrono::seconds(_raw._gc_grace_seconds);
        return std::chrono::duration_cast<gc_clock::duration>(seconds);
    }

    double dc_local_read_repair_chance() const {
        return _raw._dc_local_read_repair_chance;
    }

    double read_repair_chance() const {
        return _raw._read_repair_chance;
    }
    double crc_check_chance() const {
        return _raw._crc_check_chance;
    }

    int32_t min_compaction_threshold() const {
        return _raw._min_compaction_threshold;
    }

    int32_t max_compaction_threshold() const {
        return _raw._max_compaction_threshold;
    }

    int32_t min_index_interval() const {
        return _raw._min_index_interval;
    }

    int32_t max_index_interval() const {
        return _raw._max_index_interval;
    }

    int32_t memtable_flush_period() const {
        return _raw._memtable_flush_period;
    }

    bool compaction_enabled() const {
        return _raw._compaction_enabled;
    }

    const ::speculative_retry& speculative_retry() const {
        return _raw._speculative_retry;
    }

    dht::i_partitioner& get_partitioner() const;

    const column_definition* get_column_definition(const bytes& name) const;
    const column_definition& column_at(column_kind, column_id) const;
    // Find a column definition given column ordinal id in the schema
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
    // Returns a range of column definitions
    const_iterator_range_type partition_key_columns() const;
    // Returns a range of column definitions
    const_iterator_range_type clustering_key_columns() const;
    // Returns a range of column definitions
    const_iterator_range_type static_columns() const;
    // Returns a range of column definitions
    const_iterator_range_type regular_columns() const;
    // Returns a range of column definitions

    typedef boost::range::joined_range<const_iterator_range_type, const_iterator_range_type>
        select_order_range;

    select_order_range all_columns_in_select_order() const;
    uint32_t position(const column_definition& column) const;

    const columns_type& all_columns() const {
        return _raw._columns;
    }

    const std::unordered_map<bytes, const column_definition*>& columns_by_name() const {
        return _columns_by_name;
    }

    const auto& dropped_columns() const {
        return _raw._dropped_columns;
    }

    const auto& collections() const {
        return _raw._collections;
    }

    gc_clock::duration default_time_to_live() const {
        return _raw._default_time_to_live;
    }

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
    // Returns all index names of this schema.
    std::vector<sstring> index_names() const;
    // Returns all indices of this schema.
    std::vector<index_metadata> indices() const;
    const std::unordered_map<sstring, index_metadata>& all_indices() const;
    // Search for an index with a given name.
    bool has_index(const sstring& index_name) const;
    // Search for an existing index with same kind and options.
    std::optional<index_metadata> find_index_noname(const index_metadata& target) const;
    friend std::ostream& operator<<(std::ostream& os, const schema& s);
    /*!
     * \brief stream the CQL DESCRIBE output.
     *
     * CQL DESCRIBE is implemented at the driver level. This method mimic that functionality
     * inside Scylla.
     *
     * The output of DESCRIBE is the CQL command to create the described table with its indexes and views.
     *
     * For tables with Indexes or Materialized Views, the CQL DESCRIBE is split between the base and view tables.
     * Calling the describe method on the base table schema would result with the CQL "CREATE TABLE"
     * command for creating that table only.
     *
     * Calling the describe method on a view schema would result with the appropriate "CREATE MATERIALIZED VIEW"
     * or "CREATE INDEX" depends on the type of index that schema describes (ie. Materialized View, Global
     * Index or Local Index).
     *
     */
    std::ostream& describe(std::ostream& os) const;
    friend bool operator==(const schema&, const schema&);
    const column_mapping& get_column_mapping() const;
    friend class schema_registry_entry;
    // May be called from different shard
    schema_registry_entry* registry_entry() const noexcept;
    // Returns true iff this schema version was synced with on current node.
    // Schema version is said to be synced with when its mutations were merged
    // into current node's schema, so that current node's schema is at least as
    // recent as this version.
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

/**
 * Wrapper for schema_ptr used by functions that expect an engaged view_info field.
 */
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


// Thrown when attempted to access a schema-dependent object using
// an incompatible version of the schema object.
class schema_mismatch_error : public std::runtime_error {
public:
    schema_mismatch_error(table_schema_version expected, const schema& access);
};

// Throws schema_mismatch_error when a schema-dependent object of "expected" version
// cannot be accessed using "access" schema.
inline void check_schema_version(table_schema_version expected, const schema& access) {
    if (expected != access.version()) {
        throw_with_backtrace<schema_mismatch_error>(expected, access);
    }
}

#include <stdexcept>
#include <type_traits>
#include <seastar/core/sstring.hh>

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

//
// This header provides adaptors between the representation used by our compound_type<>
// and representation used by Origin.
//
// For single-component keys the legacy representation is equivalent
// to the only component's serialized form. For composite keys it the following
// (See org.apache.cassandra.db.marshal.CompositeType):
//
//   <representation> ::= ( <component> )+
//   <component>      ::= <length> <value> <EOC>
//   <length>         ::= <uint16_t>
//   <EOC>            ::= <uint8_t>
//
//  <value> is component's value in serialized form. <EOC> is always 0 for partition key.
//

// Given a representation serialized using @CompoundType, provides a view on the
// representation of the same components as they would be serialized by Origin.
//
// The view is exposed in a form of a byte range. For example of use see to_legacy() function.
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
        // Offset within virtual output space of a component.
        //
        // Offset: -2             -1             0  ...  LEN-1 LEN
        // Field:  [ length MSB ] [ length LSB ] [   VALUE   ] [ EOC ]
        //
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
            } else { // _offset == component_size
                return 0; // EOC field
            }
        }

        iterator& operator++() {
            auto component_size = (int32_t) _i->size();
            if (_offset < component_size
                // When _singular, we skip the EOC byte.
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

    // A trichotomic comparator defined on @CompoundType representations which
    // orders them according to lexicographical ordering of their corresponding
    // legacy representations.
    //
    //   tri_comparator(t)(k1, k2)
    //
    // ...is equivalent to:
    //
    //   compare_unsigned(to_legacy(t, k1), to_legacy(t, k2))
    //
    // ...but more efficient.
    //
    struct tri_comparator {
        const CompoundType& _type;

        tri_comparator(const CompoundType& type)
            : _type(type)
        { }

        // @k1 and @k2 must be serialized using @type, which was passed to the constructor.
        int operator()(bytes_view k1, bytes_view k2) const;
    };

    // Equivalent to std::distance(begin(), end()), but computes faster
    size_t size() const {
        if (_type.is_singular()) {
            return _type.begin(_packed)->size();
        }
        size_t s = 0;
        for (auto&& component : _type.components(_packed)) {
            s += 2 /* length field */ + component.size() + 1 /* EOC */;
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

// Converts compound_type<> representation to legacy representation
// @packed is assumed to be serialized using supplied @type.
template <typename CompoundType>
static inline
bytes to_legacy(CompoundType& type, bytes_view packed) {
    legacy_compound_view<CompoundType> lv(type, packed);
    bytes legacy_form(bytes::initialized_later(), lv.size());
    std::copy(lv.begin(), lv.end(), legacy_form.begin());
    return legacy_form;
}

class composite_view;

// Represents a value serialized according to Origin's CompositeType.
// If is_compound is true, then the value is one or more components encoded as:
//
//   <representation> ::= ( <component> )+
//   <component>      ::= <length> <value> <EOC>
//   <length>         ::= <uint16_t>
//   <EOC>            ::= <uint8_t>
//
// If false, then it encodes a single value, without a prefix length or a suffix EOC.
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
            // Range tombstones are not keys. For collections, only frozen
            // values can be keys. Therefore, for as long as it is safe to
            // assume that this code will be used to create keys, it is safe
            // to assume the trailing byte is always zero.
            write<eoc_type>(out, eoc_type(eoc::none));
        }
    }
    template <typename RangeOfSerializedComponents>
    static size_t serialized_size(RangeOfSerializedComponents&& values, bool is_compound) {
        size_t len = 0;
        auto it = values.begin();
        if (it != values.end()) {
            // CQL3 uses a specific prefix (0xFFFF) to encode "static columns"
            // (CASSANDRA-6561). This does mean the maximum size of the first component of a
            // composite is 65534, not 65535 (or we wouldn't be able to detect if the first 2
            // bytes is the static prefix or not).
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

    // marker is ignored if !is_compound
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
        // FIXME: Optimize
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

#include <stdint.h>
#include <memory>
#include <any>
#include <cstdlib>
#include <seastar/core/memory.hh>
#include <seastar/util/alloc_failure_injector.hh>
#include <malloc.h>

// A function used by compacting collectors to migrate objects during
// compaction. The function should reconstruct the object located at src
// in the location pointed by dst. The object at old location should be
// destroyed. See standard_migrator() above for example. Both src and dst
// are aligned as requested during alloc()/construct().
class migrate_fn_type {
    // Migrators may be registered by thread-local objects. The table of all
    // registered migrators is also thread-local which may cause problems with
    // the order of object destruction and lead to use-after-free.
    // This can be worked around by making migrators keep a shared pointer
    // to the table of migrators. std::any is used so that its type doesn't
    // have to be made public.
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

// Non-constant-size classes (ending with `char data[0]`) must override this
// to tell the allocator about the real size of the object
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

//
// Abstracts allocation strategy for managed objects.
//
// Managed objects may be moved by the allocator during compaction, which
// invalidates any references to those objects. Compaction may be started
// synchronously with allocations. To ensure that references remain valid, use
// logalloc::compaction_lock.
//
// Because references may get invalidated, managing allocators can't be used
// with standard containers, because they assume the reference is valid until freed.
//
// For example containers compatible with compacting allocators see:
//   - managed_ref - managed version of std::unique_ptr<>
//   - managed_bytes - managed version of "bytes"
//
// Note: When object is used as an element inside intrusive containers,
// typically no extra measures need to be taken for reference tracking, if the
// link member is movable. When object is moved, the member hook will be moved
// too and it should take care of updating any back-references. The user must
// be aware though that any iterators into such container may be invalidated
// across deferring points.
//
class allocation_strategy {
protected:
    size_t _preferred_max_contiguous_allocation = std::numeric_limits<size_t>::max();
    uint64_t _invalidate_counter = 1;
public:
    using migrate_fn = const migrate_fn_type*;

    virtual ~allocation_strategy() {}

    //
    // Allocates space for a new ManagedObject. The caller must construct the
    // object before compaction runs. "size" is the amount of space to reserve
    // in bytes. It can be larger than MangedObjects's size.
    //
    // Throws std::bad_alloc on allocation failure.
    //
    // Doesn't invalidate references to objects allocated with this strategy.
    //
    virtual void* alloc(migrate_fn, size_t size, size_t alignment) = 0;

    // Releases storage for the object. Doesn't invoke object's destructor.
    // Doesn't invalidate references to objects allocated with this strategy.
    virtual void free(void* object, size_t size) = 0;
    virtual void free(void* object) = 0;

    // Returns the total immutable memory size used by the allocator to host
    // this object.  This will be at least the size of the object itself, plus
    // any immutable overhead needed to represent the object (if any).
    //
    // The immutable overhead is the overhead that cannot change over the
    // lifetime of the object (such as padding, etc).
    virtual size_t object_memory_size_in_allocator(const void* obj) const noexcept = 0;

    // Like alloc() but also constructs the object with a migrator using
    // standard move semantics. Allocates respecting object's alignment
    // requirement.
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

    // Destroys T and releases its storage.
    // Doesn't invalidate references to allocated objects.
    template<typename T>
    void destroy(T* obj) {
        size_t size = size_for_allocation_strategy(*obj);
        obj->~T();
        free(obj, size);
    }

    size_t preferred_max_contiguous_allocation() const {
        return _preferred_max_contiguous_allocation;
    }

    // Returns a number which is increased when references to objects managed by this allocator
    // are invalidated, e.g. due to internal events like compaction or eviction.
    // When the value returned by this method doesn't change, references obtained
    // between invocations remain valid.
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
        // ASAN doesn't intercept aligned_alloc() and complains on free().
        void* ret;
        // The system posix_memalign will return EINVAL if alignment is not
        // a multiple of pointer size.
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

// std::unique_ptr which can be used for owning an object allocated using allocation_strategy.
// Must be destroyed before the pointer is invalidated. For compacting allocators, that
// means it must not escape outside allocating_section or reclaim lock.
// Must be destroyed in the same allocating context in which T was allocated.
template<typename T>
using alloc_strategy_unique_ptr = std::unique_ptr<T, alloc_strategy_deleter<T>>;

//
// Passing allocators to objects.
//
// The same object type can be allocated using different allocators, for
// example standard allocator (for temporary data), or log-structured
// allocator for long-lived data. In case of LSA, objects may be allocated
// inside different LSA regions. Objects should be freed only from the region
// which owns it.
//
// There's a problem of how to ensure correct usage of allocators. Storing the
// reference to the allocator used for construction of some object inside that
// object is a possible solution. This has a disadvantage of extra space
// overhead per-object though. We could avoid that if the code which decides
// about which allocator to use is also the code which controls object's life
// time. That seems to be the case in current uses, so a simplified scheme of
// passing allocators will do. Allocation strategy is set in a thread-local
// context, as shown below. From there, aware objects pick up the allocation
// strategy. The code controling the objects must ensure that object allocated
// in one regime is also freed in the same regime.
//
// with_allocator() provides a way to set the current allocation strategy used
// within given block of code. with_allocator() can be nested, which will
// temporarily shadow enclosing strategy. Use current_allocator() to obtain
// currently active allocation strategy. Use current_deleter() to obtain a
// Deleter object using current allocation strategy to destroy objects.
//
// Example:
//
//   logalloc::region r;
//   with_allocator(r.allocator(), [] {
//       auto obj = make_managed<int>();
//   });
//

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
#include <seastar/core/unaligned.hh>
#include <seastar/util/alloc_failure_injector.hh>
#include <unordered_map>
#include <type_traits>

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

// A managed version of "bytes" (can be used with LSA).
class managed_bytes {
    static thread_local std::unordered_map<const blob_storage*, std::unique_ptr<bytes_view::value_type[]>> _lc_state;
    struct linearization_context {
        unsigned _nesting = 0;
        // Map from first blob_storage address to linearized version
        // We use the blob_storage address to be insentive to moving
        // a managed_bytes object.
        // linearization_context is entered often in the fast path, but it is
        // actually used only in rare (slow) cases.
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
        int8_t size; // -1 -> use blob_storage
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
            // Workaround for https://github.com/scylladb/scylla/issues/4086
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
            assert(!_u.ptr->next);  // must be linearized
            return _u.ptr->data;
        } else {
            return _u.small.data;
        }
    }

    const blob_storage::char_type* data() const {
        return read_linearize();
    }

    // Returns the amount of external memory used.
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

// Run func() while ensuring that reads of managed_bytes objects are
// temporarlily linearized
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

// blob_storage is a variable-size type
inline
size_t
size_for_allocation_strategy(const blob_storage& bs) {
    return sizeof(bs) + bs.frag_size;
}

// database.hh
class database;
class keyspace;
class table;
using column_family = table;
class memtable_list;

// mutation.hh
class mutation;
class mutation_partition;

// schema.hh
class schema;
class column_definition;
class column_mapping;

// schema_mutations.hh
class schema_mutations;

// keys.hh
class exploded_clustering_prefix;
class partition_key;
class partition_key_view;
class clustering_key_prefix;
class clustering_key_prefix_view;
using clustering_key = clustering_key_prefix;
using clustering_key_view = clustering_key_prefix_view;

// memtable.hh
class memtable;

//
// This header defines type system for primary key holders.
//
// We distinguish partition keys and clustering keys. API-wise they are almost
// the same, but they're separate type hierarchies.
//
// Clustering keys are further divided into prefixed and non-prefixed (full).
// Non-prefixed keys always have full component set, as defined by schema.
// Prefixed ones can have any number of trailing components missing. They may
// differ in underlying representation.
//
// The main classes are:
//
//   partition_key           - full partition key
//   clustering_key          - full clustering key
//   clustering_key_prefix   - clustering key prefix
//
// These classes wrap only the minimum information required to store the key
// (the key value itself). Any information which can be inferred from schema
// is not stored. Therefore accessors need to be provided with a pointer to
// schema, from which information about structure is extracted.

// Abstracts a view to serialized compound.
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

    // begin() and end() return iterators over components of this compound. The iterator yields a bytes_view to the component.
    // The iterators satisfy InputIterator concept.
    auto begin() const {
        return TopLevelView::compound::element_type::begin(representation());
    }

    // See begin()
    auto end() const {
        return TopLevelView::compound::element_type::end(representation());
    }

    // begin() and end() return iterators over components of this compound. The iterator yields a bytes_view to the component.
    // The iterators satisfy InputIterator concept.
    auto begin(const schema& s) const {
        return begin();
    }

    // See begin()
    auto end(const schema& s) const {
        return end();
    }

    bytes_view get_component(const schema& s, size_t idx) const {
        auto it = begin(s);
        std::advance(it, idx);
        return *it;
    }

    // Returns a range of bytes_view
    auto components() const {
        return TopLevelView::compound::element_type::components(representation());
    }

    // Returns a range of bytes_view
    auto components(const schema& s) const {
        return components();
    }

    bool is_empty() const {
        return _bytes.empty();
    }

    explicit operator bool() const {
        return !is_empty();
    }

    // For backward compatibility with existing code.
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

    // We don't allow optional values, but provide this method as an efficient adaptor
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

    // FIXME: return views
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

    // begin() and end() return iterators over components of this compound. The iterator yields a bytes_view to the component.
    // The iterators satisfy InputIterator concept.
    auto begin(const schema& s) const {
        return get_compound_type(s)->begin(_bytes);
    }

    // See begin()
    auto end(const schema& s) const {
        return get_compound_type(s)->end(_bytes);
    }

    bool is_empty() const;

    explicit operator bool() const;

    // For backward compatibility with existing code.
    bool is_empty(const schema& s) const;

    // Returns a range of bytes_view
    auto components() const {
        return TopLevelView::compound::element_type::components(representation());
    }

    // Returns a range of bytes_view
    auto components(const schema& s) const;

    bytes_view get_component(const schema& s, size_t idx) const;

    // Returns the number of components of this compound.
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

    // In prefix equality two sequences are equal if any of them is a prefix
    // of the other. Otherwise lexicographical ordering is applied.
    // Note: full compounds sorted according to lexicographical ordering are also
    // sorted according to prefix equality ordering.
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

    // Can be called only if is_full()
    FullTopLevel to_full(const schema& s) const;

    bool is_prefixed_by(const schema& s, const TopLevel& prefix) const;

    // In prefix equality two sequences are equal if any of them is a prefix
    // of the other. Otherwise lexicographical ordering is applied.
    // Note: full compounds sorted according to lexicographical ordering are also
    // sorted according to prefix equality ordering.
    struct prefix_equality_less_compare {
        prefix_equality_less_compare(const schema& s);

        bool operator()(const TopLevel& k1, const TopLevel& k2) const;
    };

    // See prefix_equality_less_compare.
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
    // Returns key's representation which is compatible with Origin.
    // The result is valid as long as the schema is live.
    const legacy_compound_view<c_type> legacy_form(const schema& s) const;

    // A trichotomic comparator for ordering compatible with Origin.
    int legacy_tri_compare(const schema& s, partition_key_view o) const;

    // Checks if keys are equal in a way which is compatible with Origin.
    bool legacy_equal(const schema& s, partition_key_view o) const;
    // A trichotomic comparator which orders keys according to their ordering on the ring.
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
    // Returns key's representation which is compatible with Origin.
    // The result is valid as long as the schema is live.
    const legacy_compound_view<c_type> legacy_form(const schema& s) const;
    // A trichotomic comparator for ordering compatible with Origin.
    int legacy_tri_compare(const schema& s, const partition_key& o) const;
    // Checks if keys are equal in a way which is compatible with Origin.
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
    /* This function makes the passed clustering key full by filling its
     * missing trailing components with empty values.
     * This is used to represesent clustering keys of rows in compact tables that may be non-full.
     * Returns whether a key wasn't full before the call.
     */
    static bool make_full(const schema& s, clustering_key_prefix& ck);    friend std::ostream& operator<<(std::ostream& out, const clustering_key_prefix& ckp);
};

#include <functional>
#include <list>
#include <vector>
#include <optional>
#include <iosfwd>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/sliced.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <seastar/util/gcc6-concepts.hh>

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
    bool operator==(const range_bound& other) const {
        return (_value == other._value) && (_inclusive == other._inclusive);
    }
    template<typename Comparator>
    bool equal(const range_bound& other, Comparator&& cmp) const {
        return _inclusive == other._inclusive && cmp(_value, other._value) == 0;
    }
};

template<typename T>
class nonwrapping_range;

// A range which can have inclusive, exclusive or open-ended bounds on each end.
// The end bound can be smaller than the start bound.
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
    wrapping_range(optional<bound> start, optional<bound> end, bool singular = false)
        : _start(std::move(start))
        , _singular(singular) {
        if (!_singular) {
            _end = std::move(end);
        }
    }
    wrapping_range(T value)
        : _start(bound(std::move(value), true))
        , _end()
        , _singular(true)
    { }
    wrapping_range() : wrapping_range({}, {}) { }
private:
    // Bound wrappers for compile-time dispatch and safety.
    struct start_bound_ref { const optional<bound>& b; };
    struct end_bound_ref { const optional<bound>& b; };

    start_bound_ref start_bound() const { return { start() }; }
    end_bound_ref end_bound() const { return { end() }; }

    template<typename Comparator>
    static bool greater_than_or_equal(end_bound_ref end, start_bound_ref start, Comparator&& cmp) {
        return !end.b || !start.b || cmp(end.b->value(), start.b->value())
                                     >= (!end.b->is_inclusive() || !start.b->is_inclusive());
    }

    template<typename Comparator>
    static bool less_than(end_bound_ref end, start_bound_ref start, Comparator&& cmp) {
        return !greater_than_or_equal(end, start, cmp);
    }

    template<typename Comparator>
    static bool less_than_or_equal(start_bound_ref first, start_bound_ref second, Comparator&& cmp) {
        return !first.b || (second.b && cmp(first.b->value(), second.b->value())
                                        <= -(!first.b->is_inclusive() && second.b->is_inclusive()));
    }

    template<typename Comparator>
    static bool less_than(start_bound_ref first, start_bound_ref second, Comparator&& cmp) {
        return second.b && (!first.b || cmp(first.b->value(), second.b->value())
                                        < (first.b->is_inclusive() && !second.b->is_inclusive()));
    }

    template<typename Comparator>
    static bool greater_than_or_equal(end_bound_ref first, end_bound_ref second, Comparator&& cmp) {
        return !first.b || (second.b && cmp(first.b->value(), second.b->value())
                                        >= (!first.b->is_inclusive() && second.b->is_inclusive()));
    }
public:
    // the point is before the range (works only for non wrapped ranges)
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool before(const T& point, Comparator&& cmp) const {
        assert(!is_wrap_around(cmp));
        if (!start()) {
            return false; //open start, no points before
        }
        auto r = cmp(point, start()->value());
        if (r < 0) {
            return true;
        }
        if (!start()->is_inclusive() && r == 0) {
            return true;
        }
        return false;
    }
    // the point is after the range (works only for non wrapped ranges)
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool after(const T& point, Comparator&& cmp) const {
        assert(!is_wrap_around(cmp));
        if (!end()) {
            return false; //open end, no points after
        }
        auto r = cmp(end()->value(), point);
        if (r < 0) {
            return true;
        }
        if (!end()->is_inclusive() && r == 0) {
            return true;
        }
        return false;
    }
    // check if two ranges overlap.
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool overlaps(const wrapping_range& other, Comparator&& cmp) const {
        bool this_wraps = is_wrap_around(cmp);
        bool other_wraps = other.is_wrap_around(cmp);

        if (this_wraps && other_wraps) {
            return true;
        } else if (this_wraps) {
            auto unwrapped = unwrap();
            return other.overlaps(unwrapped.first, cmp) || other.overlaps(unwrapped.second, cmp);
        } else if (other_wraps) {
            auto unwrapped = other.unwrap();
            return overlaps(unwrapped.first, cmp) || overlaps(unwrapped.second, cmp);
        }

        // No range should reach this point as wrap around.
        assert(!this_wraps);
        assert(!other_wraps);

        // if both this and other have an open start, the two ranges will overlap.
        if (!start() && !other.start()) {
            return true;
        }

        return greater_than_or_equal(end_bound(), other.start_bound(), cmp)
            && greater_than_or_equal(other.end_bound(), start_bound(), cmp);
    }
    static wrapping_range make(bound start, bound end) {
        return wrapping_range({std::move(start)}, {std::move(end)});
    }
    static wrapping_range make_open_ended_both_sides() {
        return {{}, {}};
    }
    static wrapping_range make_singular(T value) {
        return {std::move(value)};
    }
    static wrapping_range make_starting_with(bound b) {
        return {{std::move(b)}, {}};
    }
    static wrapping_range make_ending_with(bound b) {
        return {{}, {std::move(b)}};
    }
    bool is_singular() const {
        return _singular;
    }
    bool is_full() const {
        return !_start && !_end;
    }
    void reverse() {
        if (!_singular) {
            std::swap(_start, _end);
        }
    }
    const optional<bound>& start() const {
        return _start;
    }
    const optional<bound>& end() const {
        return _singular ? _start : _end;
    }
    // Range is a wrap around if end value is smaller than the start value
    // or they're equal and at least one bound is not inclusive.
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool is_wrap_around(Comparator&& cmp) const {
        if (_end && _start) {
            auto r = cmp(end()->value(), start()->value());
            return r < 0
                   || (r == 0 && (!start()->is_inclusive() || !end()->is_inclusive()));
        } else {
            return false; // open ended range or singular range don't wrap around
        }
    }
    // Converts a wrap-around range to two non-wrap-around ranges.
    // The returned ranges are not overlapping and ordered.
    // Call only when is_wrap_around().
    std::pair<wrapping_range, wrapping_range> unwrap() const {
        return {
            { {}, end() },
            { start(), {} }
        };
    }
    // the point is inside the range
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool contains(const T& point, Comparator&& cmp) const {
        if (is_wrap_around(cmp)) {
            auto unwrapped = unwrap();
            return unwrapped.first.contains(point, cmp)
                   || unwrapped.second.contains(point, cmp);
        } else {
            return !before(point, cmp) && !after(point, cmp);
        }
    }
    // Returns true iff all values contained by other are also contained by this.
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool contains(const wrapping_range& other, Comparator&& cmp) const {
        bool this_wraps = is_wrap_around(cmp);
        bool other_wraps = other.is_wrap_around(cmp);

        if (this_wraps && other_wraps) {
            return cmp(start()->value(), other.start()->value())
                   <= -(!start()->is_inclusive() && other.start()->is_inclusive())
                && cmp(end()->value(), other.end()->value())
                   >= (!end()->is_inclusive() && other.end()->is_inclusive());
        }

        if (!this_wraps && !other_wraps) {
            return less_than_or_equal(start_bound(), other.start_bound(), cmp)
                    && greater_than_or_equal(end_bound(), other.end_bound(), cmp);
        }

        if (other_wraps) { // && !this_wraps
            return !start() && !end();
        }

        // !other_wraps && this_wraps
        return (other.start() && cmp(start()->value(), other.start()->value())
                                 <= -(!start()->is_inclusive() && other.start()->is_inclusive()))
                || (other.end() && cmp(end()->value(), other.end()->value())
                                   >= (!end()->is_inclusive() && other.end()->is_inclusive()));
    }
    // Returns ranges which cover all values covered by this range but not covered by the other range.
    // Ranges are not overlapping and ordered.
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    std::vector<wrapping_range> subtract(const wrapping_range& other, Comparator&& cmp) const {
        std::vector<wrapping_range> result;
        std::list<wrapping_range> left;
        std::list<wrapping_range> right;

        if (is_wrap_around(cmp)) {
            auto u = unwrap();
            left.emplace_back(std::move(u.first));
            left.emplace_back(std::move(u.second));
        } else {
            left.push_back(*this);
        }

        if (other.is_wrap_around(cmp)) {
            auto u = other.unwrap();
            right.emplace_back(std::move(u.first));
            right.emplace_back(std::move(u.second));
        } else {
            right.push_back(other);
        }

        // left and right contain now non-overlapping, ordered ranges

        while (!left.empty() && !right.empty()) {
            auto& r1 = left.front();
            auto& r2 = right.front();
            if (less_than(r2.end_bound(), r1.start_bound(), cmp)) {
                right.pop_front();
            } else if (less_than(r1.end_bound(), r2.start_bound(), cmp)) {
                result.emplace_back(std::move(r1));
                left.pop_front();
            } else { // Overlap
                auto tmp = std::move(r1);
                left.pop_front();
                if (!greater_than_or_equal(r2.end_bound(), tmp.end_bound(), cmp)) {
                    left.push_front({bound(r2.end()->value(), !r2.end()->is_inclusive()), tmp.end()});
                }
                if (!less_than_or_equal(r2.start_bound(), tmp.start_bound(), cmp)) {
                    left.push_front({tmp.start(), bound(r2.start()->value(), !r2.start()->is_inclusive())});
                }
            }
        }

        boost::copy(left, std::back_inserter(result));

        // TODO: Merge adjacent ranges (optimization)
        return result;
    }
    // split range in two around a split_point. split_point has to be inside the range
    // split_point will belong to first range
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    std::pair<wrapping_range<T>, wrapping_range<T>> split(const T& split_point, Comparator&& cmp) const {
        assert(contains(split_point, std::forward<Comparator>(cmp)));
        wrapping_range left(start(), bound(split_point));
        wrapping_range right(bound(split_point, false), end());
        return std::make_pair(std::move(left), std::move(right));
    }
    // Create a sub-range including values greater than the split_point. Returns std::nullopt if
    // split_point is after the end (but not included in the range, in case of wraparound ranges)
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    std::optional<wrapping_range<T>> split_after(const T& split_point, Comparator&& cmp) const {
        if (contains(split_point, std::forward<Comparator>(cmp))
                && (!end() || cmp(split_point, end()->value()) != 0)) {
            return wrapping_range(bound(split_point, false), end());
        } else if (end() && cmp(split_point, end()->value()) >= 0) {
            // whether to return std::nullopt or the full range is not
            // well-defined for wraparound ranges; we return nullopt
            // if split_point is after the end.
            return std::nullopt;
        } else {
            return *this;
        }
    }
    template<typename Bound, typename Transformer, typename U = transformed_type<Transformer>>
    static std::optional<typename wrapping_range<U>::bound> transform_bound(Bound&& b, Transformer&& transformer) {
        if (b) {
            return { { transformer(std::forward<Bound>(b).value().value()), b->is_inclusive() } };
        };
        return {};
    }
    // Transforms this range into a new range of a different value type
    // Supplied transformer should transform value of type T (the old type) into value of type U (the new type).
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

// A range which can have inclusive, exclusive or open-ended bounds on each end.
// The end bound can never be smaller than the start bound.
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
    // Can only be called if start <= end. IDL ctor.
    nonwrapping_range(optional<bound> start, optional<bound> end, bool singular = false)
        : _range(std::move(start), std::move(end), singular)
    { }
    // Can only be called if !r.is_wrap_around().
    explicit nonwrapping_range(wrapping_range<T>&& r)
        : _range(std::move(r))
    { }
    // Can only be called if !r.is_wrap_around().
    explicit nonwrapping_range(const wrapping_range<T>& r)
        : _range(r)
    { }
    operator wrapping_range<T>() const & {
        return _range;
    }
    operator wrapping_range<T>() && {
        return std::move(_range);
    }

    // the point is before the range.
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool before(const T& point, Comparator&& cmp) const {
        return _range.before(point, std::forward<Comparator>(cmp));
    }
    // the point is after the range.
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool after(const T& point, Comparator&& cmp) const {
        return _range.after(point, std::forward<Comparator>(cmp));
    }
    // check if two ranges overlap.
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool overlaps(const nonwrapping_range& other, Comparator&& cmp) const {
        // if both this and other have an open start, the two ranges will overlap.
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
    // the point is inside the range
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool contains(const T& point, Comparator&& cmp) const {
        return !before(point, cmp) && !after(point, cmp);
    }
    // Returns true iff all values contained by other are also contained by this.
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    bool contains(const nonwrapping_range& other, Comparator&& cmp) const {
        return wrapping_range<T>::less_than_or_equal(_range.start_bound(), other._range.start_bound(), cmp)
                && wrapping_range<T>::greater_than_or_equal(_range.end_bound(), other._range.end_bound(), cmp);
    }
    // Returns ranges which cover all values covered by this range but not covered by the other range.
    // Ranges are not overlapping and ordered.
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    std::vector<nonwrapping_range> subtract(const nonwrapping_range& other, Comparator&& cmp) const {
        auto subtracted = _range.subtract(other._range, std::forward<Comparator>(cmp));
        return boost::copy_range<std::vector<nonwrapping_range>>(subtracted | boost::adaptors::transformed([](auto&& r) {
            return nonwrapping_range(std::move(r));
        }));
    }
    // split range in two around a split_point. split_point has to be inside the range
    // split_point will belong to first range
    // Comparator must define a total ordering on T.
    template<typename Comparator>
    std::pair<nonwrapping_range<T>, nonwrapping_range<T>> split(const T& split_point, Comparator&& cmp) const {
        assert(contains(split_point, std::forward<Comparator>(cmp)));
        nonwrapping_range left(start(), bound(split_point));
        nonwrapping_range right(bound(split_point, false), end());
        return std::make_pair(std::move(left), std::move(right));
    }
    // Create a sub-range including values greater than the split_point. If split_point is after
    // the end, returns std::nullopt.
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
    // Creates a new sub-range which is the intersection of this range and a range starting with "start".
    // If there is no overlap, returns std::nullopt.
    template<typename Comparator>
    std::optional<nonwrapping_range> trim_front(std::optional<bound>&& start, Comparator&& cmp) const {
        return intersection(nonwrapping_range(std::move(start), {}), cmp);
    }
    // Transforms this range into a new range of a different value type
    // Supplied transformer should transform value of type T (the old type) into value of type U (the new type).
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
    // Takes a vector of possibly overlapping ranges and returns a vector containing
    // a set of non-overlapping ranges covering the same values.
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
                continue; // last.start <= r.start <= r.end <= last.end
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
    // These private functions optimize the case where a sequence supports the
    // lower and upper bound operations more efficiently, as is the case with
    // some boost containers.
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
    // Return the lower bound of the specified sequence according to these bounds.
    template<typename Range, typename LessComparator>
    typename std::remove_reference<Range>::type::const_iterator lower_bound(Range&& r, LessComparator&& cmp) const {
        return start()
            ? (start()->is_inclusive()
                ? do_lower_bound(start()->value(), std::forward<Range>(r), std::forward<LessComparator>(cmp), built_in_())
                : do_upper_bound(start()->value(), std::forward<Range>(r), std::forward<LessComparator>(cmp), built_in_()))
            : std::cbegin(r);
    }
    // Return the upper bound of the specified sequence according to these bounds.
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
    // Returns a subset of the range that is within these bounds.
    template<typename Range, typename LessComparator>
    boost::iterator_range<typename std::remove_reference<Range>::type::const_iterator>
    slice(Range&& range, LessComparator&& cmp) const {
        return boost::make_iterator_range(lower_bound(range, cmp), upper_bound(range, cmp));
    }

    // Returns the intersection between this range and other.
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

// Allow using range<T> in a hash table. The hash function 31 * left +
// right is the same one used by Cassandra's AbstractBounds.hashCode().
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
    // values 2 to 5 are reserved for forward Origin compatibility
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
        // To make it assignable and to avoid taking a schema_ptr, we
        // wrap the schema reference.
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
        // To make it assignable and to avoid taking a schema_ptr, we
        // wrap the schema reference.
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
#include <optional>

#include <seastar/core/shared_ptr.hh>
#include <seastar/core/sstring.hh>
#include <memory>
#include <random>
#include <utility>
#include <vector>
#include "range.hh"
#include <byteswap.h>
#include "dht/token.hh"

namespace sstables {

class key_view;
class decorated_key_view;

}

namespace dht {

//
// Origin uses a complex class hierarchy where Token is an abstract class,
// and various subclasses use different implementations (LongToken vs.
// BigIntegerToken vs. StringToken), plus other variants to to signify the
// the beginning of the token space etc.
//
// We'll fold all of that into the token class and push all of the variations
// into its users.

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

// Wraps partition_key with its corresponding token.
//
// Total ordering defined by comparators is compatible with Origin's ordering.
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

    // Trichotomic comparators defining total ordering on the union of
    // decorated_key and ring_position objects.
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

    // FIXME: token.tokenFactory
    //virtual token.tokenFactory gettokenFactory() = 0;

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

//
// Represents position in the ring of partitions, where partitions are ordered
// according to decorated_key ordering (first by token, then by key value).
// Intended to be used for defining partition ranges.
//
// The 'key' part is optional. When it's absent, this object represents a position
// which is either before or after all keys sharing given token. That's determined
// by relation_to_keys().
//
// For example for the following data:
//
//   tokens: |    t1   | t2 |
//           +----+----+----+
//   keys:   | k1 | k2 | k3 |
//
// The ordering is:
//
//   ring_position(t1, token_bound::start) < ring_position(k1)
//   ring_position(k1)                     < ring_position(k2)
//   ring_position(k1)                     == decorated_key(k1)
//   ring_position(k2)                     == decorated_key(k2)
//   ring_position(k2)                     < ring_position(t1, token_bound::end)
//   ring_position(k2)                     < ring_position(k3)
//   ring_position(t1, token_bound::end)   < ring_position(t2, token_bound::start)
//
// Maps to org.apache.cassandra.db.RowPosition and its derivatives in Origin.
//
class ring_position {
public:
    enum class token_bound : int8_t { start = -1, end = 1 };
private:
    friend class ring_position_comparator;
    friend class ring_position_ext;
    dht::token _token;
    token_bound _token_bound{}; // valid when !_key
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

    // Valid when !has_key()
    token_bound bound() const {
        return _token_bound;
    }

    // Returns -1 if smaller than keys with the same token, +1 if greater.
    int relation_to_keys() const {
        return _key ? 0 : static_cast<int>(_token_bound);
    }

    const std::optional<partition_key>& key() const {
        return _key;
    }

    bool has_key() const {
        return bool(_key);
    }

    // Call only when has_key()
    dht::decorated_key as_decorated_key() const {
        return { _token, *_key };
    }

    bool equal(const schema&, const ring_position&) const;

    // Trichotomic comparator defining a total ordering on ring_position objects
    int tri_compare(const schema&, const ring_position&) const;

    // "less" comparator corresponding to tri_compare()
    bool less_compare(const schema&, const ring_position&) const;

    friend std::ostream& operator<<(std::ostream&, const ring_position&);
};

// Non-owning version of ring_position and ring_position_ext.
//
// Unlike ring_position, it can express positions which are right after and right before the keys.
// ring_position still can not because it is sent between nodes and such a position
// would not be (yet) properly interpreted by old nodes. That's why any ring_position
// can be converted to ring_position_view, but not the other way.
//
// It is possible to express a partition_range using a pair of two ring_position_views v1 and v2,
// where v1 = ring_position_view::for_range_start(r) and v2 = ring_position_view::for_range_end(r).
// Such range includes all keys k such that v1 <= k < v2, with order defined by ring_position_comparator.
//
class ring_position_view {
    friend int ring_position_tri_compare(const schema& s, ring_position_view lh, ring_position_view rh);
    friend class ring_position_comparator;
    friend class ring_position_ext;

    // Order is lexicographical on (_token, _key) tuples, where _key part may be missing, and
    // _weight affecting order between tuples if one is a prefix of the other (including being equal).
    // A positive weight puts the position after all strictly prefixed by it, while a non-positive
    // weight puts it before them. If tuples are equal, the order is further determined by _weight.
    //
    // For example {_token=t1, _key=nullptr, _weight=1} is ordered after {_token=t1, _key=k1, _weight=0},
    // but {_token=t1, _key=nullptr, _weight=-1} is ordered before it.
    //
    const dht::token* _token; // always not nullptr
    const partition_key* _key; // Can be nullptr
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

    // Only when key() == nullptr
    token_bound get_token_bound() const { return token_bound(_weight); }
    // Only when key() != nullptr
    after_key is_after_key() const { return after_key(_weight == 1); }

    friend std::ostream& operator<<(std::ostream&, ring_position_view);
};

using ring_position_ext_view = ring_position_view;

//
// Represents position in the ring of partitions, where partitions are ordered
// according to decorated_key ordering (first by token, then by key value).
// Intended to be used for defining partition ranges.
//
// Unlike ring_position, it can express positions which are right after and right before the keys.
// ring_position still can not because it is sent between nodes and such a position
// would not be (yet) properly interpreted by old nodes. That's why any ring_position
// can be converted to ring_position_ext, but not the other way.
//
// It is possible to express a partition_range using a pair of two ring_position_exts v1 and v2,
// where v1 = ring_position_ext::for_range_start(r) and v2 = ring_position_ext::for_range_end(r).
// Such range includes all keys k such that v1 <= k < v2, with order defined by ring_position_comparator.
//
class ring_position_ext {
    // Order is lexicographical on (_token, _key) tuples, where _key part may be missing, and
    // _weight affecting order between tuples if one is a prefix of the other (including being equal).
    // A positive weight puts the position after all strictly prefixed by it, while a non-positive
    // weight puts it before them. If tuples are equal, the order is further determined by _weight.
    //
    // For example {_token=t1, _key=nullptr, _weight=1} is ordered after {_token=t1, _key=k1, _weight=0},
    // but {_token=t1, _key=nullptr, _weight=-1} is ordered before it.
    //
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

    // Only when key() == std::nullopt
    token_bound get_token_bound() const { return token_bound(_weight); }

    // Only when key() != std::nullopt
    after_key is_after_key() const { return after_key(_weight == 1); }

    operator ring_position_view() const { return { _token, _key ? &*_key : nullptr, _weight }; }

    friend std::ostream& operator<<(std::ostream&, const ring_position_ext&);
};

int ring_position_tri_compare(const schema& s, ring_position_view lh, ring_position_view rh);

// Trichotomic comparator for ring order
struct ring_position_comparator {
    const schema& s;
    ring_position_comparator(const schema& s_) : s(s_) {}
    int operator()(ring_position_view, ring_position_view) const;
    int operator()(ring_position_view, sstables::decorated_key_view) const;
    int operator()(sstables::decorated_key_view, ring_position_view) const;
};

// "less" comparator giving the same order as ring_position_comparator
struct ring_position_less_comparator {
    ring_position_comparator tri;

    ring_position_less_comparator(const schema& s) : tri(s) {}

    template<typename T, typename U>
    bool operator()(const T& lh, const U& rh) const {
        return tri(lh, rh) < 0;
    }
};

struct token_comparator {
    // Return values are those of a trichotomic comparison.
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

// Each shard gets a sorted, disjoint vector of ranges
std::map<unsigned, dht::partition_range_vector>
split_range_to_shards(dht::partition_range pr, const schema& s);

// If input ranges are sorted and disjoint then the ranges for each shard
// are also sorted and disjoint.
std::map<unsigned, dht::partition_range_vector>
split_ranges_to_shards(const dht::token_range_vector& ranges, const schema& s);

// Intersect a partition_range with a shard and return the the resulting sub-ranges, in sorted order
future<utils::chunked_vector<partition_range>> split_range_to_single_shard(const schema& s, const dht::partition_range& pr, shard_id shard);
future<utils::chunked_vector<partition_range>> split_range_to_single_shard(const i_partitioner& partitioner, const schema& s, const dht::partition_range& pr, shard_id shard);

std::unique_ptr<dht::i_partitioner> make_partitioner(sstring name, unsigned shard_count, unsigned sharding_ignore_msb_bits);

extern std::unique_ptr<i_partitioner> default_partitioner;

} // dht

namespace std {
template<>
struct hash<dht::token> {
    size_t operator()(const dht::token& t) const {
        // We have to reverse the bytes here to keep compatibility with
        // the behaviour that was here when tokens were represented as
        // sequence of bytes.
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
#include "enum_set.hh"
#include "range.hh"
#include "tracing/tracing.hh"

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

/// Trim the clustering ranges.
///
/// Equivalent of intersecting each clustering range with [pos, +inf) position
/// in partition range, or (-inf, pos] position in partition range if
/// reversed == true. Ranges that do not intersect are dropped. Ranges that
/// partially overlap are trimmed.
/// Result: each range will overlap fully with [pos, +inf), or (-int, pos] if
/// reversed is true.
void trim_clustering_row_ranges_to(const schema& s, clustering_row_ranges& ranges, position_in_partition_view pos, bool reversed = false);

/// Trim the clustering ranges.
///
/// Equivalent of intersecting each clustering range with (key, +inf) clustering
/// range, or (-inf, key) clustering range if reversed == true. Ranges that do
/// not intersect are dropped. Ranges that partially overlap are trimmed.
/// Result: each range will overlap fully with (key, +inf), or (-int, key) if
/// reversed is true.
void trim_clustering_row_ranges_to(const schema& s, clustering_row_ranges& ranges, const clustering_key& key, bool reversed = false);

class specific_ranges {
};

constexpr auto max_rows = std::numeric_limits<uint32_t>::max();

// Specifies subset of rows, columns and cell attributes to be returned in a query.
// Can be accessed across cores.
// Schema-dependent.
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
        // Normally, we don't return static row if the request has clustering
        // key restrictions and the partition doesn't have any rows matching
        // the restrictions, see #589. This flag overrides this behavior.
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
    void clear_ranges();    // FIXME: possibly make this function return a const ref instead.
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

#include <optional>

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
    // Returns placement of this position_in_partition relative to *_ck,
    // or lexicographical_relation::at_prefix if !_ck.
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

    // Returns true if all fragments that can be seen for given schema have
    // positions >= than this. partition_start is ignored.
    bool is_before_all_fragments(const schema& s) const;

    bool is_after_all_clustered_rows(const schema& s) const;

    // Valid when >= before_all_clustered_rows()
    const clustering_key_prefix& key() const;

    // Can be called only when !is_static_row && !is_clustering_row().
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

    // If given position is a clustering row position, returns a position
    // right after it. Otherwise returns it unchanged.
    // The position "pos" must be a clustering position.
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

    // Defines total order on the union of position_and_partition and composite objects.
    //
    // The ordering is compatible with position_range (r). The following is satisfied for
    // all cells with name c included by the range:
    //
    //   r.start() <= c < r.end()
    //
    // The ordering on composites given by this is compatible with but weaker than the cell name order.
    //
    // The ordering on position_in_partition given by this is compatible but weaker than the ordering
    // given by position_in_partition::tri_compare.
    //
    class composite_tri_compare {
    public:
        static int rank(partition_region t);

        composite_tri_compare(const schema& s);

        int operator()(position_in_partition_view a, position_in_partition_view b) const;

        int operator()(position_in_partition_view a, composite_view b) const;
        int operator()(composite_view a, position_in_partition_view b) const;
    };

    // Less comparator giving the same order as composite_tri_compare.
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

// Returns true if and only if there can't be any clustering_row with position > a and < b.
// It is assumed that a <= b.
bool no_clustering_row_between(const schema& s, position_in_partition_view a, position_in_partition_view b);

// Includes all position_in_partition objects "p" for which: start <= p < end
// And only those.
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

    // Constructs position_range which covers the same rows as given clustering_range.
    // position_range includes a fragment if it includes position of that fragment.
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

#include "atomic_cell_or_collection.hh"
#include "query-result.hh"
#include "mutation_partition_view.hh"
#include "utils/managed_vector.hh"
#include "range_tombstone_list.hh"
#include "clustering_key_filter.hh"
#include "utils/with_relational_operators.hh"
#include "utils/managed_ref.hh"

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

//
// Container for cells of a row. Cells are identified by column_id.
//
// All cells must belong to a single column_kind. The kind is not stored
// for space-efficiency reasons. Whenever a method accepts a column_kind,
// the caller must always supply the same column_kind.
//
// Can be used as a range of row::cell_entry.
//
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
    // Calls Func(column_id, cell_and_hash&) or Func(column_id, atomic_cell_and_collection&)
    // for each cell in this row, depending on the concrete Func type.
    // noexcept if Func doesn't throw.
    template<typename Func>
    void for_each_cell(Func&& func);

    template<typename Func>
    void for_each_cell(Func&& func) const;

    template<typename Func>
    void for_each_cell_until(Func&& func) const;

    // Merges cell's value into the row.
    // Weak exception guarantees.
    void apply(const column_definition& column, const atomic_cell_or_collection& cell, cell_hash_opt hash = cell_hash_opt());

    // Merges cell's value into the row.
    // Weak exception guarantees.
    void apply(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());

    // Monotonic exception guarantees. In case of exception the sum of cell and this remains the same as before the exception.
    void apply_monotonically(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());

    // Adds cell to the row. The column must not be already set.
    void append_cell(column_id id, atomic_cell_or_collection cell);

    // Weak exception guarantees
    void apply(const schema&, column_kind, const row& src);
    // Weak exception guarantees
    void apply(const schema&, column_kind, row&& src);
    // Monotonic exception guarantees
    void apply_monotonically(const schema&, column_kind, row&& src);

    // Expires cells based on query_time. Expires tombstones based on gc_before
    // and max_purgeable. Removes cells covered by tomb.
    // Returns true iff there are any live cells left.
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

// Like row, but optimized for the case where the row doesn't exist (e.g. static rows)
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

    // Returns a pointer to cell's value or nullptr if column is not set.
    const atomic_cell_or_collection* find_cell(column_id id) const;

    // Returns a pointer to cell's value and hash or nullptr if column is not set.
    const cell_and_hash* find_cell_and_hash(column_id id) const;

    // Calls Func(column_id, cell_and_hash&) or Func(column_id, atomic_cell_and_collection&)
    // for each cell in this row, depending on the concrete Func type.
    // noexcept if Func doesn't throw.
    template<typename Func>
    void for_each_cell(Func&& func);

    template<typename Func>
    void for_each_cell(Func&& func) const;

    template<typename Func>
    void for_each_cell_until(Func&& func) const;

    // Merges cell's value into the row.
    // Weak exception guarantees.
    void apply(const column_definition& column, const atomic_cell_or_collection& cell, cell_hash_opt hash = cell_hash_opt());
    // Merges cell's value into the row.
    // Weak exception guarantees.
    void apply(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());

    // Monotonic exception guarantees. In case of exception the sum of cell and this remains the same as before the exception.
    void apply_monotonically(const column_definition& column, atomic_cell_or_collection&& cell, cell_hash_opt hash = cell_hash_opt());
    // Adds cell to the row. The column must not be already set.
    void append_cell(column_id id, atomic_cell_or_collection cell);

    // Weak exception guarantees
    void apply(const schema& s, column_kind kind, const row& src);
    // Weak exception guarantees
    void apply(const schema& s, column_kind kind, const lazy_row& src);

    // Weak exception guarantees
    void apply(const schema& s, column_kind kind, row&& src);

    // Monotonic exception guarantees
    void apply_monotonically(const schema& s, column_kind kind, row&& src);
    // Monotonic exception guarantees
    void apply_monotonically(const schema& s, column_kind kind, lazy_row&& src);
    // Expires cells based on query_time. Expires tombstones based on gc_before
    // and max_purgeable. Removes cells covered by tomb.
    // Returns true iff there are any live cells left.
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
    // Can be called only when !is_missing().
    bool is_dead(gc_clock::time_point now) const {
        if (_ttl == dead) {
            return true;
        }
        return _ttl != no_ttl && _expiry <= now;
    }
    // Can be called only when is_live().
    bool is_expiring() const {
        return _ttl != no_ttl;
    }
    // Can be called only when is_expiring().
    gc_clock::duration ttl() const {
        return _ttl;
    }
    // Can be called only when is_expiring().
    gc_clock::time_point expiry() const {
        return _expiry;
    }
    // Should be called when is_dead() or is_expiring().
    // Safe to be called when is_missing().
    // When is_expiring(), returns the the deletion time of the marker when it finally expires.
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
    // Expires cells and tombstones. Removes items covered by higher level
    // tombstones.
    // Returns true if row marker is live.
    bool compact_and_expire(tombstone tomb, gc_clock::time_point now,
            can_gc_fn& can_gc, gc_clock::time_point gc_before, compaction_garbage_collector* collector = nullptr);
    // Consistent with feed_hash()
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
    // Consistent with operator==()
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

    // A shadowable row tombstone is valid only if the row has no live marker. In other words,
    // the row tombstone is only valid as long as no newer insert is done (thus setting a
    // live row marker; note that if the row timestamp set is lower than the tombstone's,
    // then the tombstone remains in effect as usual). If a row has a shadowable tombstone
    // with timestamp Ti and that row is updated with a timestamp Tj, such that Tj > Ti
    // (and that update sets the row marker), then the shadowable tombstone is shadowed by
    // that update. A concrete consequence is that if the update has cells with timestamp
    // lower than Ti, then those cells are preserved (since the deletion is removed), and
    // this is contrary to a regular, non-shadowable row tombstone where the tombstone is
    // preserved and such cells are removed.
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


/*
The rules for row_tombstones are as follows:
  - The shadowable tombstone is always >= than the regular one;
  - The regular tombstone works as expected;
  - The shadowable tombstone doesn't erase or compact away the regular
    row tombstone, nor dead cells;
  - The shadowable tombstone can erase live cells, but only provided they
    can be recovered (e.g., by including all cells in a MV update, both
    updated cells and pre-existing ones);
  - The shadowable tombstone can be erased or compacted away by a newer
    row marker.
*/
class row_tombstone : public with_relational_operators<row_tombstone> {
    tombstone _regular;
    shadowable_tombstone _shadowable; // _shadowable is always >= _regular
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

    // Weak exception guarantees. After exception, both src and this will commute to the same value as
    // they would should the exception not happen.
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
    // Valid only if !dummy()
    clustering_key& key();
    // Valid only if !dummy()
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

// Represents a set of writes made to a single partition.
//
// The object is schema-dependent. Each instance is governed by some
// specific schema version. Accessors require a reference to the schema object
// of that version.
//
// There is an operation of addition defined on mutation_partition objects
// (also called "apply"), which gives as a result an object representing the
// sum of writes contained in the addends. For instances governed by the same
// schema, addition is commutative and associative.
//
// In addition to representing writes, the object supports specifying a set of
// partition elements called "continuity". This set can be used to represent
// lack of information about certain parts of the partition. It can be
// specified which ranges of clustering keys belong to that set. We say that a
// key range is continuous if all keys in that range belong to the continuity
// set, and discontinuous otherwise. By default everything is continuous.
// The static row may be also continuous or not.
// Partition tombstone is always continuous.
//
// Continuity is ignored by instance equality. It's also transient, not
// preserved by serialization.
//
// Continuity is represented internally using flags on row entries. The key
// range between two consecutive entries (both ends exclusive) is continuous
// if and only if rows_entry::continuous() is true for the later entry. The
// range starting after the last entry is assumed to be continuous. The range
// corresponding to the key of the entry is continuous if and only if
// rows_entry::dummy() is false.
//
// Adding two fully-continuous instances gives a fully-continuous instance.
// Continuity doesn't affect how the write part is added.
//
// Addition of continuity is not commutative in general, but is associative.
// The default continuity merging rules are those required by MVCC to
// preserve its invariants. For details, refer to "Continuity merging rules" section
// in the doc in partition_version.hh.
class mutation_partition final {
public:
    friend class rows_entry;
    friend class size_calculator;
    friend class converting_mutation_partition_applier;
public:
    struct copy_comparators_only {};
    struct incomplete_tag {};
    // Constructs an empty instance which is fully discontinuous except for the partition tombstone.
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
    // Consistent with equal()
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
    // Makes sure there is a dummy entry after all clustered rows. Doesn't affect continuity.
    // Doesn't invalidate iterators.
    void ensure_last_dummy(const schema&);
    bool static_row_continuous() const;
    void set_static_row_continuous(bool value);
    bool is_fully_continuous() const;
    void make_fully_continuous();
    // Sets or clears continuity of clustering ranges between existing rows.
    void set_continuity(const schema&, const position_range& pr, is_continuous);
    // Returns clustering row ranges which have continuity matching the is_continuous argument.
    clustering_interval_set get_continuity(const schema&, is_continuous = is_continuous::yes) const;
    // Returns true iff all keys from given range are marked as continuous, or range is empty.
    bool fully_continuous(const schema&, const position_range&);
    // Returns true iff all keys from given range are marked as not continuous and range is not empty.
    bool fully_discontinuous(const schema&, const position_range&);
    // Returns true iff all keys from given range have continuity membership as specified by is_continuous.
    bool check_continuity(const schema&, const position_range&, is_continuous) const;
    // Frees elements of the partition in batches.
    // Returns stop_iteration::yes iff there are no more elements to free.
    // Continuity is unspecified after this.
    stop_iteration clear_gently(cache_tracker*) noexcept;
    // Applies mutation_fragment.
    // The fragment must be goverened by the same schema as this object.
    void apply(const schema& s, const mutation_fragment&);
    void apply(tombstone t);
    void apply_delete(const schema& schema, const clustering_key_prefix& prefix, tombstone t);
    void apply_delete(const schema& schema, range_tombstone rt);
    void apply_delete(const schema& schema, clustering_key_prefix&& prefix, tombstone t);
    void apply_delete(const schema& schema, clustering_key_prefix_view prefix, tombstone t);
    // Equivalent to applying a mutation with an empty row, created with given timestamp
    void apply_insert(const schema& s, clustering_key_view, api::timestamp_type created_at);
    void apply_insert(const schema& s, clustering_key_view, api::timestamp_type created_at,
                      gc_clock::duration ttl, gc_clock::time_point expiry);
    // prefix must not be full
    void apply_row_tombstone(const schema& schema, clustering_key_prefix prefix, tombstone t);
    void apply_row_tombstone(const schema& schema, range_tombstone rt);
    //
    // Applies p to current object.
    //
    // Commutative when this_schema == p_schema. If schemas differ, data in p which
    // is not representable in this_schema is dropped, thus apply() loses commutativity.
    //
    // Weak exception guarantees.
    void apply(const schema& this_schema, const mutation_partition& p, const schema& p_schema,
            mutation_application_stats& app_stats);
    // Use in case this instance and p share the same schema.
    // Same guarantees as apply(const schema&, mutation_partition&&, const schema&);
    void apply(const schema& s, mutation_partition&& p, mutation_application_stats& app_stats);
    // Same guarantees and constraints as for apply(const schema&, const mutation_partition&, const schema&).
    void apply(const schema& this_schema, mutation_partition_view p, const schema& p_schema,
            mutation_application_stats& app_stats);

    // Applies p to this instance.
    //
    // Monotonic exception guarantees. In case of exception the sum of p and this remains the same as before the exception.
    // This instance and p are governed by the same schema.
    //
    // Must be provided with a pointer to the cache_tracker, which owns both this and p.
    //
    // Returns stop_iteration::no if the operation was preempted before finished, and stop_iteration::yes otherwise.
    // On preemption the sum of this and p stays the same (represents the same set of writes), and the state of this
    // object contains at least all the writes it contained before the call (monotonicity). It may contain partial writes.
    // Also, some progress is always guaranteed (liveness).
    //
    // The operation can be drien to completion like this:
    //
    //   while (apply_monotonically(..., is_preemtable::yes) == stop_iteration::no) { }
    //
    // If is_preemptible::no is passed as argument then stop_iteration::no is never returned.
    stop_iteration apply_monotonically(const schema& s, mutation_partition&& p, cache_tracker*,
            mutation_application_stats& app_stats, is_preemptible = is_preemptible::no);
    stop_iteration apply_monotonically(const schema& s, mutation_partition&& p, const schema& p_schema,
            mutation_application_stats& app_stats, is_preemptible = is_preemptible::no);

    // Weak exception guarantees.
    // Assumes this and p are not owned by a cache_tracker.
    void apply_weak(const schema& s, const mutation_partition& p, const schema& p_schema,
            mutation_application_stats& app_stats);
    void apply_weak(const schema& s, mutation_partition&&,
            mutation_application_stats& app_stats);
    void apply_weak(const schema& s, mutation_partition_view p, const schema& p_schema,
            mutation_application_stats& app_stats);

    // Converts partition to the new schema. When succeeds the partition should only be accessed
    // using the new schema.
    //
    // Strong exception guarantees.
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

    // Calls func for each row entry inside row_ranges until func returns stop_iteration::yes.
    // Removes all entries for which func didn't return stop_iteration::no or wasn't called at all.
    // Removes all entries that are empty, check rows_entry::empty().
    // If reversed is true, func will be called on entries in reverse order. In that case row_ranges
    // must be already in reverse order.
    template<bool reversed, typename Func>
    void trim_rows(const schema& s,
        const std::vector<query::clustering_range>& row_ranges,
        Func&& func);
public:
    // Performs the following:
    //   - throws out data which doesn't belong to row_ranges
    //   - expires cells and tombstones based on query_time
    //   - drops cells covered by higher-level tombstones (compaction)
    //   - leaves at most row_limit live rows
    //
    // Note: a partition with a static row which has any cell live but no
    // clustered rows still counts as one row, according to the CQL row
    // counting rules.
    //
    // Returns the count of CQL rows which remained. If the returned number is
    // smaller than the row_limit it means that there was no more data
    // satisfying the query left.
    //
    // The row_limit parameter must be > 0.
    //
    uint32_t compact_for_query(const schema& s, gc_clock::time_point query_time,
        const std::vector<query::clustering_range>& row_ranges, bool always_return_static_content,
        bool reversed, uint32_t row_limit);

    // Performs the following:
    //   - expires cells based on compaction_time
    //   - drops cells covered by higher-level tombstones
    //   - drops expired tombstones which timestamp is before max_purgeable
    void compact_for_compaction(const schema& s, can_gc_fn&,
        gc_clock::time_point compaction_time);

    // Returns the minimal mutation_partition that when applied to "other" will
    // create a mutation_partition equal to the sum of other and this one.
    // This and other must both be governed by the same schema s.
    mutation_partition difference(schema_ptr s, const mutation_partition& other) const;

    // Returns a subset of this mutation holding only information relevant for given clustering ranges.
    // Range tombstones will be trimmed to the boundaries of the clustering ranges.
    mutation_partition sliced(const schema& s, const query::clustering_row_ranges&) const;

    // Returns true if the mutation_partition represents no writes.
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
    // return a set of rows_entry where each entry represents a CQL row sharing the same clustering key.
    const range_tombstone_list& row_tombstones() const;
    range_tombstone_list& row_tombstones();
    const row* find_row(const schema& s, const clustering_key& key) const;
    tombstone range_tombstone_for_row(const schema& schema, const clustering_key& key) const;
    row_tombstone tombstone_for_row(const schema& schema, const clustering_key& key) const;
    // Can be called only for non-dummy entries
    row_tombstone tombstone_for_row(const schema& schema, const rows_entry& e) const;
    // Returns an iterator range of rows_entry, with only non-dummy entries.
    // Writes this partition using supplied query result writer.
    // The partition should be first compacted with compact_for_query(), otherwise
    // results may include data which is deleted/expired.
    // At most row_limit CQL rows will be written and digested.
    void query_compacted(query::result::partition_writer& pw, const schema& s, uint32_t row_limit) const;
    void accept(const schema&, mutation_partition_visitor&) const;

    // Returns the number of live CQL rows in this partition.
    //
    // Note: If no regular rows are live, but there's something live in the
    // static row, the static row counts as one row. If there is at least one
    // regular row live, static row doesn't count.
    //
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


#include "dht/i_partitioner.hh"
#include "hashing.hh"
#include "mutation_fragment.hh"

#include <seastar/util/optimized_optional.hh>

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

    // Upgrades this mutation to a newer schema. The new schema must
    // be obtained using only valid schema transformation:
    //  * primary key column count must not change
    //  * column types may only change to those with compatible representations
    //
    // After upgrade, mutation's partition should only be accessed using the new schema. User must
    // ensure proper isolation of accesses.
    //
    // Strong exception guarantees.
    //
    // Note that the conversion may lose information, it's possible that m1 != m2 after:
    //
    //   auto m2 = m1;
    //   m2.upgrade(s2);
    //   m2.upgrade(m1.schema());
    //
    void upgrade(const schema_ptr&);

    const partition_key& key() const;
    const dht::decorated_key& decorated_key() const;
    dht::ring_position ring_position() const;
    const dht::token& token() const;
    const schema_ptr& schema() const;
    const mutation_partition& partition() const;
    mutation_partition& partition();
    const utils::UUID& column_family_id() const;
    // Consistent with hash<canonical_mutation>
    bool operator==(const mutation&) const;
    bool operator!=(const mutation&) const;
public:
    // The supplied partition_slice must be governed by this mutation's schema
    query::result query(const query::partition_slice&,
        query::result_options opts = query::result_options::only_result(),
        gc_clock::time_point now = gc_clock::now(),
        uint32_t row_limit = query::max_rows) &&;

    // The supplied partition_slice must be governed by this mutation's schema
    // FIXME: Slower than the r-value version
    query::result query(const query::partition_slice&,
        query::result_options opts = query::result_options::only_result(),
        gc_clock::time_point now = gc_clock::now(),
        uint32_t row_limit = query::max_rows) const&;

    // The supplied partition_slice must be governed by this mutation's schema
    void query(query::result::builder& builder,
        const query::partition_slice& slice,
        gc_clock::time_point now = gc_clock::now(),
        uint32_t row_limit = query::max_rows) &&;

    // See mutation_partition::live_row_count()
    size_t live_row_count(gc_clock::time_point query_time = gc_clock::time_point::min()) const;

    void apply(mutation&&);
    void apply(const mutation&);
    void apply(const mutation_fragment&);

    mutation operator+(const mutation& other) const;
    mutation& operator+=(const mutation& other);
    mutation& operator+=(mutation&& other);

    // Returns a subset of this mutation holding only information relevant for given clustering ranges.
    // Range tombstones will be trimmed to the boundaries of the clustering ranges.
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

// Consistent with operator==()
// Consistent across the cluster, so should not rely on particular
// serialization format, only on actual data stored.
template<>
struct appending_hash<mutation> {
    template<typename Hasher>
    void operator()(Hasher& h, const mutation& m) const;
};

void apply(mutation_opt& dst, mutation&& src);

void apply(mutation_opt& dst, mutation_opt&& src);

// Returns a range into partitions containing mutations covered by the range.
// partitions must be sorted according to decorated key.
// range must not wrap around.
boost::iterator_range<std::vector<mutation>::const_iterator> slice(
    const std::vector<mutation>& partitions,
    const dht::partition_range&);

class flat_mutation_reader;

// Reads a single partition from a reader. Returns empty optional if there are no more partitions to be read.
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

// We could just add all memtables, regardless of types, to a single list, and
// then filter them out when we read them. Here's why I have chosen not to do
// it:
//
// First, some of the methods in which a memtable is involved (like seal) are
// assume a commitlog, and go through great care of updating the replay
// position, flushing the log, etc.  We want to bypass those, and that has to
// be done either by sprikling the seal code with conditionals, or having a
// separate method for each seal.
//
// Also, if we ever want to put some of the memtables in as separate allocator
// region group to provide for extra QoS, having the classes properly wrapped
// will make that trivial: just pass a version of new_memtable() that puts it
// in a different region, while the list approach would require a lot of
// conditionals as well.
//
// If we are going to have different methods, better have different instances
// of a common class.
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


// Policy for distributed<database>:
//   broadcast metadata writes
//   local metadata reads
//   use shard_of() for data

class database {
private:
    future<mutation> do_apply_counter_update(column_family& cf, const frozen_mutation& fm, schema_ptr m_schema, db::timeout_clock::time_point timeout,
                                             tracing::trace_state_ptr trace_state);
public:

};



#include <seastar/core/future-util.hh>
#include "frozen_mutation.hh"
#include <seastar/core/do_with.hh>
#include "mutation_query.hh"

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

            // Before counter update is applied it needs to be transformed from
            // deltas to counter shards. To do that, we need to read the current
            // counter state for each modified cell...

            return counter_write_query(schema_ptr(), mutation_source(), m.decorated_key(), slice, nullptr)
                    .then([this, &cf, &m, timeout] (auto mopt) {
                // ...now, that we got existing state of all affected counter
                // cells we can look for our shard in each of them, increment
                // its clock and apply the delta.
                return std::move(m);
            });
        });
    });
}

