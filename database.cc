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
    void operator()(Hasher& h, const bytes& v) const {
        feed_hash(h, v.size());
        h.update(reinterpret_cast<const char*>(v.cbegin()), v.size() * sizeof(bytes::value_type));
    }
};

template<>
struct appending_hash<bytes_view> {
    template<typename Hasher>
    void operator()(Hasher& h, bytes_view v) const {
        feed_hash(h, v.size());
        h.update(reinterpret_cast<const char*>(v.begin()), v.size() * sizeof(bytes_view::value_type));
    }
};

inline int32_t compare_unsigned(bytes_view v1, bytes_view v2) {
    auto n = memcmp(v1.begin(), v2.begin(), std::min(v1.size(), v2.size()));
    if (n) {
        return n;
    }
    return (int32_t) (v1.size() - v2.size());
}

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include "compound.hh"
#include "schema.hh"
#include "sstables/version.hh"

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


#include "utils/managed_bytes.hh"
#include "hashing.hh"
#include "database_fwd.hh"
#include "schema_fwd.hh"

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

#include "position_in_partition.hh"
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
    atomic_cell_or_collection cell;
    mutable cell_hash_opt hash;

    cell_and_hash() = default;
    cell_and_hash(cell_and_hash&&) noexcept = default;
    cell_and_hash& operator=(cell_and_hash&&) noexcept = default;

    cell_and_hash(atomic_cell_or_collection&& cell, cell_hash_opt hash);
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
        cell_entry(column_id id, cell_and_hash c_a_h);
        cell_entry(column_id id, atomic_cell_or_collection cell);
        cell_entry(column_id id);
        cell_entry(cell_entry&&) noexcept;
        cell_entry(const abstract_type&, const cell_entry&);

        column_id id() const;
        const atomic_cell_or_collection& cell() const;
        atomic_cell_or_collection& cell();
        const cell_hash_opt& hash() const;
        const cell_and_hash& get_cell_and_hash() const;
        cell_and_hash& get_cell_and_hash();

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

    const atomic_cell_or_collection& cell_at(column_id id) const;

    // Returns a pointer to cell's value or nullptr if column is not set.
    const atomic_cell_or_collection* find_cell(column_id id) const;
    // Returns a pointer to cell's value and hash or nullptr if column is not set.
    const cell_and_hash* find_cell_and_hash(column_id id) const;
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


#include "keys.hh"
#include "schema_fwd.hh"
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

using sstable_list = sstables::sstable_list;


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

