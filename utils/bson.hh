/*
 * Copyright (C) 2025-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

#pragma once

#include "bytes_ostream.hh"
#include "marshal_exception.hh"
#include "utils/managed_bytes.hh"
#include "utils/fragment_range.hh"

#include <seastar/core/byteorder.hh>
#include <seastar/core/format.hh>

#include <bit>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>

namespace bson {

// BSON element type codes per https://bsonspec.org/spec.html
// Deprecated types (undefined, dbpointer, symbol, code_w_scope) are omitted.
enum class type : uint8_t {
    double_value = 0x01,
    string       = 0x02,
    document     = 0x03,
    array        = 0x04,
    binary       = 0x05,
    object_id    = 0x07,
    boolean      = 0x08,
    datetime     = 0x09,
    null         = 0x0A,
    regex        = 0x0B,
    javascript   = 0x0D,
    int32        = 0x10,
    timestamp    = 0x11,
    int64        = 0x12,
    decimal128   = 0x13,
    max_key      = 0x7F,
    min_key      = 0xFF,
};

// A BSON document stored as managed_bytes.
//
// The managed_bytes contains the full BSON wire format:
//   int32_le (total document size) | element* | 0x00
//
// The document is self-describing: the int32 length prefix equals size().
class document {
    managed_bytes _data;

    explicit document(managed_bytes data) : _data(std::move(data)) {}
public:
    // Default constructor creates an empty (zero-length) document.
    // This is NOT a valid BSON document — it represents the CQL "empty"
    // state (a zero-length cell value, distinct from NULL).
    document() = default;

    document(const document&) = default;
    document& operator=(const document&) = default;
    document(document&&) noexcept = default;
    document& operator=(document&&) noexcept = default;

    // Construct a document from raw managed_bytes without validation.
    // Precondition: the bytes must represent a valid BSON document.
    static document from_managed_bytes_unsafe(managed_bytes mb) {
        return document(std::move(mb));
    }

    // Total byte size of the serialized BSON document (including the
    // 4-byte length prefix and the 0x00 terminator).
    size_t size() const { return _data.size(); }

    bool empty() const { return _data.size() == 0; }

    template <typename Self>
    decltype(auto) as_managed_bytes(this Self&& self) {
        return std::forward_like<Self>(self._data);
    }
};

// --- BSON document reader (zero-copy, fragment-aware) ---
//
// Iterates elements of a validated BSON document without linearizing.
// Works on any FragmentedView (managed_bytes_view, single_fragmented_view).
// Use with_simplified() for optimal code generation on the common single-
// fragment case.
//
// Usage:
//   with_simplified(managed_bytes_view(doc.as_managed_bytes()),
//       [](FragmentedView auto v) {
//           for (auto&& e : bson::reader(v)) {
//               switch (e.type) {
//               case bson::type::string: handle(e.key, e.as_string()); break;
//               ...
//               }
//           }
//       });

// Read a little-endian integer from a fragmented view, advancing the cursor.
// Precondition: the cursor has at least sizeof(T) bytes remaining.
template <std::integral T, FragmentedView View>
inline T read_le(View& v) {
    T buf;
    if (v.current_fragment().size() >= sizeof(T)) [[likely]] {
        std::memcpy(&buf, v.current_fragment().data(), sizeof(T));
        v.remove_prefix(sizeof(T));
    } else {
        read_fragmented(v, sizeof(T),
            reinterpret_cast<bytes::value_type*>(&buf));
    }
    return seastar::le_to_cpu(buf);
}

// Read a null-terminated c-string from a fragmented view.
// Returns the string content (without the null terminator).
// Advances cursor past the null terminator.
// Precondition: cursor contains a valid null-terminated c-string.
template <FragmentedView View>
inline sstring read_cstring(View& cursor) {
    sstring result;
    while (cursor.size_bytes() > 0) {
        auto frag = cursor.current_fragment();
        auto p = reinterpret_cast<const char*>(frag.data());
        auto null_pos = static_cast<const char*>(std::memchr(p, 0, frag.size()));
        if (null_pos) {
            auto len = static_cast<size_t>(null_pos - p);
            result.append(p, len);
            cursor.remove_prefix(len + 1);
            return result;
        }
        result.append(p, frag.size());
        cursor.remove_current();
    }
    __builtin_unreachable();
}

// A single element in a BSON document.
// Holds a sub-view of the value payload — the underlying data must outlive
// this object.
template <FragmentedView View>
struct element {
    bson::type type;
    sstring key;
    View value; // raw value payload; interpretation depends on type

    // Precondition: type == type::double_value
    double as_double() const {
        auto v = value;
        return std::bit_cast<double>(read_le<uint64_t>(v));
    }

    // Precondition: type == type::string or type::javascript
    sstring as_string() const {
        auto v = value;
        auto len = read_le<uint32_t>(v);
        sstring result(sstring::initialized_later{}, len - 1);
        read_fragmented(v, len - 1,
            reinterpret_cast<bytes::value_type*>(result.begin()));
        return result;
    }

    // Precondition: type == type::int32
    int32_t as_int32() const {
        auto v = value;
        return static_cast<int32_t>(read_le<uint32_t>(v));
    }

    // Precondition: type == type::int64
    int64_t as_int64() const {
        auto v = value;
        return static_cast<int64_t>(read_le<uint64_t>(v));
    }

    // Precondition: type == type::datetime
    int64_t as_datetime() const { return as_int64(); }

    // Precondition: type == type::timestamp
    uint64_t as_timestamp() const {
        auto v = value;
        return read_le<uint64_t>(v);
    }

    // Precondition: type == type::boolean
    bool as_bool() const {
        return value.current_fragment()[0] != 0;
    }

    // Precondition: type == type::document or type::array
    // Returns a sub-view of the embedded document/array (suitable for
    // constructing another reader).
    View as_document() const { return value; }

    // Precondition: type == type::binary
    // Returns just the binary data (without the length and subtype header).
    View as_binary() const {
        auto v = value;
        auto len = read_le<uint32_t>(v);
        v.remove_prefix(1); // skip subtype byte
        return v.prefix(len);
    }

    // Precondition: type == type::binary
    uint8_t binary_subtype() const {
        auto v = value;
        v.remove_prefix(4); // skip int32 length
        return static_cast<uint8_t>(v.current_fragment()[0]);
    }

    // Precondition: type == type::object_id
    // Returns a 12-byte sub-view.
    View as_object_id() const { return value; }

    // Precondition: type == type::regex
    sstring as_regex_pattern() const {
        auto v = value;
        return read_cstring(v);
    }

    // Precondition: type == type::regex
    sstring as_regex_options() const {
        auto v = value;
        read_cstring(v); // skip pattern
        return read_cstring(v);
    }

    // Precondition: type == type::decimal128
    // Returns a 16-byte sub-view.
    View as_decimal128() const { return value; }
};

// Iterates the elements of a validated BSON document.
// Operates on a FragmentedView cursor without linearizing.
//
// Supports both has_next()/next() polling and range-for via begin()/end().
template <FragmentedView View>
class reader {
    View _cursor;

    // Compute value payload size for the element at the cursor, then return
    // a prefix sub-view of that size and advance the cursor past it.
    static View consume_value(bson::type t, View& cursor) {
        size_t n;
        switch (t) {
        case type::double_value: n = 8; break;
        case type::string:
        case type::javascript: {
            auto peek = cursor;
            n = 4 + read_le<uint32_t>(peek);
            break;
        }
        case type::document:
        case type::array: {
            auto peek = cursor;
            n = read_le<uint32_t>(peek);
            break;
        }
        case type::binary: {
            auto peek = cursor;
            n = 5 + read_le<uint32_t>(peek);
            break;
        }
        case type::object_id: n = 12; break;
        case type::boolean:   n = 1;  break;
        case type::datetime:  n = 8;  break;
        case type::null:      n = 0;  break;
        case type::regex: {
            // Two consecutive c-strings (pattern + options).
            auto scan = cursor;
            n = 0;
            for (int i = 0; i < 2; ++i) {
                for (;;) {
                    auto frag = scan.current_fragment();
                    auto p = std::memchr(frag.data(), 0, frag.size());
                    if (p) {
                        auto skip = static_cast<size_t>(
                            reinterpret_cast<const int8_t*>(p)
                            - frag.data()) + 1;
                        n += skip;
                        scan.remove_prefix(skip);
                        break;
                    }
                    n += frag.size();
                    scan.remove_current();
                }
            }
            break;
        }
        case type::int32:      n = 4;  break;
        case type::timestamp:  n = 8;  break;
        case type::int64:      n = 8;  break;
        case type::decimal128: n = 16; break;
        case type::max_key:
        case type::min_key:    n = 0;  break;
        default: __builtin_unreachable();
        }
        auto val = cursor.prefix(n);
        cursor.remove_prefix(n);
        return val;
    }

public:
    using element_type = element<View>;

    explicit reader(View v) : _cursor(std::move(v)) {
        _cursor.remove_prefix(4); // skip int32 length prefix
    }

    bool has_next() const {
        return _cursor.size_bytes() > 1; // more than just the 0x00 terminator
    }

    element_type next() {
        auto t = static_cast<bson::type>(
            static_cast<uint8_t>(_cursor.current_fragment()[0]));
        _cursor.remove_prefix(1);
        auto k = read_cstring(_cursor);
        auto v = consume_value(t, _cursor);
        return element_type{t, std::move(k), std::move(v)};
    }

    // --- Range-for support ---

    struct sentinel {};

    class iterator {
        reader* _r = nullptr;
        std::optional<element_type> _current;

        void advance() {
            if (_r && _r->has_next()) {
                _current = _r->next();
            } else {
                _current.reset();
            }
        }
    public:
        iterator() = default;
        explicit iterator(reader& r) : _r(&r) { advance(); }

        const element_type& operator*() const { return *_current; }
        const element_type* operator->() const { return &*_current; }

        iterator& operator++() { advance(); return *this; }

        friend bool operator==(const iterator& a, sentinel) {
            return !a._current;
        }
        friend bool operator!=(const iterator& a, sentinel s) {
            return !(a == s);
        }
    };

    iterator begin() { return iterator(*this); }
    sentinel end() const { return {}; }
};

// CTAD: allow bson::reader(some_view) without specifying the template arg.
template <FragmentedView View>
reader(View) -> reader<View>;

// Validate that a byte buffer is a well-formed BSON document.
//
// Checks the envelope (length prefix, terminator) and walks all elements
// to verify type codes are known, key c-strings are null-terminated, and
// value payloads have the correct size for their declared type.  Embedded
// documents and arrays are validated recursively.
//
// Throws marshal_exception on any structural error.
inline void validate(bytes_view bv) {
    if (bv.size() < 5) {
        throw marshal_exception(fmt::format(
            "BSON document too short: {} bytes, minimum is 5", bv.size()));
    }

    uint32_t raw_len;
    std::memcpy(&raw_len, bv.data(), sizeof(raw_len));
    auto doc_size = static_cast<size_t>(seastar::le_to_cpu(raw_len));
    if (doc_size != bv.size()) {
        throw marshal_exception(fmt::format(
            "BSON length prefix {} does not match data size {}",
            doc_size, bv.size()));
    }

    if (static_cast<uint8_t>(bv[bv.size() - 1]) != 0x00) {
        throw marshal_exception("BSON document missing 0x00 terminator");
    }

    const size_t end = bv.size() - 1; // position of the 0x00 terminator
    size_t pos = 4; // skip the length prefix

    auto need = [&](size_t n) {
        if (pos + n > end) {
            throw marshal_exception("truncated BSON element");
        }
    };

    auto read_le_i32 = [&]() -> int32_t {
        need(4);
        uint32_t v;
        std::memcpy(&v, bv.data() + pos, sizeof(v));
        pos += 4;
        return static_cast<int32_t>(seastar::le_to_cpu(v));
    };

    auto skip_cstring = [&] {
        while (pos < end && bv[pos] != 0) {
            ++pos;
        }
        if (pos >= end) {
            throw marshal_exception("unterminated BSON c-string");
        }
        ++pos; // skip null terminator
    };

    while (pos < end) {
        // --- type byte ---
        auto t = static_cast<uint8_t>(bv[pos]);
        ++pos;

        // --- key c-string ---
        skip_cstring();

        // --- value payload (size depends on type) ---
        switch (t) {
        case 0x01: // double
            need(8); pos += 8; break;

        case 0x02: // string
        case 0x0D: // javascript
        {
            auto slen = read_le_i32();
            if (slen < 1) {
                throw marshal_exception("BSON string length must be >= 1");
            }
            need(slen);
            if (static_cast<uint8_t>(bv[pos + slen - 1]) != 0x00) {
                throw marshal_exception("BSON string missing null terminator");
            }
            pos += slen;
            break;
        }

        case 0x03: // embedded document
        case 0x04: // array
        {
            need(4);
            uint32_t dlen_raw;
            std::memcpy(&dlen_raw, bv.data() + pos, sizeof(dlen_raw));
            auto dlen = static_cast<size_t>(seastar::le_to_cpu(dlen_raw));
            if (dlen < 5) {
                throw marshal_exception("embedded BSON document too short");
            }
            need(dlen);
            validate(bytes_view(bv.data() + pos, dlen)); // recurse
            pos += dlen;
            break;
        }

        case 0x05: // binary
        {
            auto blen = read_le_i32();
            if (blen < 0) {
                throw marshal_exception("BSON binary length must be >= 0");
            }
            need(1 + blen); // subtype byte + data
            pos += 1 + blen;
            break;
        }

        case 0x07: // ObjectId
            need(12); pos += 12; break;

        case 0x08: // boolean
        {
            need(1);
            auto b = static_cast<uint8_t>(bv[pos]);
            if (b > 0x01) {
                throw marshal_exception(fmt::format(
                    "BSON boolean must be 0x00 or 0x01, got 0x{:02x}", b));
            }
            pos += 1;
            break;
        }

        case 0x09: // datetime (int64)
            need(8); pos += 8; break;

        case 0x0A: // null
            break; // no payload

        case 0x0B: // regex (two c-strings: pattern + options)
            skip_cstring();
            skip_cstring();
            break;

        case 0x10: // int32
            need(4); pos += 4; break;

        case 0x11: // timestamp (uint64)
            need(8); pos += 8; break;

        case 0x12: // int64
            need(8); pos += 8; break;

        case 0x13: // decimal128
            need(16); pos += 16; break;

        case 0x7F: // max key
        case 0xFF: // min key
            break; // no payload

        default:
            throw marshal_exception(fmt::format(
                "unknown BSON element type 0x{:02x}", t));
        }
    }

    if (pos != end) {
        throw marshal_exception("BSON elements overrun past document terminator");
    }
}

// Construct a document from raw managed_bytes, validating the contents.
// Throws marshal_exception if the bytes are not a valid BSON document.
inline document from_managed_bytes(managed_bytes mb) {
    auto lin = linearized(managed_bytes_view(mb));
    validate(bytes_view(reinterpret_cast<const int8_t*>(lin.data()), lin.size()));
    return document::from_managed_bytes_unsafe(std::move(mb));
}

// Incrementally builds a BSON document using bytes_ostream.
//
// Usage:
//   bson::writer w;
//   w.add_int32("x", 42);
//   w.add_string("name", "hello");
//   bson::document doc = std::move(w).finish();
class writer {
    bytes_ostream _out;
    bytes_ostream::place_holder<uint32_t> _length_ph;

    // Writes the element header: 1-byte type tag + key as a C-string
    // (UTF-8 bytes followed by a 0x00 terminator).
    void write_type_and_key(type t, std::string_view key) {
        auto tag = static_cast<uint8_t>(t);
        _out.write(reinterpret_cast<const char*>(&tag), 1);
        _out.write(key.data(), key.size());
        static constexpr char null_byte = 0;
        _out.write(&null_byte, 1);
    }

    // Writes a little-endian integer to the output stream.
    template <std::integral T>
    void write_le(T value) {
        value = seastar::cpu_to_le(value);
        _out.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }

public:
    writer() : _out(), _length_ph(_out.write_place_holder<uint32_t>()) {}

    void add_double(std::string_view key, double value) {
        write_type_and_key(type::double_value, key);
        auto bits = seastar::cpu_to_le(std::bit_cast<uint64_t>(value));
        _out.write(reinterpret_cast<const char*>(&bits), sizeof(bits));
    }

    void add_string(std::string_view key, std::string_view value) {
        write_type_and_key(type::string, key);
        // BSON string value: int32 byte_count (including trailing 0x00) + UTF-8 bytes + 0x00.
        write_le<uint32_t>(value.size() + 1);
        _out.write(value.data(), value.size());
        static constexpr char null_byte = 0;
        _out.write(&null_byte, 1);
    }

    void add_document(std::string_view key, const document& doc) {
        write_type_and_key(type::document, key);
        _out.write(managed_bytes_view(doc.as_managed_bytes()));
    }

    void add_array(std::string_view key, const document& doc) {
        write_type_and_key(type::array, key);
        _out.write(managed_bytes_view(doc.as_managed_bytes()));
    }

    void add_binary(std::string_view key, bytes_view data, uint8_t subtype = 0x00) {
        write_type_and_key(type::binary, key);
        // BSON binary: int32 byte_count + subtype byte + raw bytes.
        write_le<uint32_t>(data.size());
        _out.write(reinterpret_cast<const char*>(&subtype), 1);
        _out.write(data);
    }

    void add_bool(std::string_view key, bool value) {
        write_type_and_key(type::boolean, key);
        uint8_t b = value ? 0x01 : 0x00;
        _out.write(reinterpret_cast<const char*>(&b), 1);
    }

    void add_null(std::string_view key) {
        write_type_and_key(type::null, key);
        // Null has no value bytes.
    }

    void add_int32(std::string_view key, int32_t value) {
        write_type_and_key(type::int32, key);
        write_le(value);
    }

    void add_int64(std::string_view key, int64_t value) {
        write_type_and_key(type::int64, key);
        write_le(value);
    }

    void add_datetime(std::string_view key, int64_t millis_since_epoch) {
        write_type_and_key(type::datetime, key);
        write_le(millis_since_epoch);
    }

    void add_timestamp(std::string_view key, uint64_t value) {
        write_type_and_key(type::timestamp, key);
        write_le(value);
    }

    // Finalizes the document: writes the 0x00 terminator, fills in
    // the int32 length prefix, and returns the completed document.
    // The writer is consumed (must be called on an rvalue).
    document finish() && {
        // Document terminator.
        static constexpr char null_byte = 0;
        _out.write(&null_byte, 1);

        // Fill in the length prefix (total document size, LE).
        auto size = seastar::cpu_to_le(static_cast<uint32_t>(_out.size()));
        std::memcpy(_length_ph.ptr, &size, sizeof(uint32_t));

        return document::from_managed_bytes_unsafe(std::move(_out).to_managed_bytes());
    }
};

} // namespace bson
