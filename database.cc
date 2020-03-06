#include <atomic>
#include <cstring>
#ifdef SEASTAR_USE_STD_OPTIONAL_VARIANT_STRINGVIEW
#include <optional>
#include <variant>
#else
#endif

#if __cplusplus >= 201703L && __has_include(<filesystem>)
#include <filesystem>
#else
#endif


#if __cplusplus >= 201703L && __has_include(<memory_resource>)

#define SEASTAR_HAS_POLYMORPHIC_ALLOCATOR

#include <memory_resource>

namespace seastar {

/// \cond internal

namespace compat {

using memory_resource = std::pmr::memory_resource;

template<typename T>
using polymorphic_allocator = std::pmr::polymorphic_allocator<T>;

static inline
memory_resource* pmr_get_default_resource() {
    return std::pmr::get_default_resource();
}

}

}

#elif __cplusplus >= 201703L && __has_include(<experimental/memory_resource>)

#define SEASTAR_HAS_POLYMORPHIC_ALLOCATOR


namespace seastar {

/// \cond internal

namespace compat {

using memory_resource = std::experimental::pmr::memory_resource;

template<typename T>
using polymorphic_allocator = std::experimental::pmr::polymorphic_allocator<T>;

static inline
memory_resource* pmr_get_default_resource() {
    return std::experimental::pmr::get_default_resource();
}

}

}

#else

namespace seastar {

/// \cond internal

namespace compat {

// In C++14 we do not intend to support custom allocator, but only to
// allow default allocator to work.
//
// This is done by defining a minimal polymorphic_allocator that always aborts
// and rely on the fact that if the memory allocator is the default allocator,
// we wouldn't actually use it, but would allocate memory ourselves.
//
// Hence C++14 users would be able to use seastar, but not to use a custom allocator

class memory_resource {};

static inline
memory_resource* pmr_get_default_resource() {
    static memory_resource stub;
    return &stub;
}

template <typename T>
class polymorphic_allocator {
public:
    explicit polymorphic_allocator(memory_resource*){}
    T* allocate( std::size_t n ) { __builtin_abort(); }
    void deallocate(T* p, std::size_t n ) { __builtin_abort(); }
};

}

}

#endif

// Defining SEASTAR_ASAN_ENABLED in here is a bit of a hack, but
// convenient since it is build system independent and in practice
// everything includes this header.

#ifndef __has_feature
#define __has_feature(x) 0
#endif

// clang uses __has_feature, gcc defines __SANITIZE_ADDRESS__
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#define SEASTAR_ASAN_ENABLED
#endif

namespace seastar {

/// \cond internal

namespace compat {

#ifdef SEASTAR_USE_STD_OPTIONAL_VARIANT_STRINGVIEW

template <typename T>
using optional = std::optional<T>;

using nullopt_t = std::nullopt_t;

inline constexpr auto nullopt = std::nullopt;

template <typename T>
inline constexpr optional<std::decay_t<T>> make_optional(T&& value) {
    return std::make_optional(std::forward<T>(value));
}

template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string_view = std::basic_string_view<CharT, Traits>;

template <typename CharT, typename Traits = std::char_traits<CharT>>
std::string string_view_to_string(const basic_string_view<CharT, Traits>& v) {
    return std::string(v);
}

template <typename... Types>
using variant = std::variant<Types...>;

template <std::size_t I, typename... Types>
constexpr std::variant_alternative_t<I, variant<Types...>>& get(variant<Types...>& v) {
    return std::get<I>(v);
}

template <std::size_t I, typename... Types>
constexpr const std::variant_alternative_t<I, variant<Types...>>& get(const variant<Types...>& v) {
    return std::get<I>(v);
}

template <std::size_t I, typename... Types>
constexpr std::variant_alternative_t<I, variant<Types...>>&& get(variant<Types...>&& v) {
    return std::get<I>(v);
}

template <std::size_t I, typename... Types>
constexpr const std::variant_alternative_t<I, variant<Types...>>&& get(const variant<Types...>&& v) {
    return std::get<I>(v);
}

template <typename U, typename... Types>
constexpr U& get(variant<Types...>& v) {
    return std::get<U>(v);
}

template <typename U, typename... Types>
constexpr const U& get(const variant<Types...>& v) {
    return std::get<U>(v);
}

template <typename U, typename... Types>
constexpr U&& get(variant<Types...>&& v) {
    return std::get<U>(v);
}

template <typename U, typename... Types>
constexpr const U&& get(const variant<Types...>&& v) {
    return std::get<U>(v);
}

template <typename U, typename... Types>
constexpr U* get_if(variant<Types...>* v) {
    return std::get_if<U>(v);
}

template <typename U, typename... Types>
constexpr const U* get_if(const variant<Types...>* v) {
    return std::get_if<U>(v);
}

#else

template <typename T>
using optional = std::experimental::optional<T>;

using nullopt_t = std::experimental::nullopt_t;

constexpr auto nullopt = std::experimental::nullopt;

template <typename T>
inline constexpr optional<std::decay_t<T>> make_optional(T&& value) {
    return std::experimental::make_optional(std::forward<T>(value));
}

template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string_view = std::experimental::basic_string_view<CharT, Traits>;

template <typename CharT, typename Traits = std::char_traits<CharT>>
std::string string_view_to_string(const basic_string_view<CharT, Traits>& v) {
    return v.to_string();
}

template <typename... Types>
using variant = boost::variant<Types...>;

template<typename U, typename... Types>
U& get(variant<Types...>& v) {
    return boost::get<U, Types...>(v);
}

template<typename U, typename... Types>
U&& get(variant<Types...>&& v) {
    return boost::get<U, Types...>(v);
}

template<typename U, typename... Types>
const U& get(const variant<Types...>& v) {
    return boost::get<U, Types...>(v);
}

template<typename U, typename... Types>
const U&& get(const variant<Types...>&& v) {
    return boost::get<U, Types...>(v);
}

template<typename U, typename... Types>
U* get_if(variant<Types...>* v) {
    return boost::get<U, Types...>(v);
}

template<typename U, typename... Types>
const U* get_if(const variant<Types...>* v) {
    return boost::get<U, Types...>(v);
}

#endif

#if defined(__cpp_lib_filesystem)
namespace filesystem = std::filesystem;
#elif defined(__cpp_lib_experimental_filesystem)
namespace filesystem = std::experimental::filesystem;
#else
#error No filesystem header detected.
#endif

using string_view = basic_string_view<char>;

} // namespace compat

/// \endcond

} // namespace seastar

#if __cplusplus >= 201703L && defined(__cpp_guaranteed_copy_elision)
#define SEASTAR_COPY_ELISION(x) x
#else
#define SEASTAR_COPY_ELISION(x) std::move(x)
#endif

namespace seastar {

/// \addtogroup memory-module
/// @{

/// Provides a mechanism for managing the lifetime of a buffer.
///
/// A \c deleter is an object that is used to inform the consumer
/// of some buffer (not referenced by the deleter itself) how to
/// delete the buffer.  This can be by calling an arbitrary function
/// or destroying an object carried by the deleter.  Examples of
/// a deleter's encapsulated actions are:
///
///  - calling \c std::free(p) on some captured pointer, p
///  - calling \c delete \c p on some captured pointer, p
///  - decrementing a reference count somewhere
///
/// A deleter performs its action from its destructor.
class deleter final {
public:
    /// \cond internal
    struct impl;
    struct raw_object_tag {};
    /// \endcond
private:
    // if bit 0 set, point to object to be freed directly.
    impl* _impl = nullptr;
public:
    /// Constructs an empty deleter that does nothing in its destructor.
    deleter() = default;
    deleter(const deleter&) = delete;
    /// Moves a deleter.
    deleter(deleter&& x) noexcept : _impl(x._impl) { x._impl = nullptr; }
    /// \cond internal
    explicit deleter(impl* i) : _impl(i) {}
    deleter(raw_object_tag tag, void* object)
        : _impl(from_raw_object(object)) {}
    /// \endcond
    /// Destroys the deleter and carries out the encapsulated action.
    ~deleter();
    deleter& operator=(deleter&& x) noexcept;
    deleter& operator=(deleter&) = delete;
    /// Performs a sharing operation.  The encapsulated action will only
    /// be carried out after both the original deleter and the returned
    /// deleter are both destroyed.
    ///
    /// \return a deleter with the same encapsulated action as this one.
    deleter share();
    /// Checks whether the deleter has an associated action.
    explicit operator bool() const { return bool(_impl); }
    /// \cond internal
    void reset(impl* i) {
        this->~deleter();
        new (this) deleter(i);
    }
    /// \endcond
    /// Appends another deleter to this deleter.  When this deleter is
    /// destroyed, both encapsulated actions will be carried out.
    void append(deleter d);
private:
    static bool is_raw_object(impl* i) {
        auto x = reinterpret_cast<uintptr_t>(i);
        return x & 1;
    }
    bool is_raw_object() const {
        return is_raw_object(_impl);
    }
    static void* to_raw_object(impl* i) {
        auto x = reinterpret_cast<uintptr_t>(i);
        return reinterpret_cast<void*>(x & ~uintptr_t(1));
    }
    void* to_raw_object() const {
        return to_raw_object(_impl);
    }
    impl* from_raw_object(void* object) {
        auto x = reinterpret_cast<uintptr_t>(object);
        return reinterpret_cast<impl*>(x | 1);
    }
};

/// \cond internal
struct deleter::impl {
    unsigned refs = 1;
    deleter next;
    impl(deleter next) : next(std::move(next)) {}
    virtual ~impl() {}
};
/// \endcond

inline
deleter::~deleter() {
    if (is_raw_object()) {
        std::free(to_raw_object());
        return;
    }
    if (_impl && --_impl->refs == 0) {
        delete _impl;
    }
}

inline
deleter& deleter::operator=(deleter&& x) noexcept {
    if (this != &x) {
        this->~deleter();
        new (this) deleter(std::move(x));
    }
    return *this;
}

/// \cond internal
template <typename Deleter>
struct lambda_deleter_impl final : deleter::impl {
    Deleter del;
    lambda_deleter_impl(deleter next, Deleter&& del)
        : impl(std::move(next)), del(std::move(del)) {}
    virtual ~lambda_deleter_impl() override { del(); }
};

template <typename Object>
struct object_deleter_impl final : deleter::impl {
    Object obj;
    object_deleter_impl(deleter next, Object&& obj)
        : impl(std::move(next)), obj(std::move(obj)) {}
};

template <typename Object>
inline
object_deleter_impl<Object>* make_object_deleter_impl(deleter next, Object obj) {
    return new object_deleter_impl<Object>(std::move(next), std::move(obj));
}
/// \endcond

/// Makes a \ref deleter that encapsulates the action of
/// destroying an object, as well as running another deleter.  The input
/// object is moved to the deleter, and destroyed when the deleter is destroyed.
///
/// \param d deleter that will become part of the new deleter's encapsulated action
/// \param o object whose destructor becomes part of the new deleter's encapsulated action
/// \related deleter
template <typename Object>
deleter
make_deleter(deleter next, Object o) {
    return deleter(new lambda_deleter_impl<Object>(std::move(next), std::move(o)));
}

/// Makes a \ref deleter that encapsulates the action of destroying an object.  The input
/// object is moved to the deleter, and destroyed when the deleter is destroyed.
///
/// \param o object whose destructor becomes the new deleter's encapsulated action
/// \related deleter
template <typename Object>
deleter
make_deleter(Object o) {
    return make_deleter(deleter(), std::move(o));
}

/// \cond internal
struct free_deleter_impl final : deleter::impl {
    void* obj;
    free_deleter_impl(void* obj) : impl(deleter()), obj(obj) {}
    virtual ~free_deleter_impl() override { std::free(obj); }
};
/// \endcond

inline
deleter
deleter::share() {
    if (!_impl) {
        return deleter();
    }
    if (is_raw_object()) {
        _impl = new free_deleter_impl(to_raw_object());
    }
    ++_impl->refs;
    return deleter(_impl);
}

// Appends 'd' to the chain of deleters. Avoids allocation if possible. For
// performance reasons the current chain should be shorter and 'd' should be
// longer.
inline
void deleter::append(deleter d) {
    if (!d._impl) {
        return;
    }
    impl* next_impl = _impl;
    deleter* next_d = this;
    while (next_impl) {
        if (next_impl == d._impl) {
            return; // Already appended
        }
        if (is_raw_object(next_impl)) {
            next_d->_impl = next_impl = new free_deleter_impl(to_raw_object(next_impl));
        }

        if (next_impl->refs != 1) {
            next_d->_impl = next_impl = make_object_deleter_impl(deleter(next_impl), std::move(d));
            return;
        }

        next_d = &next_impl->next;
        next_impl = next_d->_impl;
    }
    next_d->_impl = d._impl;
    d._impl = nullptr;
}

/// Makes a deleter that calls \c std::free() when it is destroyed.
///
/// \param obj object to free.
/// \related deleter
inline
deleter
make_free_deleter(void* obj) {
    if (!obj) {
        return deleter();
    }
    return deleter(deleter::raw_object_tag(), obj);
}

/// Makes a deleter that calls \c std::free() when it is destroyed, as well
/// as invoking the encapsulated action of another deleter.
///
/// \param d deleter to invoke.
/// \param obj object to free.
/// \related deleter
inline
deleter
make_free_deleter(deleter next, void* obj) {
    return make_deleter(std::move(next), [obj] () mutable { std::free(obj); });
}

/// \see make_deleter(Object)
/// \related deleter
template <typename T>
inline
deleter
make_object_deleter(T&& obj) {
    return deleter{make_object_deleter_impl(deleter(), std::move(obj))};
}

/// \see make_deleter(deleter, Object)
/// \related deleter
template <typename T>
inline
deleter
make_object_deleter(deleter d, T&& obj) {
    return deleter{make_object_deleter_impl(std::move(d), std::move(obj))};
}

/// @}

}

// Workarounds for deficiencies in Eclipse's C++ parser
//
// Tell Eclipse that IN_ECLIPSE is defined so it will ignore all the unknown syntax.

#ifndef IN_ECLIPSE

#else

// Eclipse doesn't grok alignof
#define alignof sizeof

#endif
#include <algorithm>

namespace seastar {

/// \addtogroup memory-module
/// @{

/// Temporary, self-managed byte buffer.
///
/// A \c temporary_buffer is similar to an \c std::string or a \c std::unique_ptr<char[]>,
/// but provides more flexible memory management.  A \c temporary_buffer can own the memory
/// it points to, or it can be shared with another \c temporary_buffer, or point at a substring
/// of a buffer.  It uses a \ref deleter to manage its memory.
///
/// A \c temporary_buffer should not be held indefinitely.  It can be held while a request
/// is processed, or for a similar duration, but not longer, as it can tie up more memory
/// that its size indicates.
///
/// A buffer can be shared: two \c temporary_buffer objects will point to the same data,
/// or a subset of it.  See the \ref temporary_buffer::share() method.
///
/// Unless you created a \c temporary_buffer yourself, do not modify its contents, as they
/// may be shared with another user that does not expect the data to change.
///
/// Use cases for a \c temporary_buffer include:
///  - passing a substring of a tcp packet for the user to consume (zero-copy
///    tcp input)
///  - passing a refcounted blob held in memory to tcp, ensuring that when the TCP ACK
///    is received, the blob is released (by decrementing its reference count) (zero-copy
///    tcp output)
///
/// \tparam CharType underlying character type (must be a variant of \c char).
template <typename CharType>
class temporary_buffer {
    static_assert(sizeof(CharType) == 1, "must buffer stream of bytes");
    CharType* _buffer;
    size_t _size;
    deleter _deleter;
public:
    /// Creates a \c temporary_buffer of a specified size.  The buffer is not shared
    /// with anyone, and is not initialized.
    ///
    /// \param size buffer size, in bytes
    explicit temporary_buffer(size_t size)
        : _buffer(static_cast<CharType*>(malloc(size * sizeof(CharType)))), _size(size)
        , _deleter(make_free_deleter(_buffer)) {
        if (size && !_buffer) {
            throw std::bad_alloc();
        }
    }
    //explicit temporary_buffer(CharType* borrow, size_t size) : _buffer(borrow), _size(size) {}
    /// Creates an empty \c temporary_buffer that does not point at anything.
    temporary_buffer()
        : _buffer(nullptr)
        , _size(0) {}
    temporary_buffer(const temporary_buffer&) = delete;

    /// Moves a \c temporary_buffer.
    temporary_buffer(temporary_buffer&& x) noexcept : _buffer(x._buffer), _size(x._size), _deleter(std::move(x._deleter)) {
        x._buffer = nullptr;
        x._size = 0;
    }

    /// Creates a \c temporary_buffer with a specific deleter.
    ///
    /// \param buf beginning of the buffer held by this \c temporary_buffer
    /// \param size size of the buffer
    /// \param d deleter controlling destruction of the  buffer.  The deleter
    ///          will be destroyed when there are no longer any users for the buffer.
    temporary_buffer(CharType* buf, size_t size, deleter d)
        : _buffer(buf), _size(size), _deleter(std::move(d)) {}
    /// Creates a `temporary_buffer` containing a copy of the provided data
    ///
    /// \param src  data buffer to be copied
    /// \param size size of data buffer in `src`
    temporary_buffer(const CharType* src, size_t size) : temporary_buffer(size) {
        std::copy_n(src, size, _buffer);
    }
    void operator=(const temporary_buffer&) = delete;
    /// Moves a \c temporary_buffer.
    temporary_buffer& operator=(temporary_buffer&& x) noexcept {
        if (this != &x) {
            _buffer = x._buffer;
            _size = x._size;
            _deleter = std::move(x._deleter);
            x._buffer = nullptr;
            x._size = 0;
        }
        return *this;
    }
    /// Gets a pointer to the beginning of the buffer.
    const CharType* get() const { return _buffer; }
    /// Gets a writable pointer to the beginning of the buffer.  Use only
    /// when you are certain no user expects the buffer data not to change.
    CharType* get_write() { return _buffer; }
    /// Gets the buffer size.
    size_t size() const { return _size; }
    /// Gets a pointer to the beginning of the buffer.
    const CharType* begin() const { return _buffer; }
    /// Gets a pointer to the end of the buffer.
    const CharType* end() const { return _buffer + _size; }
    /// Returns the buffer, but with a reduced size.  The original
    /// buffer is consumed by this call and can no longer be used.
    ///
    /// \param size New size; must be smaller than current size.
    /// \return the same buffer, with a prefix removed.
    temporary_buffer prefix(size_t size) && {
        auto ret = std::move(*this);
        ret._size = size;
        return ret;
    }
    /// Reads a character from a specific position in the buffer.
    ///
    /// \param pos position to read character from; must be less than size.
    CharType operator[](size_t pos) const {
        return _buffer[pos];
    }
    /// Checks whether the buffer is empty.
    bool empty() const { return !size(); }
    /// Checks whether the buffer is not empty.
    explicit operator bool() const { return size(); }
    /// Create a new \c temporary_buffer object referring to the same
    /// underlying data.  The underlying \ref deleter will not be destroyed
    /// until both the original and the clone have been destroyed.
    ///
    /// \return a clone of the buffer object.
    temporary_buffer share() {
        return temporary_buffer(_buffer, _size, _deleter.share());
    }
    /// Create a new \c temporary_buffer object referring to a substring of the
    /// same underlying data.  The underlying \ref deleter will not be destroyed
    /// until both the original and the clone have been destroyed.
    ///
    /// \param pos Position of the first character to share.
    /// \param len Length of substring to share.
    /// \return a clone of the buffer object, referring to a substring.
    temporary_buffer share(size_t pos, size_t len) {
        auto ret = share();
        ret._buffer += pos;
        ret._size = len;
        return ret;
    }
    /// Clone the current \c temporary_buffer object into a new one.
    /// This creates a temporary buffer with the same length and data but not
    /// pointing to the memory of the original object.
    temporary_buffer clone() const {
        return {_buffer, _size};
    }
    /// Remove a prefix from the buffer.  The underlying data
    /// is not modified.
    ///
    /// \param pos Position of first character to retain.
    void trim_front(size_t pos) {
        _buffer += pos;
        _size -= pos;
    }
    /// Remove a suffix from the buffer.  The underlying data
    /// is not modified.
    ///
    /// \param pos Position of first character to drop.
    void trim(size_t pos) {
        _size = pos;
    }
    /// Stops automatic memory management.  When the \c temporary_buffer
    /// object is destroyed, the underlying \ref deleter will not be called.
    /// Instead, it is the caller's responsibility to destroy the deleter object
    /// when the data is no longer needed.
    ///
    /// \return \ref deleter object managing the data's lifetime.
    deleter release() {
        return std::move(_deleter);
    }
    /// Creates a \c temporary_buffer object with a specified size, with
    /// memory aligned to a specific boundary.
    ///
    /// \param alignment Required alignment; must be a power of two and a multiple of sizeof(void *).
    /// \param size Required size; must be a multiple of alignment.
    /// \return a new \c temporary_buffer object.
    static temporary_buffer aligned(size_t alignment, size_t size) {
        void *ptr = nullptr;
        auto ret = ::posix_memalign(&ptr, alignment, size * sizeof(CharType));
        auto buf = static_cast<CharType*>(ptr);
        if (ret) {
            throw std::bad_alloc();
        }
        return temporary_buffer(buf, size, make_free_deleter(buf));
    }

    /// Compare contents of this buffer with another buffer for equality
    ///
    /// \param o buffer to compare with
    /// \return true if and only if contents are the same
    bool operator==(const temporary_buffer<char>& o) const {
        return size() == o.size() && std::equal(begin(), end(), o.begin());
    }

    /// Compare contents of this buffer with another buffer for inequality
    ///
    /// \param o buffer to compare with
    /// \return true if and only if contents are not the same
    bool operator!=(const temporary_buffer<char>& o) const {
        return !(*this == o);
    }
};

/// @}

}

namespace seastar {

template <typename char_type, typename Size, Size max_size, bool NulTerminate = true>
class basic_sstring;

using sstring = basic_sstring<char, uint32_t, 15>;

template <typename string_type = sstring, typename T>
inline string_type to_sstring(T value);

template <typename char_type, typename Size, Size max_size, bool NulTerminate>
class basic_sstring {
    static_assert(
            (std::is_same<char_type, char>::value
             || std::is_same<char_type, signed char>::value
             || std::is_same<char_type, unsigned char>::value),
            "basic_sstring only supports single byte char types");
    union contents {
        struct external_type {
            char_type* str;
            Size size;
            int8_t pad;
        } external;
        struct internal_type {
            char_type str[max_size];
            int8_t size;
        } internal;
        static_assert(sizeof(external_type) <= sizeof(internal_type), "max_size too small");
        static_assert(max_size <= 127, "max_size too large");
    } u;
    bool is_internal() const noexcept {
        return u.internal.size >= 0;
    }
    bool is_external() const noexcept {
        return !is_internal();
    }
    const char_type* str() const {
        return is_internal() ? u.internal.str : u.external.str;
    }
    char_type* str() {
        return is_internal() ? u.internal.str : u.external.str;
    }

    template <typename string_type, typename T>
    static inline string_type to_sstring_sprintf(T value, const char* fmt) {
        char tmp[sizeof(value) * 3 + 2];
        auto len = std::sprintf(tmp, fmt, value);
        using ch_type = typename string_type::value_type;
        return string_type(reinterpret_cast<ch_type*>(tmp), len);
    }

    template <typename string_type>
    static inline string_type to_sstring(int value) {
        return to_sstring_sprintf<string_type>(value, "%d");
    }

    template <typename string_type>
    static inline string_type to_sstring(unsigned value) {
        return to_sstring_sprintf<string_type>(value, "%u");
    }

    template <typename string_type>
    static inline string_type to_sstring(long value) {
        return to_sstring_sprintf<string_type>(value, "%ld");
    }

    template <typename string_type>
    static inline string_type to_sstring(unsigned long value) {
        return to_sstring_sprintf<string_type>(value, "%lu");
    }

    template <typename string_type>
    static inline string_type to_sstring(long long value) {
        return to_sstring_sprintf<string_type>(value, "%lld");
    }

    template <typename string_type>
    static inline string_type to_sstring(unsigned long long value) {
        return to_sstring_sprintf<string_type>(value, "%llu");
    }

    template <typename string_type>
    static inline string_type to_sstring(float value) {
        return to_sstring_sprintf<string_type>(value, "%g");
    }

    template <typename string_type>
    static inline string_type to_sstring(double value) {
        return to_sstring_sprintf<string_type>(value, "%g");
    }

    template <typename string_type>
    static inline string_type to_sstring(long double value) {
        return to_sstring_sprintf<string_type>(value, "%Lg");
    }

    template <typename string_type>
    static inline string_type to_sstring(const char* value) {
        return string_type(value);
    }

    template <typename string_type>
    static inline string_type to_sstring(sstring value) {
        return value;
    }

    template <typename string_type>
    static inline string_type to_sstring(const temporary_buffer<char>& buf) {
        return string_type(buf.get(), buf.size());
    }
public:
    using value_type = char_type;
    using traits_type = std::char_traits<char_type>;
    using allocator_type = std::allocator<char_type>;
    using reference = char_type&;
    using const_reference = const char_type&;
    using pointer = char_type*;
    using const_pointer = const char_type*;
    using iterator = char_type*;
    using const_iterator = const char_type*;
    // FIXME: add reverse_iterator and friend
    using difference_type = ssize_t;  // std::make_signed_t<Size> can be too small
    using size_type = Size;
    static constexpr size_type  npos = static_cast<size_type>(-1);
    static constexpr unsigned padding() { return unsigned(NulTerminate); }
public:
    struct initialized_later {};

    basic_sstring() noexcept {
        u.internal.size = 0;
        if (NulTerminate) {
            u.internal.str[0] = '\0';
        }
    }
    basic_sstring(const basic_sstring& x) {
        if (x.is_internal()) {
            u.internal = x.u.internal;
        } else {
            u.internal.size = -1;
            u.external.str = reinterpret_cast<char_type*>(std::malloc(x.u.external.size + padding()));
            if (!u.external.str) {
                throw std::bad_alloc();
            }
            std::copy(x.u.external.str, x.u.external.str + x.u.external.size + padding(), u.external.str);
            u.external.size = x.u.external.size;
        }
    }
    basic_sstring(basic_sstring&& x) noexcept {
#pragma GCC diagnostic push
        // Is a small-string construction is followed by this move constructor, then the trailing bytes
        // of x.u are not initialized, but copied. gcc complains, but it is both legitimate to copy
        // these bytes, and more efficient than a variable-size copy
#pragma GCC diagnostic ignored "-Wuninitialized"
        u = x.u;
#pragma GCC diagnostic pop
        x.u.internal.size = 0;
        x.u.internal.str[0] = '\0';
    }
    basic_sstring(initialized_later, size_t size) {
        if (size_type(size) != size) {
            throw std::overflow_error("sstring overflow");
        }
        if (size + padding() <= sizeof(u.internal.str)) {
            if (NulTerminate) {
                u.internal.str[size] = '\0';
            }
            u.internal.size = size;
        } else {
            u.internal.size = -1;
            u.external.str = reinterpret_cast<char_type*>(std::malloc(size + padding()));
            if (!u.external.str) {
                throw std::bad_alloc();
            }
            u.external.size = size;
            if (NulTerminate) {
                u.external.str[size] = '\0';
            }
        }
    }
    basic_sstring(const char_type* x, size_t size) {
        if (size_type(size) != size) {
            throw std::overflow_error("sstring overflow");
        }
        if (size + padding() <= sizeof(u.internal.str)) {
            std::copy(x, x + size, u.internal.str);
            if (NulTerminate) {
                u.internal.str[size] = '\0';
            }
            u.internal.size = size;
        } else {
            u.internal.size = -1;
            u.external.str = reinterpret_cast<char_type*>(std::malloc(size + padding()));
            if (!u.external.str) {
                throw std::bad_alloc();
            }
            u.external.size = size;
            std::copy(x, x + size, u.external.str);
            if (NulTerminate) {
                u.external.str[size] = '\0';
            }
        }
    }

    basic_sstring(size_t size, char_type x) : basic_sstring(initialized_later(), size) {
        memset(begin(), x, size);
    }

    basic_sstring(const char* x) : basic_sstring(reinterpret_cast<const char_type*>(x), std::strlen(x)) {}
    basic_sstring(std::basic_string<char_type>& x) : basic_sstring(x.c_str(), x.size()) {}
    basic_sstring(std::initializer_list<char_type> x) : basic_sstring(x.begin(), x.end() - x.begin()) {}
    basic_sstring(const char_type* b, const char_type* e) : basic_sstring(b, e - b) {}
    basic_sstring(const std::basic_string<char_type>& s)
        : basic_sstring(s.data(), s.size()) {}
    template <typename InputIterator>
    basic_sstring(InputIterator first, InputIterator last)
            : basic_sstring(initialized_later(), std::distance(first, last)) {
        std::copy(first, last, begin());
    }
    explicit basic_sstring(compat::basic_string_view<char_type, traits_type> v)
            : basic_sstring(v.data(), v.size()) {
    }
    ~basic_sstring() noexcept {
        if (is_external()) {
            std::free(u.external.str);
        }
    }
    basic_sstring& operator=(const basic_sstring& x) {
        basic_sstring tmp(x);
        swap(tmp);
        return *this;
    }
    basic_sstring& operator=(basic_sstring&& x) noexcept {
        if (this != &x) {
            swap(x);
            x.reset();
        }
        return *this;
    }
    operator std::basic_string<char_type>() const {
        return { str(), size() };
    }

    size_t size() const noexcept {
        return is_internal() ? u.internal.size : u.external.size;
    }

    size_t length() const noexcept {
        return size();
    }

    size_t find(char_type t, size_t pos = 0) const noexcept {
        const char_type* it = str() + pos;
        const char_type* end = str() + size();
        while (it < end) {
            if (*it == t) {
                return it - str();
            }
            it++;
        }
        return npos;
    }

    size_t find(const basic_sstring& s, size_t pos = 0) const noexcept {
        const char_type* it = str() + pos;
        const char_type* end = str() + size();
        const char_type* c_str = s.str();
        const char_type* c_str_end = s.str() + s.size();

        while (it < end) {
            auto i = it;
            auto j = c_str;
            while ( i < end && j < c_str_end && *i == *j) {
                i++;
                j++;
            }
            if (j == c_str_end) {
                return it - str();
            }
            it++;
        }
        return npos;
    }

    /**
     * find_last_of find the last occurrence of c in the string.
     * When pos is specified, the search only includes characters
     * at or before position pos.
     *
     */
    size_t find_last_of (char_type c, size_t pos = npos) const noexcept {
        const char_type* str_start = str();
        if (size()) {
            if (pos >= size()) {
                pos = size() - 1;
            }
            const char_type* p = str_start + pos + 1;
            do {
                p--;
                if (*p == c) {
                    return (p - str_start);
                }
            } while (p != str_start);
        }
        return npos;
    }

    /**
     *  Append a C substring.
     *  @param s  The C string to append.
     *  @param n  The number of characters to append.
     *  @return  Reference to this string.
     */
    basic_sstring& append (const char_type* s, size_t n) {
        basic_sstring ret(initialized_later(), size() + n);
        std::copy(begin(), end(), ret.begin());
        std::copy(s, s + n, ret.begin() + size());
        *this = std::move(ret);
        return *this;
    }

    /**
     *  Resize string.
     *  @param n  new size.
     *  @param c  if n greater than current size character to fill newly allocated space with.
     */
    void resize(size_t n, const char_type c  = '\0') {
        if (n > size()) {
            *this += basic_sstring(n - size(), c);
        } else if (n < size()) {
            if (is_internal()) {
                u.internal.size = n;
                if (NulTerminate) {
                    u.internal.str[n] = '\0';
                }
            } else if (n + padding() <= sizeof(u.internal.str)) {
                *this = basic_sstring(u.external.str, n);
            } else {
                u.external.size = n;
                if (NulTerminate) {
                    u.external.str[n] = '\0';
                }
            }
        }
    }

    /**
     *  Replace characters with a value of a C style substring.
     *
     */
    basic_sstring& replace(size_type pos, size_type n1, const char_type* s,
             size_type n2) {
        if (pos > size()) {
            throw std::out_of_range("sstring::replace out of range");
        }

        if (n1 > size() - pos) {
            n1 = size() - pos;
        }

        if (n1 == n2) {
            if (n2) {
                std::copy(s, s + n2, begin() + pos);
            }
            return *this;
        }
        basic_sstring ret(initialized_later(), size() + n2 - n1);
        char_type* p= ret.begin();
        std::copy(begin(), begin() + pos, p);
        p += pos;
        if (n2) {
            std::copy(s, s + n2, p);
        }
        p += n2;
        std::copy(begin() + pos + n1, end(), p);
        *this = std::move(ret);
        return *this;
    }

    template <class InputIterator>
    basic_sstring& replace (const_iterator i1, const_iterator i2,
            InputIterator first, InputIterator last) {
        if (i1 < begin() || i1 > end() || i2 < begin()) {
            throw std::out_of_range("sstring::replace out of range");
        }
        if (i2 > end()) {
            i2 = end();
        }

        if (i2 - i1 == last - first) {
            //in place replacement
            std::copy(first, last, const_cast<char_type*>(i1));
            return *this;
        }
        basic_sstring ret(initialized_later(), size() + (last - first) - (i2 - i1));
        char_type* p = ret.begin();
        p = std::copy(cbegin(), i1, p);
        p = std::copy(first, last, p);
        std::copy(i2, cend(), p);
        *this = std::move(ret);
        return *this;
    }

    iterator erase(iterator first, iterator last) {
        size_t pos = first - begin();
        replace(pos, last - first, nullptr, 0);
        return begin() + pos;
    }

    /**
     * Inserts additional characters into the string right before
     * the character indicated by p.
     */
    template <class InputIterator>
    void insert(const_iterator p, InputIterator beg, InputIterator end) {
        replace(p, p, beg, end);
    }

    /**
     *  Returns a read/write reference to the data at the last
     *  element of the string.
     *  This function shall not be called on empty strings.
     */
    reference
    back() noexcept {
        return operator[](size() - 1);
    }

    /**
     *  Returns a  read-only (constant) reference to the data at the last
     *  element of the string.
     *  This function shall not be called on empty strings.
     */
    const_reference
    back() const noexcept {
        return operator[](size() - 1);
    }

    basic_sstring substr(size_t from, size_t len = npos)  const {
        if (from > size()) {
            throw std::out_of_range("sstring::substr out of range");
        }
        if (len > size() - from) {
            len = size() - from;
        }
        if (len == 0) {
            return "";
        }
        return { str() + from , len };
    }

    const char_type& at(size_t pos) const {
        if (pos >= size()) {
            throw std::out_of_range("sstring::at out of range");
        }
        return *(str() + pos);
    }

    char_type& at(size_t pos) {
        if (pos >= size()) {
            throw std::out_of_range("sstring::at out of range");
        }
        return *(str() + pos);
    }

    bool empty() const noexcept {
        return u.internal.size == 0;
    }
    void reset() noexcept {
        if (is_external()) {
            std::free(u.external.str);
        }
        u.internal.size = 0;
        if (NulTerminate) {
            u.internal.str[0] = '\0';
        }
    }
    temporary_buffer<char_type> release() && {
        if (is_external()) {
            auto ptr = u.external.str;
            auto size = u.external.size;
            u.external.str = nullptr;
            u.external.size = 0;
            return temporary_buffer<char_type>(ptr, size, make_free_deleter(ptr));
        } else {
            auto buf = temporary_buffer<char_type>(u.internal.size);
            std::copy(u.internal.str, u.internal.str + u.internal.size, buf.get_write());
            u.internal.size = 0;
            if (NulTerminate) {
                u.internal.str[0] = '\0';
            }
            return buf;
        }
    }
    int compare(const basic_sstring& x) const noexcept {
        auto n = traits_type::compare(begin(), x.begin(), std::min(size(), x.size()));
        if (n != 0) {
            return n;
        }
        if (size() < x.size()) {
            return -1;
        } else if (size() > x.size()) {
            return 1;
        } else {
            return 0;
        }
    }

    int compare(size_t pos, size_t sz, const basic_sstring& x) const {
        if (pos > size()) {
            throw std::out_of_range("pos larger than string size");
        }

        sz = std::min(size() - pos, sz);
        auto n = traits_type::compare(begin() + pos, x.begin(), std::min(sz, x.size()));
        if (n != 0) {
            return n;
        }
        if (sz < x.size()) {
            return -1;
        } else if (sz > x.size()) {
            return 1;
        } else {
            return 0;
        }
    }

    void swap(basic_sstring& x) noexcept {
        contents tmp;
        tmp = x.u;
        x.u = u;
        u = tmp;
    }
    char_type* data() {
        return str();
    }
    const char_type* data() const {
        return str();
    }
    const char_type* c_str() const {
        return str();
    }
    const char_type* begin() const { return str(); }
    const char_type* end() const { return str() + size(); }
    const char_type* cbegin() const { return str(); }
    const char_type* cend() const { return str() + size(); }
    char_type* begin() { return str(); }
    char_type* end() { return str() + size(); }
    bool operator==(const basic_sstring& x) const {
        return size() == x.size() && std::equal(begin(), end(), x.begin());
    }
    bool operator!=(const basic_sstring& x) const {
        return !operator==(x);
    }
    bool operator<(const basic_sstring& x) const {
        return compare(x) < 0;
    }
    basic_sstring operator+(const basic_sstring& x) const {
        basic_sstring ret(initialized_later(), size() + x.size());
        std::copy(begin(), end(), ret.begin());
        std::copy(x.begin(), x.end(), ret.begin() + size());
        return ret;
    }
    basic_sstring& operator+=(const basic_sstring& x) {
        return *this = *this + x;
    }
    char_type& operator[](size_type pos) {
        return str()[pos];
    }
    const char_type& operator[](size_type pos) const {
        return str()[pos];
    }

    operator compat::basic_string_view<char_type>() const {
        return compat::basic_string_view<char_type>(str(), size());
    }

    template <typename string_type, typename T>
    friend inline string_type to_sstring(T value);
};
template <typename char_type, typename Size, Size max_size, bool NulTerminate>
constexpr Size basic_sstring<char_type, Size, max_size, NulTerminate>::npos;

template <typename char_type, typename size_type, size_type Max, size_type N, bool NulTerminate>
inline
basic_sstring<char_type, size_type, Max, NulTerminate>
operator+(const char(&s)[N], const basic_sstring<char_type, size_type, Max, NulTerminate>& t) {
    using sstring = basic_sstring<char_type, size_type, Max, NulTerminate>;
    // don't copy the terminating NUL character
    sstring ret(typename sstring::initialized_later(), N-1 + t.size());
    auto p = std::copy(std::begin(s), std::end(s)-1, ret.begin());
    std::copy(t.begin(), t.end(), p);
    return ret;
}

template <size_t N>
static inline
size_t str_len(const char(&s)[N]) { return N - 1; }

template <size_t N>
static inline
const char* str_begin(const char(&s)[N]) { return s; }

template <size_t N>
static inline
const char* str_end(const char(&s)[N]) { return str_begin(s) + str_len(s); }

template <typename char_type, typename size_type, size_type max_size, bool NulTerminate>
static inline
const char_type* str_begin(const basic_sstring<char_type, size_type, max_size, NulTerminate>& s) { return s.begin(); }

template <typename char_type, typename size_type, size_type max_size, bool NulTerminate>
static inline
const char_type* str_end(const basic_sstring<char_type, size_type, max_size, NulTerminate>& s) { return s.end(); }

template <typename char_type, typename size_type, size_type max_size, bool NulTerminate>
static inline
size_type str_len(const basic_sstring<char_type, size_type, max_size, NulTerminate>& s) { return s.size(); }

template <typename First, typename Second, typename... Tail>
static inline
size_t str_len(const First& first, const Second& second, const Tail&... tail) {
    return str_len(first) + str_len(second, tail...);
}

template <typename char_type, typename size_type, size_type max_size>
inline
void swap(basic_sstring<char_type, size_type, max_size>& x,
          basic_sstring<char_type, size_type, max_size>& y) noexcept
{
    return x.swap(y);
}

template <typename char_type, typename size_type, size_type max_size, bool NulTerminate, typename char_traits>
inline
std::basic_ostream<char_type, char_traits>&
operator<<(std::basic_ostream<char_type, char_traits>& os,
        const basic_sstring<char_type, size_type, max_size, NulTerminate>& s) {
    return os.write(s.begin(), s.size());
}

template <typename char_type, typename size_type, size_type max_size, bool NulTerminate, typename char_traits>
inline
std::basic_istream<char_type, char_traits>&
operator>>(std::basic_istream<char_type, char_traits>& is,
        basic_sstring<char_type, size_type, max_size, NulTerminate>& s) {
    std::string tmp;
    is >> tmp;
    s = tmp;
    return is;
}

}

namespace std {

template <typename char_type, typename size_type, size_type max_size, bool NulTerminate>
struct hash<seastar::basic_sstring<char_type, size_type, max_size, NulTerminate>> {
    size_t operator()(const seastar::basic_sstring<char_type, size_type, max_size, NulTerminate>& s) const {
        return std::hash<seastar::compat::basic_string_view<char_type>>()(s);
    }
};

}

namespace seastar {

static inline
char* copy_str_to(char* dst) {
    return dst;
}

template <typename Head, typename... Tail>
static inline
char* copy_str_to(char* dst, const Head& head, const Tail&... tail) {
    return copy_str_to(std::copy(str_begin(head), str_end(head), dst), tail...);
}

template <typename String = sstring, typename... Args>
static String make_sstring(Args&&... args)
{
    String ret(sstring::initialized_later(), str_len(args...));
    copy_str_to(ret.begin(), args...);
    return ret;
}

template <typename string_type, typename T>
inline string_type to_sstring(T value) {
    return sstring::to_sstring<string_type>(value);
}

}

namespace std {
template <typename T>
inline
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    bool first = true;
    os << "{";
    for (auto&& elem : v) {
        if (!first) {
            os << ", ";
        } else {
            first = false;
        }
        os << elem;
    }
    os << "}";
    return os;
}

template <typename Key, typename T, typename Hash, typename KeyEqual, typename Allocator>
std::ostream& operator<<(std::ostream& os, const std::unordered_map<Key, T, Hash, KeyEqual, Allocator>& v) {
    bool first = true;
    os << "{";
    for (auto&& elem : v) {
        if (!first) {
            os << ", ";
        } else {
            first = false;
        }
        os << "{ " << elem.first << " -> " << elem.second << "}";
    }
    os << "}";
    return os;
}
}
using namespace seastar;
#ifndef SEASTAR_HAVE_GCC6_CONCEPTS

#define GCC6_CONCEPT(x...)
#define GCC6_NO_CONCEPT(x...) x

#else

#define GCC6_CONCEPT(x...) x
#define GCC6_NO_CONCEPT(x...)

#endif
GCC6_CONCEPT(template <typename H> concept bool Hasher() {
  return requires(H & h, const char *ptr, size_t size) {
    { h.update(ptr, size) }
    ->void;
  };
})
template <typename T, typename Enable = void>
struct appending_hash;

namespace seastar {

template<typename T>
struct is_smart_ptr : std::false_type {};

template<typename T>
struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};

}

namespace seastar {

// This header defines functors for comparing and hashing pointers by pointed-to values instead of pointer addresses.
//
// Examples:
//
//  std::multiset<shared_ptr<sstring>, indirect_less<shared_ptr<sstring>>> _multiset;
//
//  std::unordered_map<shared_ptr<sstring>, bool,
//      indirect_hash<shared_ptr<sstring>>, indirect_equal_to<shared_ptr<sstring>>> _unordered_map;
//

template<typename Pointer, typename Equal = std::equal_to<typename std::pointer_traits<Pointer>::element_type>>
struct indirect_equal_to {
    Equal _eq;
    indirect_equal_to(Equal eq = Equal()) : _eq(std::move(eq)) {}
    bool operator()(const Pointer& i1, const Pointer& i2) const {
        if (bool(i1) ^ bool(i2)) {
            return false;
        }
        return !i1 || _eq(*i1, *i2);
    }
};

template<typename Pointer, typename Less = std::less<typename std::pointer_traits<Pointer>::element_type>>
struct indirect_less {
    Less _cmp;
    indirect_less(Less cmp = Less()) : _cmp(std::move(cmp)) {}
    bool operator()(const Pointer& i1, const Pointer& i2) const {
        if (i1 && i2) {
            return _cmp(*i1, *i2);
        }
        return !i1 && i2;
    }
};

template<typename Pointer, typename Hash = std::hash<typename std::pointer_traits<Pointer>::element_type>>
struct indirect_hash {
    Hash _h;
    indirect_hash(Hash h = Hash()) : _h(std::move(h)) {}
    size_t operator()(const Pointer& p) const {
        if (p) {
            return _h(*p);
        }
        return 0;
    }
};

}

#include <boost/intrusive/parent_from_member.hpp>

namespace seastar {

// This header defines two shared pointer facilities, lw_shared_ptr<> and
// shared_ptr<>, both modeled after std::shared_ptr<>.
//
// Unlike std::shared_ptr<>, neither of these implementations are thread
// safe, and two pointers sharing the same object must not be used in
// different threads.
//
// lw_shared_ptr<> is the more lightweight variant, with a lw_shared_ptr<>
// occupying just one machine word, and adding just one word to the shared
// object.  However, it does not support polymorphism.
//
// shared_ptr<> is more expensive, with a pointer occupying two machine
// words, and with two words of overhead in the shared object.  In return,
// it does support polymorphism.
//
// Both variants support shared_from_this() via enable_shared_from_this<>
// and lw_enable_shared_from_this<>().
//

#ifndef SEASTAR_DEBUG_SHARED_PTR
using shared_ptr_counter_type = long;
#else
using shared_ptr_counter_type = debug_shared_ptr_counter_type;
#endif

template <typename T>
class lw_shared_ptr;

template <typename T>
class shared_ptr;

template <typename T>
class enable_lw_shared_from_this;

template <typename T>
class enable_shared_from_this;

template <typename T, typename... A>
lw_shared_ptr<T> make_lw_shared(A&&... a);

template <typename T>
lw_shared_ptr<T> make_lw_shared(T&& a);

template <typename T>
lw_shared_ptr<T> make_lw_shared(T& a);

template <typename T, typename... A>
shared_ptr<T> make_shared(A&&... a);

template <typename T>
shared_ptr<T> make_shared(T&& a);

template <typename T, typename U>
shared_ptr<T> static_pointer_cast(const shared_ptr<U>& p);

template <typename T, typename U>
shared_ptr<T> dynamic_pointer_cast(const shared_ptr<U>& p);

template <typename T, typename U>
shared_ptr<T> const_pointer_cast(const shared_ptr<U>& p);

struct lw_shared_ptr_counter_base {
    shared_ptr_counter_type _count = 0;
};


namespace internal {

template <class T, class U>
struct lw_shared_ptr_accessors;

template <class T>
struct lw_shared_ptr_accessors_esft;

template <class T>
struct lw_shared_ptr_accessors_no_esft;

}


// We want to support two use cases for shared_ptr<T>:
//
//   1. T is any type (primitive or class type)
//
//   2. T is a class type that inherits from enable_shared_from_this<T>.
//
// In the first case, we must wrap T in an object containing the counter,
// since T may be a primitive type and cannot be a base class.
//
// In the second case, we want T to reach the counter through its
// enable_shared_from_this<> base class, so that we can implement
// shared_from_this().
//
// To implement those two conflicting requirements (T alongside its counter;
// T inherits from an object containing the counter) we use std::conditional<>
// and some accessor functions to select between two implementations.


// CRTP from this to enable shared_from_this:
template <typename T>
class enable_lw_shared_from_this : private lw_shared_ptr_counter_base {
    using ctor = T;
protected:
    enable_lw_shared_from_this() noexcept {}
    enable_lw_shared_from_this(enable_lw_shared_from_this&&) noexcept {}
    enable_lw_shared_from_this(const enable_lw_shared_from_this&) noexcept {}
    enable_lw_shared_from_this& operator=(const enable_lw_shared_from_this&) noexcept { return *this; }
    enable_lw_shared_from_this& operator=(enable_lw_shared_from_this&&) noexcept { return *this; }
public:
    lw_shared_ptr<T> shared_from_this();
    lw_shared_ptr<const T> shared_from_this() const;

    template <typename X>
    friend class lw_shared_ptr;
    template <typename X>
    friend struct internal::lw_shared_ptr_accessors_esft;
    template <typename X, class Y>
    friend struct internal::lw_shared_ptr_accessors;
};

template <typename T>
struct shared_ptr_no_esft : private lw_shared_ptr_counter_base {
    T _value;

    shared_ptr_no_esft() = default;
    shared_ptr_no_esft(const T& x) : _value(x) {}
    shared_ptr_no_esft(T&& x) : _value(std::move(x)) {}
    template <typename... A>
    shared_ptr_no_esft(A&&... a) : _value(std::forward<A>(a)...) {}

    template <typename X>
    friend class lw_shared_ptr;
    template <typename X>
    friend struct internal::lw_shared_ptr_accessors_no_esft;
    template <typename X, class Y>
    friend struct internal::lw_shared_ptr_accessors;
};


/// Extension point: the user may override this to change how \ref lw_shared_ptr objects are destroyed,
/// primarily so that incomplete classes can be used.
///
/// Customizing the deleter requires that \c T be derived from \c enable_lw_shared_from_this<T>.
/// The specialization must be visible for all uses of \c lw_shared_ptr<T>.
///
/// To customize, the template must have a `static void dispose(T*)` operator that disposes of
/// the object.
template <typename T>
struct lw_shared_ptr_deleter;  // No generic implementation

namespace internal {

template <typename T>
struct lw_shared_ptr_accessors_esft {
    using concrete_type = std::remove_const_t<T>;
    static T* to_value(lw_shared_ptr_counter_base* counter) {
        return static_cast<T*>(counter);
    }
    static void dispose(lw_shared_ptr_counter_base* counter) {
        dispose(static_cast<T*>(counter));
    }
    static void dispose(T* value_ptr) {
        delete value_ptr;
    }
    static void instantiate_to_value(lw_shared_ptr_counter_base* p) {
        // since to_value() is defined above, we don't need to do anything special
        // to force-instantiate it
    }
};

template <typename T>
struct lw_shared_ptr_accessors_no_esft {
    using concrete_type = shared_ptr_no_esft<T>;
    static T* to_value(lw_shared_ptr_counter_base* counter) {
        return &static_cast<concrete_type*>(counter)->_value;
    }
    static void dispose(lw_shared_ptr_counter_base* counter) {
        delete static_cast<concrete_type*>(counter);
    }
    static void dispose(T* value_ptr) {
        delete boost::intrusive::get_parent_from_member(value_ptr, &concrete_type::_value);
    }
    static void instantiate_to_value(lw_shared_ptr_counter_base* p) {
        // since to_value() is defined above, we don't need to do anything special
        // to force-instantiate it
    }
};

// Generic case: lw_shared_ptr_deleter<T> is not specialized, select
// implementation based on whether T inherits from enable_lw_shared_from_this<T>.
template <typename T, typename U = void>
struct lw_shared_ptr_accessors : std::conditional_t<
         std::is_base_of<enable_lw_shared_from_this<T>, T>::value,
         lw_shared_ptr_accessors_esft<T>,
         lw_shared_ptr_accessors_no_esft<T>> {
};

// void_t is C++17, use this temporarily
template <typename... T>
using void_t = void;

// Overload when lw_shared_ptr_deleter<T> specialized
template <typename T>
struct lw_shared_ptr_accessors<T, void_t<decltype(lw_shared_ptr_deleter<T>{})>> {
    using concrete_type = T;
    static T* to_value(lw_shared_ptr_counter_base* counter);
    static void dispose(lw_shared_ptr_counter_base* counter) {
        lw_shared_ptr_deleter<T>::dispose(to_value(counter));
    }
    static void instantiate_to_value(lw_shared_ptr_counter_base* p) {
        // instantiate to_value(); must be defined by shared_ptr_incomplete.hh
        to_value(p);
    }
};

}

template <typename T>
class lw_shared_ptr {
    using accessors = internal::lw_shared_ptr_accessors<std::remove_const_t<T>>;
    using concrete_type = typename accessors::concrete_type;
    mutable lw_shared_ptr_counter_base* _p = nullptr;
private:
    lw_shared_ptr(lw_shared_ptr_counter_base* p) noexcept : _p(p) {
        if (_p) {
            ++_p->_count;
        }
    }
    template <typename... A>
    static lw_shared_ptr make(A&&... a) {
        auto p = new concrete_type(std::forward<A>(a)...);
        accessors::instantiate_to_value(p);
        return lw_shared_ptr(p);
    }
public:
    using element_type = T;

    // Destroys the object pointed to by p and disposes of its storage.
    // The pointer to the object must have been obtained through release().
    static void dispose(T* p) noexcept {
        accessors::dispose(const_cast<std::remove_const_t<T>*>(p));
    }

    // A functor which calls dispose().
    class disposer {
    public:
        void operator()(T* p) const noexcept {
            dispose(p);
        }
    };

    lw_shared_ptr() noexcept = default;
    lw_shared_ptr(std::nullptr_t) noexcept : lw_shared_ptr() {}
    lw_shared_ptr(const lw_shared_ptr& x) noexcept : _p(x._p) {
        if (_p) {
            ++_p->_count;
        }
    }
    lw_shared_ptr(lw_shared_ptr&& x) noexcept  : _p(x._p) {
        x._p = nullptr;
    }
    [[gnu::always_inline]]
    ~lw_shared_ptr() {
        if (_p && !--_p->_count) {
            accessors::dispose(_p);
        }
    }
    lw_shared_ptr& operator=(const lw_shared_ptr& x) noexcept {
        if (_p != x._p) {
            this->~lw_shared_ptr();
            new (this) lw_shared_ptr(x);
        }
        return *this;
    }
    lw_shared_ptr& operator=(lw_shared_ptr&& x) noexcept {
        if (_p != x._p) {
            this->~lw_shared_ptr();
            new (this) lw_shared_ptr(std::move(x));
        }
        return *this;
    }
    lw_shared_ptr& operator=(std::nullptr_t) noexcept {
        return *this = lw_shared_ptr();
    }
    lw_shared_ptr& operator=(T&& x) noexcept {
        this->~lw_shared_ptr();
        new (this) lw_shared_ptr(make_lw_shared<T>(std::move(x)));
        return *this;
    }

    T& operator*() const noexcept { return *accessors::to_value(_p); }
    T* operator->() const noexcept { return accessors::to_value(_p); }
    T* get() const noexcept {
        if (_p) {
            return accessors::to_value(_p);
        } else {
            return nullptr;
        }
    }

    // Releases ownership of the object without destroying it.
    // If this was the last owner then returns an engaged unique_ptr
    // which is now the sole owner of the object.
    // Returns a disengaged pointer if there are still some owners.
    //
    // Note that in case the raw pointer is extracted from the unique_ptr
    // using unique_ptr::release(), it must be still destroyed using
    // lw_shared_ptr::disposer or lw_shared_ptr::dispose().
    std::unique_ptr<T, disposer> release() noexcept {
        auto p = std::exchange(_p, nullptr);
        if (--p->_count) {
            return nullptr;
        } else {
            return std::unique_ptr<T, disposer>(accessors::to_value(p));
        }
    }

    long int use_count() const noexcept {
        if (_p) {
            return _p->_count;
        } else {
            return 0;
        }
    }

    operator lw_shared_ptr<const T>() const noexcept {
        return lw_shared_ptr<const T>(_p);
    }

    explicit operator bool() const noexcept {
        return _p;
    }

    bool owned() const noexcept {
        return _p->_count == 1;
    }

    bool operator==(const lw_shared_ptr<const T>& x) const {
        return _p == x._p;
    }

    bool operator!=(const lw_shared_ptr<const T>& x) const {
        return !operator==(x);
    }

    bool operator==(const lw_shared_ptr<std::remove_const_t<T>>& x) const {
        return _p == x._p;
    }

    bool operator!=(const lw_shared_ptr<std::remove_const_t<T>>& x) const {
        return !operator==(x);
    }

    bool operator<(const lw_shared_ptr<const T>& x) const {
        return _p < x._p;
    }

    bool operator<(const lw_shared_ptr<std::remove_const_t<T>>& x) const {
        return _p < x._p;
    }

    template <typename U>
    friend class lw_shared_ptr;

    template <typename X, typename... A>
    friend lw_shared_ptr<X> make_lw_shared(A&&...);

    template <typename U>
    friend lw_shared_ptr<U> make_lw_shared(U&&);

    template <typename U>
    friend lw_shared_ptr<U> make_lw_shared(U&);

    template <typename U>
    friend class enable_lw_shared_from_this;
};

template <typename T, typename... A>
inline
lw_shared_ptr<T> make_lw_shared(A&&... a) {
    return lw_shared_ptr<T>::make(std::forward<A>(a)...);
}

template <typename T>
inline
lw_shared_ptr<T> make_lw_shared(T&& a) {
    return lw_shared_ptr<T>::make(std::move(a));
}

template <typename T>
inline
lw_shared_ptr<T> make_lw_shared(T& a) {
    return lw_shared_ptr<T>::make(a);
}

template <typename T>
inline
lw_shared_ptr<T>
enable_lw_shared_from_this<T>::shared_from_this() {
    return lw_shared_ptr<T>(this);
}

template <typename T>
inline
lw_shared_ptr<const T>
enable_lw_shared_from_this<T>::shared_from_this() const {
    return lw_shared_ptr<const T>(const_cast<enable_lw_shared_from_this*>(this));
}

template <typename T>
static inline
std::ostream& operator<<(std::ostream& out, const lw_shared_ptr<T>& p) {
    if (!p) {
        return out << "null";
    }
    return out << *p;
}

// Polymorphic shared pointer class

struct shared_ptr_count_base {
    // destructor is responsible for fully-typed deletion
    virtual ~shared_ptr_count_base() {}
    shared_ptr_counter_type count = 0;
};

template <typename T>
struct shared_ptr_count_for : shared_ptr_count_base {
    T data;
    template <typename... A>
    shared_ptr_count_for(A&&... a) : data(std::forward<A>(a)...) {}
};

template <typename T>
class enable_shared_from_this : private shared_ptr_count_base {
public:
    shared_ptr<T> shared_from_this();
    shared_ptr<const T> shared_from_this() const;

    template <typename U>
    friend class shared_ptr;

    template <typename U, bool esft>
    friend struct shared_ptr_make_helper;
};

template <typename T>
class shared_ptr {
    mutable shared_ptr_count_base* _b = nullptr;
    mutable T* _p = nullptr;
private:
    explicit shared_ptr(shared_ptr_count_for<T>* b) noexcept : _b(b), _p(&b->data) {
        ++_b->count;
    }
    shared_ptr(shared_ptr_count_base* b, T* p) noexcept : _b(b), _p(p) {
        if (_b) {
            ++_b->count;
        }
    }
    explicit shared_ptr(enable_shared_from_this<std::remove_const_t<T>>* p) noexcept : _b(p), _p(static_cast<T*>(p)) {
        if (_b) {
            ++_b->count;
        }
    }
public:
    using element_type = T;

    shared_ptr() noexcept = default;
    shared_ptr(std::nullptr_t) noexcept : shared_ptr() {}
    shared_ptr(const shared_ptr& x) noexcept
            : _b(x._b)
            , _p(x._p) {
        if (_b) {
            ++_b->count;
        }
    }
    shared_ptr(shared_ptr&& x) noexcept
            : _b(x._b)
            , _p(x._p) {
        x._b = nullptr;
        x._p = nullptr;
    }
    template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
    shared_ptr(const shared_ptr<U>& x) noexcept
            : _b(x._b)
            , _p(x._p) {
        if (_b) {
            ++_b->count;
        }
    }
    template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
    shared_ptr(shared_ptr<U>&& x) noexcept
            : _b(x._b)
            , _p(x._p) {
        x._b = nullptr;
        x._p = nullptr;
    }
    ~shared_ptr() {
        if (_b && !--_b->count) {
            delete _b;
        }
    }
    shared_ptr& operator=(const shared_ptr& x) noexcept {
        if (this != &x) {
            this->~shared_ptr();
            new (this) shared_ptr(x);
        }
        return *this;
    }
    shared_ptr& operator=(shared_ptr&& x) noexcept {
        if (this != &x) {
            this->~shared_ptr();
            new (this) shared_ptr(std::move(x));
        }
        return *this;
    }
    shared_ptr& operator=(std::nullptr_t) noexcept {
        return *this = shared_ptr();
    }
    template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
    shared_ptr& operator=(const shared_ptr<U>& x) noexcept {
        if (*this != x) {
            this->~shared_ptr();
            new (this) shared_ptr(x);
        }
        return *this;
    }
    template <typename U, typename = std::enable_if_t<std::is_base_of<T, U>::value>>
    shared_ptr& operator=(shared_ptr<U>&& x) noexcept {
        if (*this != x) {
            this->~shared_ptr();
            new (this) shared_ptr(std::move(x));
        }
        return *this;
    }
    explicit operator bool() const noexcept {
        return _p;
    }
    T& operator*() const noexcept {
        return *_p;
    }
    T* operator->() const noexcept {
        return _p;
    }
    T* get() const noexcept {
        return _p;
    }
    long use_count() const noexcept {
        if (_b) {
            return _b->count;
        } else {
            return 0;
        }
    }

    template <bool esft>
    struct make_helper;

    template <typename U, typename... A>
    friend shared_ptr<U> make_shared(A&&... a);

    template <typename U>
    friend shared_ptr<U> make_shared(U&& a);

    template <typename V, typename U>
    friend shared_ptr<V> static_pointer_cast(const shared_ptr<U>& p);

    template <typename V, typename U>
    friend shared_ptr<V> dynamic_pointer_cast(const shared_ptr<U>& p);

    template <typename V, typename U>
    friend shared_ptr<V> const_pointer_cast(const shared_ptr<U>& p);

    template <bool esft, typename... A>
    static shared_ptr make(A&&... a);

    template <typename U>
    friend class enable_shared_from_this;

    template <typename U, bool esft>
    friend struct shared_ptr_make_helper;

    template <typename U>
    friend class shared_ptr;
};

template <typename U, bool esft>
struct shared_ptr_make_helper;

template <typename T>
struct shared_ptr_make_helper<T, false> {
    template <typename... A>
    static shared_ptr<T> make(A&&... a) {
        return shared_ptr<T>(new shared_ptr_count_for<T>(std::forward<A>(a)...));
    }
};

template <typename T>
struct shared_ptr_make_helper<T, true> {
    template <typename... A>
    static shared_ptr<T> make(A&&... a) {
        auto p = new T(std::forward<A>(a)...);
        return shared_ptr<T>(p, p);
    }
};

template <typename T, typename... A>
inline
shared_ptr<T>
make_shared(A&&... a) {
    using helper = shared_ptr_make_helper<T, std::is_base_of<shared_ptr_count_base, T>::value>;
    return helper::make(std::forward<A>(a)...);
}

template <typename T>
inline
shared_ptr<T>
make_shared(T&& a) {
    using helper = shared_ptr_make_helper<T, std::is_base_of<shared_ptr_count_base, T>::value>;
    return helper::make(std::forward<T>(a));
}

template <typename T, typename U>
inline
shared_ptr<T>
static_pointer_cast(const shared_ptr<U>& p) {
    return shared_ptr<T>(p._b, static_cast<T*>(p._p));
}

template <typename T, typename U>
inline
shared_ptr<T>
dynamic_pointer_cast(const shared_ptr<U>& p) {
    auto q = dynamic_cast<T*>(p._p);
    return shared_ptr<T>(q ? p._b : nullptr, q);
}

template <typename T, typename U>
inline
shared_ptr<T>
const_pointer_cast(const shared_ptr<U>& p) {
    return shared_ptr<T>(p._b, const_cast<T*>(p._p));
}

template <typename T>
inline
shared_ptr<T>
enable_shared_from_this<T>::shared_from_this() {
    auto unconst = reinterpret_cast<enable_shared_from_this<std::remove_const_t<T>>*>(this);
    return shared_ptr<T>(unconst);
}

template <typename T>
inline
shared_ptr<const T>
enable_shared_from_this<T>::shared_from_this() const {
    auto esft = const_cast<enable_shared_from_this*>(this);
    auto unconst = reinterpret_cast<enable_shared_from_this<std::remove_const_t<T>>*>(esft);
    return shared_ptr<const T>(unconst);
}

template <typename T, typename U>
inline
bool
operator==(const shared_ptr<T>& x, const shared_ptr<U>& y) {
    return x.get() == y.get();
}

template <typename T>
inline
bool
operator==(const shared_ptr<T>& x, std::nullptr_t) {
    return x.get() == nullptr;
}

template <typename T>
inline
bool
operator==(std::nullptr_t, const shared_ptr<T>& y) {
    return nullptr == y.get();
}

template <typename T, typename U>
inline
bool
operator!=(const shared_ptr<T>& x, const shared_ptr<U>& y) {
    return x.get() != y.get();
}

template <typename T>
inline
bool
operator!=(const shared_ptr<T>& x, std::nullptr_t) {
    return x.get() != nullptr;
}

template <typename T>
inline
bool
operator!=(std::nullptr_t, const shared_ptr<T>& y) {
    return nullptr != y.get();
}

template <typename T, typename U>
inline
bool
operator<(const shared_ptr<T>& x, const shared_ptr<U>& y) {
    return x.get() < y.get();
}

template <typename T>
inline
bool
operator<(const shared_ptr<T>& x, std::nullptr_t) {
    return x.get() < nullptr;
}

template <typename T>
inline
bool
operator<(std::nullptr_t, const shared_ptr<T>& y) {
    return nullptr < y.get();
}

template <typename T, typename U>
inline
bool
operator<=(const shared_ptr<T>& x, const shared_ptr<U>& y) {
    return x.get() <= y.get();
}

template <typename T>
inline
bool
operator<=(const shared_ptr<T>& x, std::nullptr_t) {
    return x.get() <= nullptr;
}

template <typename T>
inline
bool
operator<=(std::nullptr_t, const shared_ptr<T>& y) {
    return nullptr <= y.get();
}

template <typename T, typename U>
inline
bool
operator>(const shared_ptr<T>& x, const shared_ptr<U>& y) {
    return x.get() > y.get();
}

template <typename T>
inline
bool
operator>(const shared_ptr<T>& x, std::nullptr_t) {
    return x.get() > nullptr;
}

template <typename T>
inline
bool
operator>(std::nullptr_t, const shared_ptr<T>& y) {
    return nullptr > y.get();
}

template <typename T, typename U>
inline
bool
operator>=(const shared_ptr<T>& x, const shared_ptr<U>& y) {
    return x.get() >= y.get();
}

template <typename T>
inline
bool
operator>=(const shared_ptr<T>& x, std::nullptr_t) {
    return x.get() >= nullptr;
}

template <typename T>
inline
bool
operator>=(std::nullptr_t, const shared_ptr<T>& y) {
    return nullptr >= y.get();
}

template <typename T>
static inline
std::ostream& operator<<(std::ostream& out, const shared_ptr<T>& p) {
    if (!p) {
        return out << "null";
    }
    return out << *p;
}

template<typename T>
using shared_ptr_equal_by_value = indirect_equal_to<shared_ptr<T>>;

template<typename T>
using shared_ptr_value_hash = indirect_hash<shared_ptr<T>>;

}

namespace std {

template <typename T>
struct hash<seastar::lw_shared_ptr<T>> : private hash<T*> {
    size_t operator()(const seastar::lw_shared_ptr<T>& p) const {
        return hash<T*>::operator()(p.get());
    }
};

template <typename T>
struct hash<seastar::shared_ptr<T>> : private hash<T*> {
    size_t operator()(const seastar::shared_ptr<T>& p) const {
        return hash<T*>::operator()(p.get());
    }
};

}

namespace seastar {

template<typename T>
struct is_smart_ptr<shared_ptr<T>> : std::true_type {};

template<typename T>
struct is_smart_ptr<lw_shared_ptr<T>> : std::true_type {};

}
using column_count_type = uint32_t;
using column_id = column_count_type;
class schema;
using schema_ptr = seastar::lw_shared_ptr<const schema>;
GCC6_CONCEPT(template <typename T> concept bool HasTriCompare =
                 requires(const T &t) {
                   { t.compare(t) }
                   ->int;
                 } &&
                 std::is_same<std::result_of_t<decltype (&T::compare)(T, T)>,
                              int>::value;)
template <typename T>
class with_relational_operators {
private:
  template <typename U>
  GCC6_CONCEPT(requires HasTriCompare<U>)
  int do_compare(const U &t) const;
};

#include <experimental/type_traits>

namespace seastar {

namespace stdx = std::experimental;

GCC6_CONCEPT(

template<typename T>
concept bool OptimizableOptional() {
    return stdx::is_default_constructible_v<T>
        && stdx::is_nothrow_move_assignable_v<T>
        && requires(const T& obj) {
            { bool(obj) } noexcept;
        };
}

)

/// \c optimized_optional<> is intended mainly for use with classes that store
/// their data externally and expect pointer to this data to be always non-null.
/// In such case there is no real need for another flag signifying whether
/// the optional is engaged.
template<typename T>
class optimized_optional {
    T _object;
public:
    optimized_optional() = default;
    optimized_optional(compat::nullopt_t) noexcept { }
    optimized_optional(const T& obj) : _object(obj) { }
    optimized_optional(T&& obj) noexcept : _object(std::move(obj)) { }
    optimized_optional(compat::optional<T>&& obj) noexcept {
        if (obj) {
            _object = std::move(*obj);
        }
    }
    optimized_optional(const optimized_optional&) = default;
    optimized_optional(optimized_optional&&) = default;

    optimized_optional& operator=(compat::nullopt_t) noexcept {
        _object = T();
        return *this;
    }
    template<typename U>
    std::enable_if_t<std::is_same<std::decay_t<U>, T>::value, optimized_optional&>
    operator=(U&& obj) noexcept {
        _object = std::forward<U>(obj);
        return *this;
    }
    optimized_optional& operator=(const optimized_optional&) = default;
    optimized_optional& operator=(optimized_optional&&) = default;

    explicit operator bool() const noexcept {
        return bool(_object);
    }

    T* operator->() noexcept { return &_object; }
    const T* operator->() const noexcept { return &_object; }

    T& operator*() noexcept { return _object; }
    const T& operator*() const noexcept { return _object; }

    bool operator==(const optimized_optional& other) const {
        return _object == other._object;
    }
    bool operator!=(const optimized_optional& other) const {
        return _object != other._object;
    }
    friend std::ostream& operator<<(std::ostream& out, const optimized_optional& opt) {
        if (!opt) {
            return out << "null";
        }
        return out << *opt;
    }
};

}
using bytes = basic_sstring<int8_t, uint32_t, 31, false>;
template <typename CharOutputIterator>
GCC6_CONCEPT(requires requires(CharOutputIterator it) {
  *it++ = 'a';
  *it++ = 'a';
})
inline void serialize_string(CharOutputIterator &out, const char *s) {
  auto len = strlen(s);
  out = std::copy_n(s, len, out);
}
namespace utils {
class UUID {
private:
  int64_t most_sig_bits;
  int64_t least_sig_bits;
public:
  ;
};
} // namespace utils
class abstract_type;
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
    auto ptr =
        static_cast<T *>(::aligned_alloc(alignof(T), new_capacity * sizeof(T)));
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
    auto ptr =
        static_cast<T *>(::aligned_alloc(alignof(T), other.size() * sizeof(T)));
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
      : _begin(_internal.storage), _end(_begin), _capacity_end(_begin + N) {}
  template <typename InputIterator>
  small_vector(InputIterator first, InputIterator last) : small_vector() {
    if constexpr (std::is_base_of_v<std::forward_iterator_tag,
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
        std::memcpy(_internal.storage, other._internal.storage, N * sizeof(T));
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
        std::memcpy(_internal.storage, other._internal.storage, N * sizeof(T));
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
    if constexpr (std::is_base_of_v<std::forward_iterator_tag,
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
  iterator erase(const_iterator cit) noexcept { return erase(cit, cit + 1); }
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
    return size() == other.size() && std::equal(_begin, _end, other.begin());
  }
  bool operator!=(const small_vector &other) const noexcept {
    return !(*this == other);
  }
};
} // namespace utils
#include <list>

namespace seastar {

// unordered_map implemented as a simple array

template <typename Value, size_t Max>
class array_map {
    std::array<Value, Max> _a {};
public:
    array_map(std::initializer_list<std::pair<size_t, Value>> i) {
        for (auto kv : i) {
            _a[kv.first] = kv.second;
        }
    }
    Value& operator[](size_t key) { return _a[key]; }
    const Value& operator[](size_t key) const { return _a[key]; }

    Value& at(size_t key) {
        if (key >= Max) {
            throw std::out_of_range(std::to_string(key) + " >= " + std::to_string(Max));
        }
        return _a[key];
    }
};

}
#include <arpa/inet.h>  // for ntohs() and friends

namespace seastar {

template <typename T>
struct unaligned {
    T raw;
    unaligned() = default;
    unaligned(T x) : raw(x) {}
    unaligned& operator=(const T& x) { raw = x; return *this; }
    operator T() const { return raw; }
} __attribute__((packed));


// deprecated: violates strict aliasing rules
template <typename T, typename F>
inline auto unaligned_cast(F* p) {
    return reinterpret_cast<unaligned<std::remove_pointer_t<T>>*>(p);
}

// deprecated: violates strict aliasing rules
template <typename T, typename F>
inline auto unaligned_cast(const F* p) {
    return reinterpret_cast<const unaligned<std::remove_pointer_t<T>>*>(p);
}

}

namespace seastar {

inline uint64_t ntohq(uint64_t v) {
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    // big endian, nothing to do
    return v;
#else
    // little endian, reverse bytes
    return __builtin_bswap64(v);
#endif
}
inline uint64_t htonq(uint64_t v) {
    // htonq and ntohq have identical implementations
    return ntohq(v);
}

namespace net {

inline void ntoh() {}
inline void hton() {}

inline uint8_t ntoh(uint8_t x) { return x; }
inline uint8_t hton(uint8_t x) { return x; }
inline uint16_t ntoh(uint16_t x) { return ntohs(x); }
inline uint16_t hton(uint16_t x) { return htons(x); }
inline uint32_t ntoh(uint32_t x) { return ntohl(x); }
inline uint32_t hton(uint32_t x) { return htonl(x); }
inline uint64_t ntoh(uint64_t x) { return ntohq(x); }
inline uint64_t hton(uint64_t x) { return htonq(x); }

inline int8_t ntoh(int8_t x) { return x; }
inline int8_t hton(int8_t x) { return x; }
inline int16_t ntoh(int16_t x) { return ntohs(x); }
inline int16_t hton(int16_t x) { return htons(x); }
inline int32_t ntoh(int32_t x) { return ntohl(x); }
inline int32_t hton(int32_t x) { return htonl(x); }
inline int64_t ntoh(int64_t x) { return ntohq(x); }
inline int64_t hton(int64_t x) { return htonq(x); }

// Deprecated alias net::packed<> for unaligned<> from unaligned.hh.
// TODO: get rid of this alias.
template <typename T> using packed = unaligned<T>;

template <typename T>
inline T ntoh(const packed<T>& x) {
    T v = x;
    return ntoh(v);
}

template <typename T>
inline T hton(const packed<T>& x) {
    T v = x;
    return hton(v);
}

template <typename T>
inline std::ostream& operator<<(std::ostream& os, const packed<T>& v) {
    auto x = v.raw;
    return os << x;
}

inline
void ntoh_inplace() {}
inline
void hton_inplace() {};

template <typename First, typename... Rest>
inline
void ntoh_inplace(First& first, Rest&... rest) {
    first = ntoh(first);
    ntoh_inplace(std::forward<Rest&>(rest)...);
}

template <typename First, typename... Rest>
inline
void hton_inplace(First& first, Rest&... rest) {
    first = hton(first);
    hton_inplace(std::forward<Rest&>(rest)...);
}

template <class T>
inline
T ntoh(const T& x) {
    T tmp = x;
    tmp.adjust_endianness([] (auto&&... what) { ntoh_inplace(std::forward<decltype(what)&>(what)...); });
    return tmp;
}

template <class T>
inline
T hton(const T& x) {
    T tmp = x;
    tmp.adjust_endianness([] (auto&&... what) { hton_inplace(std::forward<decltype(what)&>(what)...); });
    return tmp;
}

}

}

namespace seastar {

inline uint8_t cpu_to_le(uint8_t x) { return x; }
inline uint8_t le_to_cpu(uint8_t x) { return x; }
inline uint16_t cpu_to_le(uint16_t x) { return htole16(x); }
inline uint16_t le_to_cpu(uint16_t x) { return le16toh(x); }
inline uint32_t cpu_to_le(uint32_t x) { return htole32(x); }
inline uint32_t le_to_cpu(uint32_t x) { return le32toh(x); }
inline uint64_t cpu_to_le(uint64_t x) { return htole64(x); }
inline uint64_t le_to_cpu(uint64_t x) { return le64toh(x); }

inline int8_t cpu_to_le(int8_t x) { return x; }
inline int8_t le_to_cpu(int8_t x) { return x; }
inline int16_t cpu_to_le(int16_t x) { return htole16(x); }
inline int16_t le_to_cpu(int16_t x) { return le16toh(x); }
inline int32_t cpu_to_le(int32_t x) { return htole32(x); }
inline int32_t le_to_cpu(int32_t x) { return le32toh(x); }
inline int64_t cpu_to_le(int64_t x) { return htole64(x); }
inline int64_t le_to_cpu(int64_t x) { return le64toh(x); }

inline uint8_t cpu_to_be(uint8_t x) { return x; }
inline uint8_t be_to_cpu(uint8_t x) { return x; }
inline uint16_t cpu_to_be(uint16_t x) { return htobe16(x); }
inline uint16_t be_to_cpu(uint16_t x) { return be16toh(x); }
inline uint32_t cpu_to_be(uint32_t x) { return htobe32(x); }
inline uint32_t be_to_cpu(uint32_t x) { return be32toh(x); }
inline uint64_t cpu_to_be(uint64_t x) { return htobe64(x); }
inline uint64_t be_to_cpu(uint64_t x) { return be64toh(x); }

inline int8_t cpu_to_be(int8_t x) { return x; }
inline int8_t be_to_cpu(int8_t x) { return x; }
inline int16_t cpu_to_be(int16_t x) { return htobe16(x); }
inline int16_t be_to_cpu(int16_t x) { return be16toh(x); }
inline int32_t cpu_to_be(int32_t x) { return htobe32(x); }
inline int32_t be_to_cpu(int32_t x) { return be32toh(x); }
inline int64_t cpu_to_be(int64_t x) { return htobe64(x); }
inline int64_t be_to_cpu(int64_t x) { return be64toh(x); }

template <typename T>
inline T cpu_to_le(const unaligned<T>& v) {
    return cpu_to_le(T(v));
}

template <typename T>
inline T le_to_cpu(const unaligned<T>& v) {
    return le_to_cpu(T(v));
}

template <typename T>
inline
T
read_le(const char* p) {
    T datum;
    std::copy_n(p, sizeof(T), reinterpret_cast<char*>(&datum));
    return le_to_cpu(datum);
}

template <typename T>
inline
void
write_le(char* p, T datum) {
    datum = cpu_to_le(datum);
    std::copy_n(reinterpret_cast<const char*>(&datum), sizeof(T), p);
}

template <typename T>
inline
T
read_be(const char* p) {
    T datum;
    std::copy_n(p, sizeof(T), reinterpret_cast<char*>(&datum));
    return be_to_cpu(datum);
}

template <typename T>
inline
void
write_be(char* p, T datum) {
    datum = cpu_to_be(datum);
    std::copy_n(reinterpret_cast<const char*>(&datum), sizeof(T), p);
}

template <typename T>
inline
T
consume_be(const char*& p) {
    auto ret = read_be<T>(p);
    p += sizeof(T);
    return ret;
}

template <typename T>
inline
void
produce_be(char*& p, T datum) {
    write_be<T>(p, datum);
    p += sizeof(T);
}

}
/// \mainpage
///
/// Seastar is a high performance C++ application framework for high
/// concurrency server applications.
///
/// A good place to start is the [Tutorial](doc/tutorial.md).
///
/// Please see:
///   - \ref future-module Documentation on futures and promises, which are
///          the seastar building blocks.
///   - \ref future-util Utililty functions for working with futures
///   - \ref memory-module Memory management
///   - \ref networking-module TCP/IP networking
///   - \ref fileio-module File Input/Output
///   - \ref smp-module Multicore support
///   - \ref fiber-module Utilities for managing loosely coupled chains of
///          continuations, also known as fibers
///   - \ref thread-module Support for traditional threaded execution


namespace seastar {

template <typename Func, typename Args, typename IndexList>
struct apply_helper;

template <typename Func, typename Tuple, size_t... I>
struct apply_helper<Func, Tuple, std::index_sequence<I...>> {
    static auto apply(Func&& func, Tuple args) {
        return func(std::get<I>(std::forward<Tuple>(args))...);
    }
};

template <typename Func, typename... T>
inline
auto apply(Func&& func, std::tuple<T...>&& args) {
    using helper = apply_helper<Func, std::tuple<T...>&&, std::index_sequence_for<T...>>;
    return helper::apply(std::forward<Func>(func), std::move(args));
}

template <typename Func, typename... T>
inline
auto apply(Func&& func, std::tuple<T...>& args) {
    using helper = apply_helper<Func, std::tuple<T...>&, std::index_sequence_for<T...>>;
    return helper::apply(std::forward<Func>(func), args);
}

template <typename Func, typename... T>
inline
auto apply(Func&& func, const std::tuple<T...>& args) {
    using helper = apply_helper<Func, const std::tuple<T...>&, std::index_sequence_for<T...>>;
    return helper::apply(std::forward<Func>(func), args);
}

}

#include <typeindex>

namespace seastar {

template<typename T>
struct function_traits;

template<typename Ret, typename... Args>
struct function_traits<Ret(Args...)>
{
    using return_type = Ret;
    using args_as_tuple = std::tuple<Args...>;
    using signature = Ret (Args...);

    static constexpr std::size_t arity = sizeof...(Args);

    template <std::size_t N>
    struct arg
    {
        static_assert(N < arity, "no such parameter index.");
        using type = typename std::tuple_element<N, std::tuple<Args...>>::type;
    };
};

template<typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> : public function_traits<Ret(Args...)>
{};

template <typename T, typename Ret, typename... Args>
struct function_traits<Ret(T::*)(Args...)> : public function_traits<Ret(Args...)>
{};

template <typename T, typename Ret, typename... Args>
struct function_traits<Ret(T::*)(Args...) const> : public function_traits<Ret(Args...)>
{};

template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())>
{};

template<typename T>
struct function_traits<T&> : public function_traits<std::remove_reference_t<T>>
{};

}

/// \file

namespace seastar {

constexpr unsigned max_scheduling_groups() { return 16; }

template <typename... T>
class future;

class reactor;

class scheduling_group;

namespace internal {

// Returns an index between 0 and max_scheduling_groups()
unsigned scheduling_group_index(scheduling_group sg);
scheduling_group scheduling_group_from_index(unsigned index);

}


/// Creates a scheduling group with a specified number of shares.
///
/// The operation is global and affects all shards. The returned scheduling
/// group can then be used in any shard.
///
/// \param name A name that identifiers the group; will be used as a label
///             in the group's metrics
/// \param shares number of shares of the CPU time allotted to the group;
///              Use numbers in the 1-1000 range (but can go above).
/// \return a scheduling group that can be used on any shard
future<scheduling_group> create_scheduling_group(sstring name, float shares);

/// Destroys a scheduling group.
///
/// Destroys a \ref scheduling_group previously created with create_scheduling_group().
/// The destroyed group must not be currently in use and must not be used later.
///
/// The operation is global and affects all shards.
///
/// \param sg The scheduling group to be destroyed
/// \return a future that is ready when the scheduling group has been torn down
future<> destroy_scheduling_group(scheduling_group sg);

/// Rename scheduling group.
///
/// Renames a \ref scheduling_group previously created with create_scheduling_group().
///
/// The operation is global and affects all shards.
/// The operation affects the exported statistics labels.
///
/// \param sg The scheduling group to be renamed
/// \param new_name The new name for the scheduling group.
/// \return a future that is ready when the scheduling group has been renamed
future<> rename_scheduling_group(scheduling_group sg, sstring new_name);


/**
 * Represents a configuration for a specific scheduling group value,
 * it contains all that is needed to maintain a scheduling group specific
 * value when it needs to be created, due to, for example, a new \ref scheduling
 * group being created.
 *
 * @note is is recomended to use @ref make_scheduling_group_key_config in order to
 * create and configure this syructure. The only reason that one might want to not use
 * this method is because of a need for specific intervention in the construction or
 * destruction of the value. Even then, it is recommended to first create the configuration
 * with @ref make_scheduling_group_key_config and only the change it.
 *
 */
struct scheduling_group_key_config {
    /**
     * Constructs a default configuration
     */
    scheduling_group_key_config() :
        scheduling_group_key_config(typeid(void)) {}
    /**
     * Creates a configuration that is made for a specific type.
     * It does not contain the right alignment and allocation sizes
     * neither the correct construction or destruction logic, but only
     * the indication for the intended type which is used in debug mode
     * to make sure that the correct type is reffered to when accessing
     * the value.
     * @param type_info - the type information class (create with typeid(T)).
     */
    scheduling_group_key_config(const std::type_info& type_info) :
            type_index(type_info) {}
    /// The allocation size for the value (usually: sizeof(T))
    size_t allocation_size;
    /// The required alignment of the value (usually: alignof(T))
    size_t alignment;
    /// Holds the type information for debug mode runtime validation
    std::type_index type_index;
    /// A function that will be called for each newly allocated value
    std::function<void (void*)> constructor;
    /// A function that will be called for each element that is about
    /// to be dealocated.
    std::function<void (void*)> destructor;

};


/**
 * A class that is intended to encapsulate the scheduling group specific
 * key and "hide" it implementation concerns and details.
 *
 * @note this object can be copied accross shards and scheduling groups.
 */
class scheduling_group_key {
public:
    /// The only user allowed operation on a key is copying.
    scheduling_group_key(const scheduling_group_key&) = default;
private:
    scheduling_group_key(unsigned long id) :
        _id(id) {}
    unsigned long _id;
    unsigned long id() const {
        return _id;
    }
    friend class reactor;
    friend future<scheduling_group_key> scheduling_group_key_create(scheduling_group_key_config cfg);
    template<typename T>
    friend T& scheduling_group_get_specific(scheduling_group sg, scheduling_group_key key);
    template<typename T>
    friend T& scheduling_group_get_specific(scheduling_group_key key);
};

namespace internal {
/**
 * @brief A function in the spirit of Cpp17 apply, but specifically for constructors.
 * This function is used in order to preserve support in Cpp14.

 * @tparam ConstructorType - the constructor type or in other words the type to be constructed
 * @tparam Tuple - T params tuple type (should be deduced)
 * @tparam size_t...Idx - a sequence of indexes in order to access the typpels members in compile time.
 * (should be deduced)
 *
 * @param pre_alocated_mem - a pointer to the pre allocated memory chunk that will hold the
 * the initialized object.
 * @param args - A tupple that holds the prarameters for the constructor
 * @param idx_seq - An index sequence that will be used to access the members of the tuple in compile
 * time.
 *
 * @note this function was not intended to be called by users and it is only a utility function
 * for suporting \ref make_scheduling_group_key_config
 */
template<typename ConstructorType, typename Tuple, size_t...Idx>
void apply_constructor(void* pre_alocated_mem, Tuple args, std::index_sequence<Idx...> idx_seq) {
    new (pre_alocated_mem) ConstructorType(std::get<Idx>(args)...);
}
}

/**
 * A template function that builds a scheduling group specific value configuration.
 * This configuration is used by the infrastructure to allocate memory for the values
 * and initialize or deinitialize them when they are created or destroyed.
 *
 * @tparam T - the type for the newly created value.
 * @tparam ...ConstructorArgs - the types for the constructor parameters (should be deduced)
 * @param args - The parameters for the constructor.
 * @return a fully initialized \ref scheduling_group_key_config object.
 */
template <typename T, typename... ConstructorArgs>
scheduling_group_key_config
make_scheduling_group_key_config(ConstructorArgs... args) {
    scheduling_group_key_config sgkc(typeid(T));
    sgkc.allocation_size = sizeof(T);
    sgkc.alignment = alignof(T);
    sgkc.constructor = [args = std::make_tuple(args...)] (void* p) {
        internal::apply_constructor<T>(p, args, std::make_index_sequence<sizeof...(ConstructorArgs)>());
    };
    sgkc.destructor = [] (void* p) {
        static_cast<T*>(p)->~T();
    };
    return sgkc;
}

/**
 * Returns a future that holds a scheduling key and resolves when this key can be used
 * to access the scheduling group specific value it represents.
 * @param cfg - A \ref scheduling_group_key_config object (by recomendation: initialized with
 * \ref make_scheduling_group_key_config )
 * @return A future containing \ref scheduling_group_key for the newly created specific value.
 */
future<scheduling_group_key> scheduling_group_key_create(scheduling_group_key_config cfg);

/**
 * Returnes a reference to the given scheduling group specific value
 * @tparam T - the type of the scheduling specific type (cannot be deduced)
 * @param sg - the scheduling group which it's specific value to retrieve
 * @param key - the key of the value to retrieve.
 * @return A reference to the scheduling specific value.
 */
template<typename T>
T& scheduling_group_get_specific(scheduling_group sg, scheduling_group_key key);


/// \brief Identifies function calls that are accounted as a group
///
/// A `scheduling_group` is a tag that can be used to mark a function call.
/// Executions of such tagged calls are accounted as a group.
class scheduling_group {
    unsigned _id;
private:
    explicit scheduling_group(unsigned id) : _id(id) {}
public:
    /// Creates a `scheduling_group` object denoting the default group
    constexpr scheduling_group() noexcept : _id(0) {} // must be constexpr for current_scheduling_group_holder
    bool active() const;
    const sstring& name() const;
    bool operator==(scheduling_group x) const { return _id == x._id; }
    bool operator!=(scheduling_group x) const { return _id != x._id; }
    bool is_main() const { return _id == 0; }
    template<typename T>
    /**
     * Returnes a reference to this scheduling group specific value
     * @tparam T - the type of the scheduling specific type (cannot be deduced)
     * @param key - the key of the value to retrieve.
     * @return A reference to this scheduling specific value.
     */
    T& get_specific(scheduling_group_key key) {
        return scheduling_group_get_specific<T>(*this, key);
    }
    /// Adjusts the number of shares allotted to the group.
    ///
    /// Dynamically adjust the number of shares allotted to the group, increasing or
    /// decreasing the amount of CPU bandwidth it gets. The adjustment is local to
    /// the shard.
    ///
    /// This can be used to reduce a background job's interference with a foreground
    /// load: the shares can be started at a low value, increased when the background
    /// job's backlog increases, and reduced again when the backlog decreases.
    ///
    /// \param shares number of shares allotted to the group. Use numbers
    ///               in the 1-1000 range.
    void set_shares(float shares);
    friend future<scheduling_group> create_scheduling_group(sstring name, float shares);
    friend future<> destroy_scheduling_group(scheduling_group sg);
    friend future<> rename_scheduling_group(scheduling_group sg, sstring new_name);
    friend class reactor;
    friend unsigned internal::scheduling_group_index(scheduling_group sg);
    friend scheduling_group internal::scheduling_group_from_index(unsigned index);

    template<typename SpecificValType, typename Mapper, typename Reducer, typename Initial>
    GCC6_CONCEPT( requires requires(SpecificValType specific_val, Mapper mapper, Reducer reducer, Initial initial) {
        {reducer(initial, mapper(specific_val))} -> Initial;
    })
    friend future<typename function_traits<Reducer>::return_type>
    map_reduce_scheduling_group_specific(Mapper mapper, Reducer reducer, Initial initial_val, scheduling_group_key key);

    template<typename SpecificValType, typename Reducer, typename Initial>
    GCC6_CONCEPT( requires requires(SpecificValType specific_val, Reducer reducer, Initial initial) {
        {reducer(initial, specific_val)} -> Initial;
    })
    friend future<typename function_traits<Reducer>::return_type>
        reduce_scheduling_group_specific(Reducer reducer, Initial initial_val, scheduling_group_key key);


};

/// \cond internal
namespace internal {

inline
unsigned
scheduling_group_index(scheduling_group sg) {
    return sg._id;
}

inline
scheduling_group
scheduling_group_from_index(unsigned index) {
    return scheduling_group(index);
}

inline
scheduling_group*
current_scheduling_group_ptr() {
    // Slow unless constructor is constexpr
    static thread_local scheduling_group sg;
    return &sg;
}

}
/// \endcond

/// Returns the current scheduling group
inline
scheduling_group
current_scheduling_group() {
    return *internal::current_scheduling_group_ptr();
}

inline
scheduling_group
default_scheduling_group() {
    return scheduling_group();
}

inline
bool
scheduling_group::active() const {
    return *this == current_scheduling_group();
}

}

namespace std {

template <>
struct hash<seastar::scheduling_group> {
    size_t operator()(seastar::scheduling_group sg) const {
        return seastar::internal::scheduling_group_index(sg);
    }
};

}

namespace seastar {

class task {
    scheduling_group _sg;
protected:
    // Task destruction is performed by run_and_dispose() via a concrete type,
    // so no need for a virtual destructor here. Derived classes that implement
    // run_and_dispose() should be declared final to avoid losing concrete type
    // information via inheritance.
    ~task() = default;
public:
    explicit task(scheduling_group sg = current_scheduling_group()) : _sg(sg) {}
    virtual void run_and_dispose() noexcept = 0;
    scheduling_group group() const { return _sg; }
};

void schedule(task* t) noexcept;
void schedule_urgent(task* t) noexcept;

template <typename Func>
class lambda_task final : public task {
    Func _func;
public:
    lambda_task(scheduling_group sg, const Func& func) : task(sg), _func(func) {}
    lambda_task(scheduling_group sg, Func&& func) : task(sg), _func(std::move(func)) {}
    virtual void run_and_dispose() noexcept override {
        _func();
        delete this;
    }
};

template <typename Func>
inline
task*
make_task(Func&& func) noexcept {
    return new lambda_task<Func>(current_scheduling_group(), std::forward<Func>(func));
}

template <typename Func>
inline
task*
make_task(scheduling_group sg, Func&& func) noexcept {
    return new lambda_task<Func>(sg, std::forward<Func>(func));
}

}
namespace seastar {

namespace internal {

struct preemption_monitor {
    // We preempt when head != tail
    // This happens to match the Linux aio completion ring, so we can have the
    // kernel preempt a task by queuing a completion event to an io_context.
    std::atomic<uint32_t> head;
    std::atomic<uint32_t> tail;
};

}

extern __thread const internal::preemption_monitor* g_need_preempt;

inline bool need_preempt() noexcept {
#ifndef SEASTAR_DEBUG
    // prevent compiler from eliminating loads in a loop
    std::atomic_signal_fence(std::memory_order_seq_cst);
    auto np = g_need_preempt;
    // We aren't reading anything from the ring, so we don't need
    // any barriers.
    auto head = np->head.load(std::memory_order_relaxed);
    auto tail = np->tail.load(std::memory_order_relaxed);
    // Possible optimization: read head and tail in a single 64-bit load,
    // and find a funky way to compare the two 32-bit halves.
    return __builtin_expect(head != tail, false);
#else
    return true;
#endif
}

}
#include <setjmp.h>
#include <ucontext.h>

namespace seastar {
/// Clock used for scheduling threads
using thread_clock = std::chrono::steady_clock;

/// \cond internal
class thread_context;
class scheduling_group;

struct jmp_buf_link {
#ifdef SEASTAR_ASAN_ENABLED
    ucontext_t context;
    void* fake_stack = nullptr;
    const void* stack_bottom;
    size_t stack_size;
#else
    jmp_buf jmpbuf;
#endif
    jmp_buf_link* link;
    thread_context* thread;
public:
    void initial_switch_in(ucontext_t* initial_context, const void* stack_bottom, size_t stack_size);
    void switch_in();
    void switch_out();
    void initial_switch_in_completed();
    void final_switch_out();
};

extern thread_local jmp_buf_link* g_current_context;

namespace thread_impl {

inline thread_context* get() {
    return g_current_context->thread;
}

inline bool should_yield() {
    if (need_preempt()) {
        return true;
    } else {
        return false;
    }
}

scheduling_group sched_group(const thread_context*);

void yield();
void switch_in(thread_context* to);
void switch_out(thread_context* from);
void init();

}
}
/// \endcond


#include <assert.h>



namespace seastar {
namespace internal {
// Empty types have a size of 1, but that byte is not actually
// used. This helper is used to avoid accessing that byte.
template<typename T>
struct used_size {
    static constexpr size_t value = std::is_empty<T>::value ? 0 : sizeof(T);
};
}
}


namespace seastar {

template <typename Signature>
class noncopyable_function;

namespace internal {

class noncopyable_function_base {
private:
    noncopyable_function_base() = default;
    static constexpr size_t nr_direct = 32;
    union [[gnu::may_alias]] storage {
        char direct[nr_direct];
        void* indirect;
    };
    using move_type = void (*)(noncopyable_function_base* from, noncopyable_function_base* to);
    using destroy_type = void (*)(noncopyable_function_base* func);

    static void empty_move(noncopyable_function_base* from, noncopyable_function_base* to) {}
    static void empty_destroy(noncopyable_function_base* func) {}

    static void indirect_move(noncopyable_function_base* from, noncopyable_function_base* to) {
        using void_ptr = void*;
        new (&to->_storage.indirect) void_ptr(from->_storage.indirect);
    }

    template <size_t N>
    static void trivial_direct_move(noncopyable_function_base* from, noncopyable_function_base* to) {
        for (unsigned i = 0; i != N; ++i) {
            to->_storage.direct[i] = from->_storage.direct[i];
        }
    }

    static void trivial_direct_destroy(noncopyable_function_base* func) {
    }

private:
    storage _storage;

    template <typename Signature>
    friend class seastar::noncopyable_function;
};

}

/// A clone of \c std::function, but only invokes the move constructor
/// of the contained function.
template <typename Ret, typename... Args>
class noncopyable_function<Ret (Args...)> : private internal::noncopyable_function_base {
    using call_type = Ret (*)(const noncopyable_function* func, Args...);
    struct vtable {
        const call_type call;
        const move_type move;
        const destroy_type destroy;
    };
private:
    const vtable* _vtable;
private:
    static Ret empty_call(const noncopyable_function* func, Args... args) {
        throw std::bad_function_call();
    }

    static constexpr vtable _s_empty_vtable = {empty_call, empty_move, empty_destroy};

    template <typename Func>
    struct direct_vtable_for {
        static Func* access(noncopyable_function* func) { return reinterpret_cast<Func*>(func->_storage.direct); }
        static const Func* access(const noncopyable_function* func) { return reinterpret_cast<const Func*>(func->_storage.direct); }
        static Func* access(noncopyable_function_base* func) { return access(static_cast<noncopyable_function*>(func)); }
        static Ret call(const noncopyable_function* func, Args... args) {
            return (*access(const_cast<noncopyable_function*>(func)))(std::forward<Args>(args)...);
        }
        static void move(noncopyable_function_base* from, noncopyable_function_base* to) {
            new (access(to)) Func(std::move(*access(from)));
            destroy(from);
        }
        static constexpr move_type select_move_thunk() {
            bool can_trivially_move = std::is_trivially_move_constructible<Func>::value
                    && std::is_trivially_destructible<Func>::value;
            return can_trivially_move ? trivial_direct_move<internal::used_size<Func>::value> : move;
        }
        static void destroy(noncopyable_function_base* func) {
            access(func)->~Func();
        }
        static constexpr destroy_type select_destroy_thunk() {
            return std::is_trivially_destructible<Func>::value ? trivial_direct_destroy : destroy;
        }
        static void initialize(Func&& from, noncopyable_function* to) {
            new (access(to)) Func(std::move(from));
        }
        static constexpr vtable make_vtable() { return { call, select_move_thunk(), select_destroy_thunk() }; }
        static const vtable s_vtable;
    };
    template <typename Func>
    struct indirect_vtable_for {
        static Func* access(noncopyable_function* func) { return reinterpret_cast<Func*>(func->_storage.indirect); }
        static const Func* access(const noncopyable_function* func) { return reinterpret_cast<const Func*>(func->_storage.indirect); }
        static Func* access(noncopyable_function_base* func) { return access(static_cast<noncopyable_function*>(func)); }
        static Ret call(const noncopyable_function* func, Args... args) {
            return (*access(const_cast<noncopyable_function*>(func)))(std::forward<Args>(args)...);
        }
        static void destroy(noncopyable_function_base* func) {
            delete access(func);
        }
        static void initialize(Func&& from, noncopyable_function* to) {
            to->_storage.indirect = new Func(std::move(from));
        }
        static constexpr vtable make_vtable() { return { call, indirect_move, destroy }; }
        static const vtable s_vtable;
    };
    template <typename Func, bool Direct = true>
    struct select_vtable_for : direct_vtable_for<Func> {};
    template <typename Func>
    struct select_vtable_for<Func, false> : indirect_vtable_for<Func> {};
    template <typename Func>
    static constexpr bool is_direct() {
        return sizeof(Func) <= nr_direct && alignof(Func) <= alignof(storage)
                && std::is_nothrow_move_constructible<Func>::value;
    }
    template <typename Func>
    struct vtable_for : select_vtable_for<Func, is_direct<Func>()> {};
public:
    noncopyable_function() noexcept : _vtable(&_s_empty_vtable) {}
    template <typename Func>
    noncopyable_function(Func func) {
        vtable_for<Func>::initialize(std::move(func), this);
        _vtable = &vtable_for<Func>::s_vtable;
    }
    template <typename Object, typename... AllButFirstArg>
    noncopyable_function(Ret (Object::*member)(AllButFirstArg...)) : noncopyable_function(std::mem_fn(member)) {}
    template <typename Object, typename... AllButFirstArg>
    noncopyable_function(Ret (Object::*member)(AllButFirstArg...) const) : noncopyable_function(std::mem_fn(member)) {}

    ~noncopyable_function() {
        _vtable->destroy(this);
    }

    noncopyable_function(const noncopyable_function&) = delete;
    noncopyable_function& operator=(const noncopyable_function&) = delete;

    noncopyable_function(noncopyable_function&& x) noexcept : _vtable(std::exchange(x._vtable, &_s_empty_vtable)) {
        _vtable->move(&x, this);
    }

    noncopyable_function& operator=(noncopyable_function&& x) noexcept {
        if (this != &x) {
            this->~noncopyable_function();
            new (this) noncopyable_function(std::move(x));
        }
        return *this;
    }

    Ret operator()(Args... args) const {
        return _vtable->call(this, std::forward<Args>(args)...);
    }

    explicit operator bool() const {
        return _vtable != &_s_empty_vtable;
    }
};


template <typename Ret, typename... Args>
constexpr typename noncopyable_function<Ret (Args...)>::vtable noncopyable_function<Ret (Args...)>::_s_empty_vtable;

template <typename Ret, typename... Args>
template <typename Func>
const typename noncopyable_function<Ret (Args...)>::vtable noncopyable_function<Ret (Args...)>::direct_vtable_for<Func>::s_vtable
        = noncopyable_function<Ret (Args...)>::direct_vtable_for<Func>::make_vtable();


template <typename Ret, typename... Args>
template <typename Func>
const typename noncopyable_function<Ret (Args...)>::vtable noncopyable_function<Ret (Args...)>::indirect_vtable_for<Func>::s_vtable
        = noncopyable_function<Ret (Args...)>::indirect_vtable_for<Func>::make_vtable();

}


namespace seastar {
namespace memory {

///
/// Allocation failure injection framework. Allows testing for exception safety.
///
/// To exhaustively inject failure at every allocation point:
///
///    uint64_t i = 0;
///    while (true) {
///        try {
///            local_failure_injector().fail_after(i++);
///            code_under_test();
///            local_failure_injector().cancel();
///            break;
///        } catch (const std::bad_alloc&) {
///            // expected
///        }
///    }
///

class alloc_failure_injector {
    uint64_t _alloc_count;
    uint64_t _fail_at = std::numeric_limits<uint64_t>::max();
    noncopyable_function<void()> _on_alloc_failure = [] { throw std::bad_alloc(); };
    bool _failed;
    uint64_t _suppressed = 0;
    friend struct disable_failure_guard;
private:
    void fail();
public:
    void on_alloc_point() {
        if (_suppressed) {
            return;
        }
        if (_alloc_count >= _fail_at) {
            fail();
        }
        ++_alloc_count;
    }

    // Counts encountered allocation points which didn't fail and didn't have failure suppressed
    uint64_t alloc_count() const {
        return _alloc_count;
    }

    // Will cause count-th allocation point from now to fail, counting from 0
    void fail_after(uint64_t count) {
        _fail_at = _alloc_count + count;
        _failed = false;
    }

    // Cancels the failure scheduled by fail_after()
    void cancel() {
        _fail_at = std::numeric_limits<uint64_t>::max();
    }

    // Returns true iff allocation was failed since last fail_after()
    bool failed() const {
        return _failed;
    }

    // Runs given function with a custom failure action instead of the default std::bad_alloc throw.
    void run_with_callback(noncopyable_function<void()> callback, noncopyable_function<void()> to_run);
};

extern thread_local alloc_failure_injector the_alloc_failure_injector;

inline
alloc_failure_injector& local_failure_injector() {
    return the_alloc_failure_injector;
}

#ifdef SEASTAR_ENABLE_ALLOC_FAILURE_INJECTION

struct disable_failure_guard {
    disable_failure_guard() { ++local_failure_injector()._suppressed; }
    ~disable_failure_guard() { --local_failure_injector()._suppressed; }
};

#else

struct disable_failure_guard {
    ~disable_failure_guard() {}
};

#endif

// Marks a point in code which should be considered for failure injection
inline
void on_alloc_point() {
#ifdef SEASTAR_ENABLE_ALLOC_FAILURE_INJECTION
    local_failure_injector().on_alloc_point();
#endif
}

}
}

#if defined(__has_cpp_attribute) && __has_cpp_attribute(nodiscard)
    #define SEASTAR_NODISCARD [[nodiscard]]
#else
    #define SEASTAR_NODISCARD
#endif

namespace seastar {

/// \defgroup future-module Futures and Promises
///
/// \brief
/// Futures and promises are the basic tools for asynchronous
/// programming in seastar.  A future represents a result that
/// may not have been computed yet, for example a buffer that
/// is being read from the disk, or the result of a function
/// that is executed on another cpu.  A promise object allows
/// the future to be eventually resolved by assigning it a value.
///
/// \brief
/// Another way to look at futures and promises are as the reader
/// and writer sides, respectively, of a single-item, single use
/// queue.  You read from the future, and write to the promise,
/// and the system takes care that it works no matter what the
/// order of operations is.
///
/// \brief
/// The normal way of working with futures is to chain continuations
/// to them.  A continuation is a block of code (usually a lamdba)
/// that is called when the future is assigned a value (the future
/// is resolved); the continuation can then access the actual value.
///

/// \defgroup future-util Future Utilities
///
/// \brief
/// These utilities are provided to help perform operations on futures.


/// \addtogroup future-module
/// @{

template <class... T>
class promise;

template <class... T>
class future;

template <typename... T>
class shared_future;

struct future_state_base;

/// \brief Creates a \ref future in an available, value state.
///
/// Creates a \ref future object that is already resolved.  This
/// is useful when it is determined that no I/O needs to be performed
/// to perform a computation (for example, because the data is cached
/// in some buffer).
template <typename... T, typename... A>
future<T...> make_ready_future(A&&... value);

/// \brief Creates a \ref future in an available, failed state.
///
/// Creates a \ref future object that is already resolved in a failed
/// state.  This is useful when no I/O needs to be performed to perform
/// a computation (for example, because the connection is closed and
/// we cannot read from it).
template <typename... T>
future<T...> make_exception_future(std::exception_ptr&& value) noexcept;

template <typename... T>
future<T...> make_exception_future(const std::exception_ptr& ex) noexcept {
    return make_exception_future<T...>(std::exception_ptr(ex));
}

template <typename... T>
future<T...> make_exception_future(std::exception_ptr& ex) noexcept {
    return make_exception_future<T...>(static_cast<const std::exception_ptr&>(ex));
}

/// \cond internal
void engine_exit(std::exception_ptr eptr = {});

void report_failed_future(const std::exception_ptr& ex) noexcept;

void report_failed_future(const future_state_base& state) noexcept;

/// \endcond

/// \brief Exception type for broken promises
///
/// When a promise is broken, i.e. a promise object with an attached
/// continuation is destroyed before setting any value or exception, an
/// exception of `broken_promise` type is propagated to that abandoned
/// continuation.
struct broken_promise : std::logic_error {
    broken_promise();
};

namespace internal {
template <class... T>
class promise_base_with_type;

template <typename... T>
future<T...> current_exception_as_future() noexcept;

extern template
future<> current_exception_as_future() noexcept;

// It doesn't seem to be possible to use std::tuple_element_t with an empty tuple. There is an static_assert in it that
// fails the build even if it is in the non enabled side of std::conditional.
template <typename... T>
struct get0_return_type {
    using type = void;
    static type get0(std::tuple<T...> v) { }
};

template <typename T0, typename... T>
struct get0_return_type<T0, T...> {
    using type = T0;
    static type get0(std::tuple<T0, T...> v) { return std::get<0>(std::move(v)); }
};

/// \brief Wrapper for keeping uninitialized values of non default constructible types.
///
/// This is similar to a std::optional<T>, but it doesn't know if it is holding a value or not, so the user is
/// responsible for calling constructors and destructors.
///
/// The advantage over just using a union directly is that this uses inheritance when possible and so benefits from the
/// empty base optimization.
template <typename T, bool is_trivial_class>
struct uninitialized_wrapper_base;

template <typename T>
struct uninitialized_wrapper_base<T, false> {
    union any {
        any() {}
        ~any() {}
        T value;
    } _v;

public:
    void uninitialized_set(T&& v) {
        new (&_v.value) T(std::move(v));
    }
    T& uninitialized_get() {
        return _v.value;
    }
    const T& uninitialized_get() const {
        return _v.value;
    }
};

template <typename T> struct uninitialized_wrapper_base<T, true> : private T {
    void uninitialized_set(T&& v) {
        new (this) T(std::move(v));
    }
    T& uninitialized_get() {
        return *this;
    }
    const T& uninitialized_get() const {
        return *this;
    }
};

template <typename T>
constexpr bool can_inherit =
#ifdef _LIBCPP_VERSION
// We expect std::tuple<> to be trivially constructible and
// destructible. That is not the case with libc++
// (https://bugs.llvm.org/show_bug.cgi?id=41714).  We could avoid this
// optimization when using libc++ and relax the asserts, but
// inspection suggests that std::tuple<> is trivial, it is just not
// marked as such.
        std::is_same<std::tuple<>, T>::value ||
#endif
        (std::is_trivially_destructible<T>::value && std::is_trivially_constructible<T>::value &&
                std::is_class<T>::value && !std::is_final<T>::value);

// The objective is to avoid extra space for empty types like std::tuple<>. We could use std::is_empty_v, but it is
// better to check that both the constructor and destructor can be skipped.
template <typename T>
struct uninitialized_wrapper
    : public uninitialized_wrapper_base<T, can_inherit<T>> {};

static_assert(std::is_empty<uninitialized_wrapper<std::tuple<>>>::value, "This should still be empty");

template <typename T>
struct is_trivially_move_constructible_and_destructible {
    static constexpr bool value = std::is_trivially_move_constructible<T>::value && std::is_trivially_destructible<T>::value;
};

template <bool... v>
struct all_true : std::false_type {};

template <>
struct all_true<> : std::true_type {};

template <bool... v>
struct all_true<true, v...> : public all_true<v...> {};
}

//
// A future/promise pair maintain one logical value (a future_state).
// There are up to three places that can store it, but only one is
// active at any time.
//
// - in the promise _local_state member variable
//
//   This is necessary because a promise is created first and there
//   would be nowhere else to put the value.
//
// - in the future _state variable
//
//   This is used anytime a future exists and then has not been called
//   yet. This guarantees a simple access to the value for any code
//   that already has a future.
//
// - in the task associated with the .then() clause (after .then() is called,
//   if a value was not set)
//
//
// The promise maintains a pointer to the state, which is modified as
// the state moves to a new location due to events (such as .then() or
// get_future being called) or due to the promise or future being
// moved around.
//

// non templated base class to reduce code duplication
struct future_state_base {
    static_assert(std::is_nothrow_copy_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's copy constructor must not throw");
    static_assert(std::is_nothrow_move_constructible<std::exception_ptr>::value,
                  "std::exception_ptr's move constructor must not throw");
    static_assert(sizeof(std::exception_ptr) == sizeof(void*), "exception_ptr not a pointer");
    enum class state : uintptr_t {
         invalid = 0,
         future = 1,
         // the substate is intended to decouple the run-time prevention
         // for duplicative result extraction (calling e.g. then() twice
         // ends up in abandoned()) from the wrapped object's destruction
         // handling which is orchestrated by future_state. Instead of
         // creating a temporary future_state just for the sake of setting
         // the "invalid" in the source instance, result_unavailable can
         // be set to ensure future_state_base::available() returns false.
         result_unavailable = 2,
         result = 3,
         exception_min = 4,  // or anything greater
    };
    union any {
        any() { st = state::future; }
        any(state s) { st = s; }
        void set_exception(std::exception_ptr&& e) {
            new (&ex) std::exception_ptr(std::move(e));
            assert(st >= state::exception_min);
        }
        any(std::exception_ptr&& e) {
            set_exception(std::move(e));
        }
        ~any() {}
        std::exception_ptr take_exception() {
            std::exception_ptr ret(std::move(ex));
            // Unfortunately in libstdc++ ~exception_ptr is defined out of line. We know that it does nothing for
            // moved out values, so we omit calling it. This is critical for the code quality produced for this
            // function. Without the out of line call, gcc can figure out that both sides of the if produce
            // identical code and merges them.if
            // We don't make any assumptions about other c++ libraries.
            // There is request with gcc to define it inline: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=90295
#ifndef __GLIBCXX__
            ex.~exception_ptr();
#endif
            st = state::invalid;
            return ret;
        }
        any(any&& x) {
            if (x.st < state::exception_min) {
                st = x.st;
                x.st = state::invalid;
            } else {
                new (&ex) std::exception_ptr(x.take_exception());
            }
        }
        bool has_result() const {
            return st == state::result || st == state::result_unavailable;
        }
        state st;
        std::exception_ptr ex;
    } _u;

    future_state_base() noexcept { }
    future_state_base(state st) noexcept : _u(st) { }
    future_state_base(std::exception_ptr&& ex) noexcept : _u(std::move(ex)) { }
    future_state_base(future_state_base&& x) noexcept : _u(std::move(x._u)) { }

    // We never need to destruct this polymorphicly, so we can make it
    // protected instead of virtual.
protected:
    ~future_state_base() noexcept {
        if (failed()) {
            report_failed_future(_u.take_exception());
        }
    }

public:

    bool valid() const noexcept { return _u.st != state::invalid; }
    bool available() const noexcept { return _u.st == state::result || _u.st >= state::exception_min; }
    bool failed() const noexcept { return __builtin_expect(_u.st >= state::exception_min, false); }

    void set_to_broken_promise() noexcept;

    void ignore() noexcept {
        switch (_u.st) {
        case state::invalid:
        case state::future:
            assert(0 && "invalid state for ignore");
        case state::result_unavailable:
        case state::result:
            _u.st = state::result_unavailable;
            break;
        default:
            // Ignore the exception
            _u.take_exception();
        }
    }

    void set_exception(std::exception_ptr&& ex) noexcept {
        assert(_u.st == state::future);
        _u.set_exception(std::move(ex));
    }
    future_state_base& operator=(future_state_base&& x) noexcept {
        this->~future_state_base();
        new (this) future_state_base(std::move(x));
        return *this;
    }
    void set_exception(future_state_base&& state) noexcept {
        assert(_u.st == state::future);
        *this = std::move(state);
    }
    std::exception_ptr get_exception() && noexcept {
        assert(_u.st >= state::exception_min);
        // Move ex out so future::~future() knows we've handled it
        return _u.take_exception();
    }
    const std::exception_ptr& get_exception() const& noexcept {
        assert(_u.st >= state::exception_min);
        return _u.ex;
    }

    static future_state_base current_exception();

    template <typename... U>
    friend future<U...> internal::current_exception_as_future() noexcept;
};

struct ready_future_marker {};
struct exception_future_marker {};
struct future_for_get_promise_marker {};

/// \cond internal
template <typename... T>
struct future_state :  public future_state_base, private internal::uninitialized_wrapper<std::tuple<T...>> {
    static constexpr bool copy_noexcept = std::is_nothrow_copy_constructible<std::tuple<T...>>::value;
    static constexpr bool has_trivial_move_and_destroy =
        internal::all_true<internal::is_trivially_move_constructible_and_destructible<T>::value...>::value;
    static_assert(std::is_nothrow_move_constructible<std::tuple<T...>>::value,
                  "Types must be no-throw move constructible");
    static_assert(std::is_nothrow_destructible<std::tuple<T...>>::value,
                  "Types must be no-throw destructible");
    future_state() noexcept {}
    [[gnu::always_inline]]
    future_state(future_state&& x) noexcept : future_state_base(std::move(x)) {
        if (has_trivial_move_and_destroy) {
            memcpy(reinterpret_cast<char*>(&this->uninitialized_get()),
                   &x.uninitialized_get(),
                   internal::used_size<std::tuple<T...>>::value);
        } else if (_u.has_result()) {
            this->uninitialized_set(std::move(x.uninitialized_get()));
            x.uninitialized_get().~tuple();
        }
    }
    __attribute__((always_inline))
    ~future_state() noexcept {
        if (_u.has_result()) {
            this->uninitialized_get().~tuple();
        }
    }
    future_state& operator=(future_state&& x) noexcept {
        this->~future_state();
        new (this) future_state(std::move(x));
        return *this;
    }
    template <typename... A>
    future_state(ready_future_marker, A&&... a) : future_state_base(state::result) {
        this->uninitialized_set(std::tuple<T...>(std::forward<A>(a)...));
    }
    template <typename... A>
    void set(A&&... a) {
        assert(_u.st == state::future);
        new (this) future_state(ready_future_marker(), std::forward<A>(a)...);
    }
    future_state(exception_future_marker m, std::exception_ptr&& ex) : future_state_base(std::move(ex)) { }
    future_state(exception_future_marker m, future_state_base&& state) : future_state_base(std::move(state)) { }
    std::tuple<T...>&& get_value() && noexcept {
        assert(_u.st == state::result);
        return std::move(this->uninitialized_get());
    }
    std::tuple<T...>&& take_value() && noexcept {
        assert(_u.st == state::result);
        _u.st = state::result_unavailable;
        return std::move(this->uninitialized_get());
    }
    template<typename U = std::tuple<T...>>
    const std::enable_if_t<std::is_copy_constructible<U>::value, U>& get_value() const& noexcept(copy_noexcept) {
        assert(_u.st == state::result);
        return this->uninitialized_get();
    }
    std::tuple<T...>&& take() && {
        assert(available());
        if (_u.st >= state::exception_min) {
            // Move ex out so future::~future() knows we've handled it
            std::rethrow_exception(std::move(*this).get_exception());
        }
        _u.st = state::result_unavailable;
        return std::move(this->uninitialized_get());
    }
    std::tuple<T...>&& get() && {
        assert(available());
        if (_u.st >= state::exception_min) {
            // Move ex out so future::~future() knows we've handled it
            std::rethrow_exception(std::move(*this).get_exception());
        }
        return std::move(this->uninitialized_get());
    }
    const std::tuple<T...>& get() const& {
        assert(available());
        if (_u.st >= state::exception_min) {
            std::rethrow_exception(_u.ex);
        }
        return this->uninitialized_get();
    }
    using get0_return_type = typename internal::get0_return_type<T...>::type;
    static get0_return_type get0(std::tuple<T...>&& x) {
        return internal::get0_return_type<T...>::get0(std::move(x));
    }
};

static_assert(sizeof(future_state<>) <= 8, "future_state<> is too large");
static_assert(sizeof(future_state<long>) <= 16, "future_state<long> is too large");

template <typename... T>
class continuation_base : public task {
protected:
    future_state<T...> _state;
    using future_type = future<T...>;
    using promise_type = promise<T...>;
public:
    continuation_base() = default;
    explicit continuation_base(future_state<T...>&& state) : _state(std::move(state)) {}
    void set_state(future_state<T...>&& state) {
        _state = std::move(state);
    }
    friend class internal::promise_base_with_type<T...>;
    friend class promise<T...>;
    friend class future<T...>;
};

template <typename Func, typename... T>
struct continuation final : continuation_base<T...> {
    continuation(Func&& func, future_state<T...>&& state) : continuation_base<T...>(std::move(state)), _func(std::move(func)) {}
    continuation(Func&& func) : _func(std::move(func)) {}
    virtual void run_and_dispose() noexcept override {
        _func(std::move(this->_state));
        delete this;
    }
    Func _func;
};

namespace internal {

template <typename... T>
future<T...> make_exception_future(future_state_base&& state) noexcept;

template <typename... T, typename U>
void set_callback(future<T...>& fut, U* callback) noexcept;

class future_base;

class promise_base {
protected:
    enum class urgent { no, yes };
    future_base* _future = nullptr;

    // This points to the future_state that is currently being
    // used. See comment above the future_state struct definition for
    // details.
    future_state_base* _state;

    task* _task = nullptr;

    promise_base(const promise_base&) = delete;
    promise_base(future_state_base* state) noexcept : _state(state) {}
    promise_base(future_base* future, future_state_base* state) noexcept;
    promise_base(promise_base&& x) noexcept;

    // We never need to destruct this polymorphicly, so we can make it
    // protected instead of virtual
    ~promise_base() noexcept;

    void operator=(const promise_base&) = delete;
    promise_base& operator=(promise_base&& x) = delete;

    template<urgent Urgent>
    void make_ready() noexcept;

    template<typename T>
    void set_exception_impl(T&& val) noexcept {
        if (_state) {
            _state->set_exception(std::move(val));
            make_ready<urgent::no>();
        } else {
            // We get here if promise::get_future is called and the
            // returned future is destroyed without creating a
            // continuation.
            // In older versions of seastar we would store a local
            // copy of ex and warn in the promise destructor.
            // Since there isn't any way for the user to clear
            // the exception, we issue the warning from here.
            report_failed_future(val);
        }
    }

    void set_exception(future_state_base&& state) noexcept {
        set_exception_impl(std::move(state));
    }

    void set_exception(std::exception_ptr&& ex) noexcept {
        set_exception_impl(std::move(ex));
    }

    void set_exception(const std::exception_ptr& ex) noexcept {
        set_exception(std::exception_ptr(ex));
    }

    template<typename Exception>
    std::enable_if_t<!std::is_same<std::remove_reference_t<Exception>, std::exception_ptr>::value, void> set_exception(Exception&& e) noexcept {
        set_exception(make_exception_ptr(std::forward<Exception>(e)));
    }

    friend class future_base;
    template <typename... U> friend class seastar::future;
};

/// \brief A promise with type but no local data.
///
/// This is a promise without any local data. We use this for when the
/// future is created first, so we know the promise always has an
/// external place to point to. We cannot just use promise_base
/// because we need to know the type that is being stored.
template <typename... T>
class promise_base_with_type : protected internal::promise_base {
protected:
    future_state<T...>* get_state() {
        return static_cast<future_state<T...>*>(_state);
    }
    static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
public:
    promise_base_with_type(future_state_base* state) noexcept : promise_base(state) { }
    promise_base_with_type(future<T...>* future) noexcept : promise_base(future, &future->_state) { }
    promise_base_with_type(promise_base_with_type&& x) noexcept : promise_base(std::move(x)) { }
    promise_base_with_type(const promise_base_with_type&) = delete;
    promise_base_with_type& operator=(promise_base_with_type&& x) noexcept {
        this->~promise_base_with_type();
        new (this) promise_base_with_type(std::move(x));
        return *this;
    }
    void operator=(const promise_base_with_type&) = delete;

    void set_urgent_state(future_state<T...>&& state) noexcept {
        if (_state) {
            *get_state() = std::move(state);
            make_ready<urgent::yes>();
        }
    }

    template <typename... A>
    void set_value(A&&... a) {
        if (auto *s = get_state()) {
            s->set(std::forward<A>(a)...);
            make_ready<urgent::no>();
        }
    }

#if SEASTAR_COROUTINES_TS
    void set_coroutine(future_state<T...>& state, task& coroutine) noexcept {
        _state = &state;
        _task = &coroutine;
    }
#endif
private:
    template <typename Func>
    void schedule(Func&& func) noexcept {
        auto tws = new continuation<Func, T...>(std::move(func));
        _state = &tws->_state;
        _task = tws;
    }
    void schedule(continuation_base<T...>* callback) noexcept {
        _state = &callback->_state;
        _task = callback;
    }

    template <typename... U>
    friend class seastar::future;

    friend struct seastar::future_state<T...>;
};
}
/// \endcond

/// \brief promise - allows a future value to be made available at a later time.
///
/// \tparam T A list of types to be carried as the result of the associated future.
///           A list with two or more types is deprecated; use
///           \c promise<std::tuple<T...>> instead.
template <typename... T>
class promise : private internal::promise_base_with_type<T...> {
    future_state<T...> _local_state;

public:
    /// \brief Constructs an empty \c promise.
    ///
    /// Creates promise with no associated future yet (see get_future()).
    promise() noexcept : internal::promise_base_with_type<T...>(&_local_state) {}

    /// \brief Moves a \c promise object.
    promise(promise&& x) noexcept;
    promise(const promise&) = delete;
    promise& operator=(promise&& x) noexcept {
        this->~promise();
        new (this) promise(std::move(x));
        return *this;
    }
    void operator=(const promise&) = delete;

    /// \brief Gets the promise's associated future.
    ///
    /// The future and promise will be remember each other, even if either or
    /// both are moved.  When \c set_value() or \c set_exception() are called
    /// on the promise, the future will be become ready, and if a continuation
    /// was attached to the future, it will run.
    future<T...> get_future() noexcept;

    /// \brief Sets the promises value
    ///
    /// Forwards the arguments and makes them available to the associated
    /// future.  May be called either before or after \c get_future().
    ///
    /// The arguments can have either the types the promise is
    /// templated with, or a corresponding std::tuple. That is, given
    /// a promise<int, double>, both calls are valid:
    ///
    /// pr.set_value(42, 43.0);
    /// pr.set_value(std::tuple<int, double>(42, 43.0))
    template <typename... A>
    void set_value(A&&... a) {
        internal::promise_base_with_type<T...>::set_value(std::forward<A>(a)...);
    }

    /// \brief Marks the promise as failed
    ///
    /// Forwards the exception argument to the future and makes it
    /// available.  May be called either before or after \c get_future().
    void set_exception(std::exception_ptr&& ex) noexcept {
        internal::promise_base::set_exception(std::move(ex));
    }

    void set_exception(const std::exception_ptr& ex) noexcept {
        internal::promise_base::set_exception(ex);
    }

    /// \brief Marks the promise as failed
    ///
    /// Forwards the exception argument to the future and makes it
    /// available.  May be called either before or after \c get_future().
    template<typename Exception>
    std::enable_if_t<!std::is_same<std::remove_reference_t<Exception>, std::exception_ptr>::value, void> set_exception(Exception&& e) noexcept {
        internal::promise_base::set_exception(std::forward<Exception>(e));
    }

    using internal::promise_base_with_type<T...>::set_urgent_state;

    template <typename... U>
    friend class future;
};

/// \brief Specialization of \c promise<void>
///
/// This is an alias for \c promise<>, for generic programming purposes.
/// For example, You may have a \c promise<T> where \c T can legally be
/// \c void.
template<>
class promise<void> : public promise<> {};

/// @}

/// \addtogroup future-util
/// @{


/// \brief Check whether a type is a future
///
/// This is a type trait evaluating to \c true if the given type is a
/// future.
///
template <typename... T> struct is_future : std::false_type {};

/// \cond internal
/// \addtogroup future-util
template <typename... T> struct is_future<future<T...>> : std::true_type {};

/// \endcond


/// \brief Converts a type to a future type, if it isn't already.
///
/// \return Result in member type 'type'.
template <typename T>
struct futurize;

template <typename T>
struct futurize {
    /// If \c T is a future, \c T; otherwise \c future<T>
    using type = future<T>;
    /// The promise type associated with \c type.
    using promise_type = promise<T>;
    /// The value tuple type associated with \c type
    using value_type = std::tuple<T>;

    /// Apply a function to an argument list (expressed as a tuple)
    /// and return the result, as a future (if it wasn't already).
    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    /// Apply a function to an argument list
    /// and return the result, as a future (if it wasn't already).
    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

    /// Convert a value or a future to a future
    static inline type convert(T&& value) { return make_ready_future<T>(std::move(value)); }
    static inline type convert(type&& value) { return std::move(value); }

    /// Convert the tuple representation into a future
    static type from_tuple(value_type&& value);
    /// Convert the tuple representation into a future
    static type from_tuple(const value_type& value);

    /// Makes an exceptional future of type \ref type.
    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};

/// \cond internal
template <>
struct futurize<void> {
    using type = future<>;
    using promise_type = promise<>;
    using value_type = std::tuple<>;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

    static inline type from_tuple(value_type&& value);
    static inline type from_tuple(const value_type& value);

    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};

template <typename... Args>
struct futurize<future<Args...>> {
    using type = future<Args...>;
    using promise_type = promise<Args...>;
    using value_type = std::tuple<Args...>;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept;

    template<typename Func, typename... FuncArgs>
    static inline type apply(Func&& func, FuncArgs&&... args) noexcept;

    static inline type from_tuple(value_type&& value);
    static inline type from_tuple(const value_type& value);

    static inline type convert(Args&&... values) { return make_ready_future<Args...>(std::move(values)...); }
    static inline type convert(type&& value) { return std::move(value); }

    template <typename Arg>
    static type make_exception_future(Arg&& arg);
};
/// \endcond

// Converts a type to a future type, if it isn't already.
template <typename T>
using futurize_t = typename futurize<T>::type;

/// @}

GCC6_CONCEPT(

template <typename T>
concept bool Future = is_future<T>::value;

template <typename Func, typename... T>
concept bool CanApply = requires (Func f, T... args) {
    f(std::forward<T>(args)...);
};

template <typename Func, typename Return, typename... T>
concept bool ApplyReturns = requires (Func f, T... args) {
    { f(std::forward<T>(args)...) } -> Return;
};

template <typename Func, typename... T>
concept bool ApplyReturnsAnyFuture = requires (Func f, T... args) {
    requires is_future<decltype(f(std::forward<T>(args)...))>::value;
};

)

/// \addtogroup future-module
/// @{
namespace internal {
class future_base {
protected:
    promise_base* _promise;
    future_base() noexcept : _promise(nullptr) {}
    future_base(promise_base* promise, future_state_base* state) noexcept : _promise(promise) {
        _promise->_future = this;
        _promise->_state = state;
    }

    future_base(future_base&& x, future_state_base* state) noexcept : _promise(x._promise) {
        if (auto* p = _promise) {
            x.detach_promise();
            p->_future = this;
            p->_state = state;
        }
    }
    ~future_base() noexcept {
        if (_promise) {
            detach_promise();
        }
    }

    promise_base* detach_promise() noexcept {
        _promise->_state = nullptr;
        _promise->_future = nullptr;
        return std::exchange(_promise, nullptr);
    }

    friend class promise_base;
};

template <bool IsVariadic>
struct warn_variadic_future {
    // Non-varidic case, do nothing
    void check_deprecation() {}
};


// Note: placing the deprecated attribute on the class specialization has no effect.
template <>
struct warn_variadic_future<true> {
    // Variadic case, has deprecation attribute
    [[deprecated("Variadic future<> with more than one template parmeter is deprecated, replace with future<std::tuple<...>>")]]
    void check_deprecation() {}
};

}

/// \brief A representation of a possibly not-yet-computed value.
///
/// A \c future represents a value that has not yet been computed
/// (an asynchronous computation).  It can be in one of several
/// states:
///    - unavailable: the computation has not been completed yet
///    - value: the computation has been completed successfully and a
///      value is available.
///    - failed: the computation completed with an exception.
///
/// methods in \c future allow querying the state and, most importantly,
/// scheduling a \c continuation to be executed when the future becomes
/// available.  Only one such continuation may be scheduled.
///
/// A \ref future should not be discarded before it is waited upon and
/// its result is extracted. Discarding a \ref future means that the
/// computed value becomes inaccessible, but more importantly, any
/// exceptions raised from the computation will disappear unchecked as
/// well. Another very important consequence is potentially unbounded
/// resource consumption due to the launcher of the deserted
/// continuation not being able track the amount of in-progress
/// continuations, nor their individual resource consumption.
/// To prevent accidental discarding of futures, \ref future is
/// declared `[[nodiscard]]` if the compiler supports it. Also, when a
/// discarded \ref future resolves with an error a warning is logged
/// (at runtime).
/// That said there can be legitimate cases where a \ref future is
/// discarded. The most prominent example is launching a new
/// [fiber](\ref fiber-module), or in other words, moving a continuation
/// chain to the background (off the current [fiber](\ref fiber-module)).
/// Even if a \ref future is discarded purposefully, it is still strongly
/// advisable to wait on it indirectly (via a \ref gate or
/// \ref semaphore), control their concurrency, their resource consumption
/// and handle any errors raised from them.
///
/// \tparam T A list of types to be carried as the result of the future,
///           similar to \c std::tuple<T...>. An empty list (\c future<>)
///           means that there is no result, and an available future only
///           contains a success/failure indication (and in the case of a
///           failure, an exception).
///           A list with two or more types is deprecated; use
///           \c future<std::tuple<T...>> instead.
template <typename... T>
class SEASTAR_NODISCARD future : private internal::future_base, internal::warn_variadic_future<(sizeof...(T) > 1)> {
    future_state<T...> _state;
    static constexpr bool copy_noexcept = future_state<T...>::copy_noexcept;
private:
    // This constructor creates a future that is not ready but has no
    // associated promise yet. The use case is to have a less flexible
    // but more efficient future/promise pair where we know that
    // promise::set_value cannot possibly be called without a matching
    // future and so that promise doesn't need to store a
    // future_state.
    future(future_for_get_promise_marker m) { }

    future(promise<T...>* pr) noexcept : future_base(pr, &_state), _state(std::move(pr->_local_state)) { }
    template <typename... A>
    future(ready_future_marker m, A&&... a) : _state(m, std::forward<A>(a)...) { }
    future(exception_future_marker m, std::exception_ptr&& ex) noexcept : _state(m, std::move(ex)) { }
    future(exception_future_marker m, future_state_base&& state) noexcept : _state(m, std::move(state)) { }
    [[gnu::always_inline]]
    explicit future(future_state<T...>&& state) noexcept
            : _state(std::move(state)) {
        this->check_deprecation();
    }
    internal::promise_base_with_type<T...> get_promise() noexcept {
        assert(!_promise);
        return internal::promise_base_with_type<T...>(this);
    }
    internal::promise_base_with_type<T...>* detach_promise() {
        return static_cast<internal::promise_base_with_type<T...>*>(future_base::detach_promise());
    }
    template <typename Func>
    void schedule(Func&& func) noexcept {
        if (_state.available() || !_promise) {
            if (__builtin_expect(!_state.available() && !_promise, false)) {
                _state.set_to_broken_promise();
            }
            ::seastar::schedule(new continuation<Func, T...>(std::move(func), std::move(_state)));
        } else {
            assert(_promise);
            detach_promise()->schedule(std::move(func));
            _state._u.st = future_state_base::state::invalid;
        }
    }

    [[gnu::always_inline]]
    future_state<T...>&& get_available_state_ref() noexcept {
        if (_promise) {
            detach_promise();
        }
        return std::move(_state);
    }

    [[gnu::noinline]]
    future<T...> rethrow_with_nested() {
        if (!failed()) {
            return internal::current_exception_as_future<T...>();
        } else {
            //
            // Encapsulate the current exception into the
            // std::nested_exception because the current libstdc++
            // implementation has a bug requiring the value of a
            // std::throw_with_nested() parameter to be of a polymorphic
            // type.
            //
            std::nested_exception f_ex;
            try {
                get();
            } catch (...) {
                std::throw_with_nested(f_ex);
            }
            __builtin_unreachable();
        }
    }

    template<typename... U>
    friend class shared_future;
public:
    /// \brief The data type carried by the future.
    using value_type = std::tuple<T...>;
    /// \brief The data type carried by the future.
    using promise_type = promise<T...>;
    /// \brief Moves the future into a new object.
    [[gnu::always_inline]]
    future(future&& x) noexcept : future_base(std::move(x), &_state), _state(std::move(x._state)) { }
    future(const future&) = delete;
    future& operator=(future&& x) noexcept {
        this->~future();
        new (this) future(std::move(x));
        return *this;
    }
    void operator=(const future&) = delete;
    /// \brief gets the value returned by the computation
    ///
    /// Requires that the future be available.  If the value
    /// was computed successfully, it is returned (as an
    /// \c std::tuple).  Otherwise, an exception is thrown.
    ///
    /// If get() is called in a \ref seastar::thread context,
    /// then it need not be available; instead, the thread will
    /// be paused until the future becomes available.
    [[gnu::always_inline]]
    std::tuple<T...>&& get() {
        if (!_state.available()) {
            do_wait();
        }
        return get_available_state_ref().take();
    }

    [[gnu::always_inline]]
     std::exception_ptr get_exception() {
        return get_available_state_ref().get_exception();
    }

    /// Gets the value returned by the computation.
    ///
    /// Similar to \ref get(), but instead of returning a
    /// tuple, returns the first value of the tuple.  This is
    /// useful for the common case of a \c future<T> with exactly
    /// one type parameter.
    ///
    /// Equivalent to: \c std::get<0>(f.get()).
    typename future_state<T...>::get0_return_type get0() {
        return future_state<T...>::get0(get());
    }

    /// Wait for the future to be available (in a seastar::thread)
    ///
    /// When called from a seastar::thread, this function blocks the
    /// thread until the future is availble. Other threads and
    /// continuations continue to execute; only the thread is blocked.
    void wait() noexcept {
        if (!_state.available()) {
            do_wait();
        }
    }
private:
    class thread_wake_task final : public continuation_base<T...> {
        thread_context* _thread;
        future* _waiting_for;
    public:
        thread_wake_task(thread_context* thread, future* waiting_for)
                : _thread(thread), _waiting_for(waiting_for) {
        }
        virtual void run_and_dispose() noexcept override {
            _waiting_for->_state = std::move(this->_state);
            thread_impl::switch_in(_thread);
            // no need to delete, since this is always allocated on
            // _thread's stack.
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
        detach_promise()->schedule(static_cast<continuation_base<T...>*>(&wake_task));
        thread_impl::switch_out(thread);
    }

public:
    /// \brief Checks whether the future is available.
    ///
    /// \return \c true if the future has a value, or has failed.
    [[gnu::always_inline]]
    bool available() const noexcept {
        return _state.available();
    }

    /// \brief Checks whether the future has failed.
    ///
    /// \return \c true if the future is availble and has failed.
    [[gnu::always_inline]]
    bool failed() const noexcept {
        return _state.failed();
    }

    /// \brief Schedule a block of code to run when the future is ready.
    ///
    /// Schedules a function (often a lambda) to run when the future becomes
    /// available.  The function is called with the result of this future's
    /// computation as parameters.  The return value of the function becomes
    /// the return value of then(), itself as a future; this allows then()
    /// calls to be chained.
    ///
    /// If the future failed, the function is not called, and the exception
    /// is propagated into the return value of then().
    ///
    /// \param func - function to be called when the future becomes available,
    ///               unless it has failed.
    /// \return a \c future representing the return value of \c func, applied
    ///         to the eventual value of this future.
    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
    GCC6_CONCEPT( requires ::seastar::CanApply<Func, T...> )
    Result
    then(Func&& func) noexcept {
#ifndef SEASTAR_TYPE_ERASE_MORE
        return then_impl(std::move(func));
#else
        using futurator = futurize<std::result_of_t<Func(T&&...)>>;
        return then_impl(noncopyable_function<Result (T&&...)>([func = std::forward<Func>(func)] (T&&... args) mutable {
            return futurator::apply(func, std::forward_as_tuple(std::move(args)...));
        }));
#endif
    }

private:

    template <typename Func, typename Result = futurize_t<std::result_of_t<Func(T&&...)>>>
    Result
    then_impl(Func&& func) noexcept {
        using futurator = futurize<std::result_of_t<Func(T&&...)>>;
        if (available() && !need_preempt()) {
            if (failed()) {
                return futurator::make_exception_future(static_cast<future_state_base&&>(get_available_state_ref()));
            } else {
                return futurator::apply(std::forward<Func>(func), get_available_state_ref().take_value());
            }
        }
        typename futurator::type fut(future_for_get_promise_marker{});
        // If there is a std::bad_alloc in schedule() there is nothing that can be done about it, we cannot break future
        // chain by returning ready future while 'this' future is not ready. The noexcept will call std::terminate if
        // that happens.
        [&] () noexcept {
            memory::disable_failure_guard dfg;
            schedule([pr = fut.get_promise(), func = std::forward<Func>(func)] (future_state<T...>&& state) mutable {
                if (state.failed()) {
                    pr.set_exception(static_cast<future_state_base&&>(std::move(state)));
                } else {
                    futurator::apply(std::forward<Func>(func), std::move(state).get_value()).forward_to(std::move(pr));
                }
            });
        } ();
        return fut;
    }

public:
    /// \brief Schedule a block of code to run when the future is ready, allowing
    ///        for exception handling.
    ///
    /// Schedules a function (often a lambda) to run when the future becomes
    /// available.  The function is called with the this future as a parameter;
    /// it will be in an available state.  The return value of the function becomes
    /// the return value of then_wrapped(), itself as a future; this allows
    /// then_wrapped() calls to be chained.
    ///
    /// Unlike then(), the function will be called for both value and exceptional
    /// futures.
    ///
    /// \param func - function to be called when the future becomes available,
    /// \return a \c future representing the return value of \c func, applied
    ///         to the eventual value of this future.
    template <typename Func, typename FuncResult = std::result_of_t<Func(future)>>
    GCC6_CONCEPT( requires ::seastar::CanApply<Func, future> )
    futurize_t<FuncResult>
    then_wrapped(Func&& func) & noexcept {
        return then_wrapped_maybe_erase<false, FuncResult>(std::forward<Func>(func));
    }

    template <typename Func, typename FuncResult = std::result_of_t<Func(future&&)>>
    GCC6_CONCEPT( requires ::seastar::CanApply<Func, future&&> )
    futurize_t<FuncResult>
    then_wrapped(Func&& func) && noexcept {
        return then_wrapped_maybe_erase<true, FuncResult>(std::forward<Func>(func));
    }

private:

    template <bool AsSelf, typename FuncResult, typename Func>
    futurize_t<FuncResult>
    then_wrapped_maybe_erase(Func&& func) noexcept {
#ifndef SEASTAR_TYPE_ERASE_MORE
        return then_wrapped_common<AsSelf, FuncResult>(std::forward<Func>(func));
#else
        using futurator = futurize<FuncResult>;
        return then_wrapped_common<AsSelf, FuncResult>(noncopyable_function<typename futurator::type (future&&)>([func = std::forward<Func>(func)] (future&& f) mutable {
            return futurator::apply(std::forward<Func>(func), std::move(f));
        }));
#endif
    }

    template <bool AsSelf, typename FuncResult, typename Func>
    futurize_t<FuncResult>
    then_wrapped_common(Func&& func) noexcept {
        using futurator = futurize<FuncResult>;
        if (available() && !need_preempt()) {
            // TODO: after dropping C++14 support use `if constexpr ()` instead.
            if (AsSelf) {
                if (_promise) {
                    detach_promise();
                }
                return futurator::apply(std::forward<Func>(func), std::move(*this));
            } else {
                return futurator::apply(std::forward<Func>(func), future(get_available_state_ref()));
            }
        }
        typename futurator::type fut(future_for_get_promise_marker{});
        // If there is a std::bad_alloc in schedule() there is nothing that can be done about it, we cannot break future
        // chain by returning ready future while 'this' future is not ready. The noexcept will call std::terminate if
        // that happens.
        [&] () noexcept {
            memory::disable_failure_guard dfg;
            schedule([pr = fut.get_promise(), func = std::forward<Func>(func)] (future_state<T...>&& state) mutable {
                futurator::apply(std::forward<Func>(func), future(std::move(state))).forward_to(std::move(pr));
            });
        } ();
        return fut;
    }

    void forward_to(internal::promise_base_with_type<T...>&& pr) noexcept {
        if (_state.available()) {
            pr.set_urgent_state(std::move(_state));
        } else {
            *detach_promise() = std::move(pr);
        }
    }

public:
    /// \brief Satisfy some \ref promise object with this future as a result.
    ///
    /// Arranges so that when this future is resolve, it will be used to
    /// satisfy an unrelated promise.  This is similar to scheduling a
    /// continuation that moves the result of this future into the promise
    /// (using promise::set_value() or promise::set_exception(), except
    /// that it is more efficient.
    ///
    /// \param pr a promise that will be fulfilled with the results of this
    /// future.
    void forward_to(promise<T...>&& pr) noexcept {
        if (_state.available()) {
            pr.set_urgent_state(std::move(_state));
        } else if (&pr._local_state != pr._state) {
            // The only case when _state points to _local_state is
            // when get_future was never called. Given that pr will
            // soon be destroyed, we know get_future will never be
            // called and we can just ignore this request.
            *detach_promise() = std::move(pr);
        }
    }



    /**
     * Finally continuation for statements that require waiting for the result.
     * I.e. you need to "finally" call a function that returns a possibly
     * unavailable future. The returned future will be "waited for", any
     * exception generated will be propagated, but the return value is ignored.
     * I.e. the original return value (the future upon which you are making this
     * call) will be preserved.
     *
     * If the original return value or the callback return value is an
     * exceptional future it will be propagated.
     *
     * If both of them are exceptional - the std::nested_exception exception
     * with the callback exception on top and the original future exception
     * nested will be propagated.
     */
    template <typename Func>
    GCC6_CONCEPT( requires ::seastar::CanApply<Func> )
    future<T...> finally(Func&& func) noexcept {
        return then_wrapped(finally_body<Func, is_future<std::result_of_t<Func()>>::value>(std::forward<Func>(func)));
    }


    template <typename Func, bool FuncReturnsFuture>
    struct finally_body;

    template <typename Func>
    struct finally_body<Func, true> {
        Func _func;

        finally_body(Func&& func) : _func(std::forward<Func>(func))
        { }

        future<T...> operator()(future<T...>&& result) {
            using futurator = futurize<std::result_of_t<Func()>>;
            return futurator::apply(_func).then_wrapped([result = std::move(result)](auto f_res) mutable {
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

    template <typename Func>
    struct finally_body<Func, false> {
        Func _func;

        finally_body(Func&& func) : _func(std::forward<Func>(func))
        { }

        future<T...> operator()(future<T...>&& result) {
            try {
                _func();
                return std::move(result);
            } catch (...) {
                return result.rethrow_with_nested();
            }
        };
    };

    /// \brief Terminate the program if this future fails.
    ///
    /// Terminates the entire program is this future resolves
    /// to an exception.  Use with caution.
    future<> or_terminate() noexcept {
        return then_wrapped([] (auto&& f) {
            try {
                f.get();
            } catch (...) {
                engine_exit(std::current_exception());
            }
        });
    }

    /// \brief Discards the value carried by this future.
    ///
    /// Converts the future into a no-value \c future<>, by
    /// ignoring any result.  Exceptions are propagated unchanged.
    future<> discard_result() noexcept {
        return then([] (T&&...) {});
    }

    /// \brief Handle the exception carried by this future.
    ///
    /// When the future resolves, if it resolves with an exception,
    /// handle_exception(func) replaces the exception with the value
    /// returned by func. The exception is passed (as a std::exception_ptr)
    /// as a parameter to func; func may return the replacement value
    /// immediately (T or std::tuple<T...>) or in the future (future<T...>)
    /// and is even allowed to return (or throw) its own exception.
    ///
    /// The idiom fut.discard_result().handle_exception(...) can be used
    /// to handle an exception (if there is one) without caring about the
    /// successful value; Because handle_exception() is used here on a
    /// future<>, the handler function does not need to return anything.
    template <typename Func>
    /* Broken?
    GCC6_CONCEPT( requires ::seastar::ApplyReturns<Func, future<T...>, std::exception_ptr>
                    || (sizeof...(T) == 0 && ::seastar::ApplyReturns<Func, void, std::exception_ptr>)
                    || (sizeof...(T) == 1 && ::seastar::ApplyReturns<Func, T..., std::exception_ptr>)
    ) */
    future<T...> handle_exception(Func&& func) noexcept {
        using func_ret = std::result_of_t<Func(std::exception_ptr)>;
        return then_wrapped([func = std::forward<Func>(func)]
                             (auto&& fut) mutable -> future<T...> {
            if (!fut.failed()) {
                return make_ready_future<T...>(fut.get());
            } else {
                return futurize<func_ret>::apply(func, fut.get_exception());
            }
        });
    }

    /// \brief Handle the exception of a certain type carried by this future.
    ///
    /// When the future resolves, if it resolves with an exception of a type that
    /// provided callback receives as a parameter, handle_exception(func) replaces
    /// the exception with the value returned by func. The exception is passed (by
    /// reference) as a parameter to func; func may return the replacement value
    /// immediately (T or std::tuple<T...>) or in the future (future<T...>)
    /// and is even allowed to return (or throw) its own exception.
    /// If exception, that future holds, does not match func parameter type
    /// it is propagated as is.
    template <typename Func>
    future<T...> handle_exception_type(Func&& func) noexcept {
        using trait = function_traits<Func>;
        static_assert(trait::arity == 1, "func can take only one parameter");
        using ex_type = typename trait::template arg<0>::type;
        using func_ret = typename trait::return_type;
        return then_wrapped([func = std::forward<Func>(func)]
                             (auto&& fut) mutable -> future<T...> {
            try {
                return make_ready_future<T...>(fut.get());
            } catch(ex_type& ex) {
                return futurize<func_ret>::apply(func, ex);
            }
        });
    }

    /// \brief Ignore any result hold by this future
    ///
    /// Ignore any result (value or exception) hold by this future.
    /// Use with caution since usually ignoring exception is not what
    /// you want
    void ignore_ready_future() noexcept {
        _state.ignore();
    }

#if SEASTAR_COROUTINES_TS
    void set_coroutine(task& coroutine) noexcept {
        assert(!_state.available());
        assert(_promise);
        detach_promise()->set_coroutine(_state, coroutine);
    }
#endif
private:
    void set_callback(continuation_base<T...>* callback) noexcept {
        if (_state.available()) {
            callback->set_state(get_available_state_ref());
            ::seastar::schedule(callback);
        } else {
            assert(_promise);
            detach_promise()->schedule(callback);
        }

    }

    /// \cond internal
    template <typename... U>
    friend class future;
    template <typename... U>
    friend class promise;
    template <typename... U>
    friend class internal::promise_base_with_type;
    template <typename... U, typename... A>
    friend future<U...> make_ready_future(A&&... value);
    template <typename... U>
    friend future<U...> make_exception_future(std::exception_ptr&& ex) noexcept;
    template <typename... U, typename Exception>
    friend future<U...> make_exception_future(Exception&& ex) noexcept;
    template <typename... U>
    friend future<U...> internal::make_exception_future(future_state_base&& state) noexcept;
    template <typename... U, typename V>
    friend void internal::set_callback(future<U...>&, V*) noexcept;
    /// \endcond
};

inline internal::promise_base::promise_base(future_base* future, future_state_base* state) noexcept
    : _future(future), _state(state) {
    _future->_promise = this;
}

template <typename... T>
inline
future<T...>
promise<T...>::get_future() noexcept {
    assert(!this->_future && this->_state && !this->_task);
    return future<T...>(this);
}

template <typename... T>
inline
promise<T...>::promise(promise&& x) noexcept : internal::promise_base_with_type<T...>(std::move(x)) {
    if (this->_state == &x._local_state) {
        this->_state = &_local_state;
        _local_state = std::move(x._local_state);
    }
}

template <typename... T, typename... A>
inline
future<T...> make_ready_future(A&&... value) {
    return future<T...>(ready_future_marker(), std::forward<A>(value)...);
}

template <typename... T>
inline
future<T...> make_exception_future(std::exception_ptr&& ex) noexcept {
    return future<T...>(exception_future_marker(), std::move(ex));
}

template <typename... T>
inline
future<T...> internal::make_exception_future(future_state_base&& state) noexcept {
    return future<T...>(exception_future_marker(), std::move(state));
}

template <typename... T>
future<T...> internal::current_exception_as_future() noexcept {
    return internal::make_exception_future<T...>(future_state_base::current_exception());
}

void log_exception_trace() noexcept;

/// \brief Creates a \ref future in an available, failed state.
///
/// Creates a \ref future object that is already resolved in a failed
/// state.  This no I/O needs to be performed to perform a computation
/// (for example, because the connection is closed and we cannot read
/// from it).
template <typename... T, typename Exception>
inline
future<T...> make_exception_future(Exception&& ex) noexcept {
    log_exception_trace();
    return make_exception_future<T...>(std::make_exception_ptr(std::forward<Exception>(ex)));
}

/// @}

/// \cond internal

template<typename T>
template<typename Func, typename... FuncArgs>
typename futurize<T>::type futurize<T>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    try {
        return convert(::seastar::apply(std::forward<Func>(func), std::move(args)));
    } catch (...) {
        return internal::current_exception_as_future<T>();
    }
}

template<typename T>
template<typename Func, typename... FuncArgs>
typename futurize<T>::type futurize<T>::apply(Func&& func, FuncArgs&&... args) noexcept {
    try {
        return convert(func(std::forward<FuncArgs>(args)...));
    } catch (...) {
        return internal::current_exception_as_future<T>();
    }
}

template <typename Ret>  // Ret = void | future<>
struct do_void_futurize_helper;

template <>
struct do_void_futurize_helper<void> {
    template <typename Func, typename... FuncArgs>
    static future<> apply(Func&& func, FuncArgs&&... args) noexcept {
        try {
            func(std::forward<FuncArgs>(args)...);
            return make_ready_future<>();
        } catch (...) {
            return internal::current_exception_as_future<>();
        }
    }

    template<typename Func, typename... FuncArgs>
    static future<> apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
        try {
            ::seastar::apply(std::forward<Func>(func), std::move(args));
            return make_ready_future<>();
        } catch (...) {
            return internal::current_exception_as_future<>();
        }
    }
};

template <>
struct do_void_futurize_helper<future<>> {
    template <typename Func, typename... FuncArgs>
    static future<> apply(Func&& func, FuncArgs&&... args) noexcept {
        try {
            return func(std::forward<FuncArgs>(args)...);
        } catch (...) {
            return internal::current_exception_as_future<>();
        }
    }

    template<typename Func, typename... FuncArgs>
    static future<> apply_tuple(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
        try {
            return ::seastar::apply(std::forward<Func>(func), std::move(args));
        } catch (...) {
            return internal::current_exception_as_future<>();
        }
    }
};

template <typename Func, typename... FuncArgs>
using void_futurize_helper = do_void_futurize_helper<std::result_of_t<Func(FuncArgs&&...)>>;

template<typename Func, typename... FuncArgs>
typename futurize<void>::type futurize<void>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    return void_futurize_helper<Func, FuncArgs...>::apply_tuple(std::forward<Func>(func), std::move(args));
}

template<typename Func, typename... FuncArgs>
typename futurize<void>::type futurize<void>::apply(Func&& func, FuncArgs&&... args) noexcept {
    return void_futurize_helper<Func, FuncArgs...>::apply(std::forward<Func>(func), std::forward<FuncArgs>(args)...);
}

template<typename... Args>
template<typename Func, typename... FuncArgs>
typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(Func&& func, std::tuple<FuncArgs...>&& args) noexcept {
    try {
        return ::seastar::apply(std::forward<Func>(func), std::move(args));
    } catch (...) {
        return internal::current_exception_as_future<Args...>();
    }
}

template<typename... Args>
template<typename Func, typename... FuncArgs>
typename futurize<future<Args...>>::type futurize<future<Args...>>::apply(Func&& func, FuncArgs&&... args) noexcept {
    try {
        return func(std::forward<FuncArgs>(args)...);
    } catch (...) {
        return internal::current_exception_as_future<Args...>();
    }
}

template <typename T>
template <typename Arg>
inline
future<T>
futurize<T>::make_exception_future(Arg&& arg) {
    using ::seastar::make_exception_future;
    using ::seastar::internal::make_exception_future;
    return make_exception_future<T>(std::forward<Arg>(arg));
}

template <typename... T>
template <typename Arg>
inline
future<T...>
futurize<future<T...>>::make_exception_future(Arg&& arg) {
    using ::seastar::make_exception_future;
    using ::seastar::internal::make_exception_future;
    return make_exception_future<T...>(std::forward<Arg>(arg));
}

template <typename Arg>
inline
future<>
futurize<void>::make_exception_future(Arg&& arg) {
    using ::seastar::make_exception_future;
    using ::seastar::internal::make_exception_future;
    return make_exception_future<>(std::forward<Arg>(arg));
}

template <typename T>
inline
future<T>
futurize<T>::from_tuple(std::tuple<T>&& value) {
    return make_ready_future<T>(std::move(value));
}

template <typename T>
inline
future<T>
futurize<T>::from_tuple(const std::tuple<T>& value) {
    return make_ready_future<T>(value);
}

inline
future<>
futurize<void>::from_tuple(std::tuple<>&& value) {
    return make_ready_future<>();
}

inline
future<>
futurize<void>::from_tuple(const std::tuple<>& value) {
    return make_ready_future<>();
}

template <typename... Args>
inline
future<Args...>
futurize<future<Args...>>::from_tuple(std::tuple<Args...>&& value) {
    return make_ready_future<Args...>(std::move(value));
}

template <typename... Args>
inline
future<Args...>
futurize<future<Args...>>::from_tuple(const std::tuple<Args...>& value) {
    return make_ready_future<Args...>(value);
}

template<typename Func, typename... Args>
auto futurize_apply(Func&& func, Args&&... args) {
    using futurator = futurize<std::result_of_t<Func(Args&&...)>>;
    return futurator::apply(std::forward<Func>(func), std::forward<Args>(args)...);
}

namespace internal {

template <typename... T, typename U>
inline
void set_callback(future<T...>& fut, U* callback) noexcept {
    // It would be better to use continuation_base<T...> for U, but
    // then a derived class of continuation_base<T...> won't be matched
    return fut.set_callback(callback);
}

}


/// \endcond

}
#include <fcntl.h>

namespace seastar {

enum class open_flags {
    rw = O_RDWR,
    ro = O_RDONLY,
    wo = O_WRONLY,
    create = O_CREAT,
    truncate = O_TRUNC,
    exclusive = O_EXCL,
    dsync = O_DSYNC,
};

inline open_flags operator|(open_flags a, open_flags b) {
    return open_flags(std::underlying_type_t<open_flags>(a) | std::underlying_type_t<open_flags>(b));
}

inline void operator|=(open_flags& a, open_flags b) {
    a = (a | b);
}

inline open_flags operator&(open_flags a, open_flags b) {
    return open_flags(std::underlying_type_t<open_flags>(a) & std::underlying_type_t<open_flags>(b));
}

inline void operator&=(open_flags& a, open_flags b) {
    a = (a & b);
}

/// \addtogroup fileio-module
/// @{

/// Enumeration describing the type of a directory entry being listed.
///
/// \see file::list_directory()
enum class directory_entry_type {
    unknown,
    block_device,
    char_device,
    directory,
    fifo,
    link,
    regular,
    socket,
};

/// Enumeration describing the type of a particular filesystem
enum class fs_type {
    other,
    xfs,
    ext2,
    ext3,
    ext4,
    btrfs,
    hfs,
    tmpfs,
};

// Access flags for files/directories
enum class access_flags {
    exists = F_OK,
    read = R_OK,
    write = W_OK,
    execute = X_OK,

    // alias for directory access
    lookup = execute,
};

inline access_flags operator|(access_flags a, access_flags b) {
    return access_flags(std::underlying_type_t<access_flags>(a) | std::underlying_type_t<access_flags>(b));
}

inline access_flags operator&(access_flags a, access_flags b) {
    return access_flags(std::underlying_type_t<access_flags>(a) & std::underlying_type_t<access_flags>(b));
}

// Permissions for files/directories
enum class file_permissions {
    user_read = S_IRUSR,        // Read by owner
    user_write = S_IWUSR,       // Write by owner
    user_execute = S_IXUSR,     // Execute by owner

    group_read = S_IRGRP,       // Read by group
    group_write = S_IWGRP,      // Write by group
    group_execute = S_IXGRP,    // Execute by group

    others_read = S_IROTH,      // Read by others
    others_write = S_IWOTH,     // Write by others
    others_execute = S_IXOTH,   // Execute by others

    user_permissions = user_read | user_write | user_execute,
    group_permissions = group_read | group_write | group_execute,
    others_permissions = others_read | others_write | others_execute,
    all_permissions = user_permissions | group_permissions | others_permissions,

    default_file_permissions = user_read | user_write | group_read | group_write | others_read | others_write, // 0666
    default_dir_permissions = all_permissions, // 0777
};

inline constexpr file_permissions operator|(file_permissions a, file_permissions b) {
    return file_permissions(std::underlying_type_t<file_permissions>(a) | std::underlying_type_t<file_permissions>(b));
}

inline constexpr file_permissions operator&(file_permissions a, file_permissions b) {
    return file_permissions(std::underlying_type_t<file_permissions>(a) & std::underlying_type_t<file_permissions>(b));
}

/// @}

} // namespace seastar


namespace seastar {

/// \addtogroup utilities
/// @{

/// \brief Type-safe boolean
///
/// bool_class objects are type-safe boolean values that cannot be implicitly
/// casted to untyped bools, integers or different bool_class types while still
/// provides all relevant logical and comparison operators.
///
/// bool_class template parameter is a tag type that is going to be used to
/// distinguish booleans of different types.
///
/// Usage examples:
/// \code
/// struct foo_tag { };
/// using foo = bool_class<foo_tag>;
///
/// struct bar_tag { };
/// using bar = bool_class<bar_tag>;
///
/// foo v1 = foo::yes; // OK
/// bar v2 = foo::yes; // ERROR, no implicit cast
/// foo v4 = v1 || foo::no; // OK
/// bar v5 = bar::yes && bar(true); // OK
/// bool v6 = v5; // ERROR, no implicit cast
/// \endcode
///
/// \tparam Tag type used as a tag
template<typename Tag>
class bool_class {
    bool _value;
public:
    static const bool_class yes;
    static const bool_class no;

    /// Constructs a bool_class object initialised to \c false.
    constexpr bool_class() noexcept : _value(false) { }

    /// Constructs a bool_class object initialised to \c v.
    constexpr explicit bool_class(bool v) noexcept : _value(v) { }

    /// Casts a bool_class object to an untyped \c bool.
    explicit operator bool() const noexcept { return _value; }

    /// Logical OR.
    friend bool_class operator||(bool_class x, bool_class y) noexcept {
        return bool_class(x._value || y._value);
    }

    /// Logical AND.
    friend bool_class operator&&(bool_class x, bool_class y) noexcept {
        return bool_class(x._value && y._value);
    }

    /// Logical NOT.
    friend bool_class operator!(bool_class x) noexcept {
        return bool_class(!x._value);
    }

    /// Equal-to operator.
    friend bool operator==(bool_class x, bool_class y) noexcept {
        return x._value == y._value;
    }

    /// Not-equal-to operator.
    friend bool operator!=(bool_class x, bool_class y) noexcept {
        return x._value != y._value;
    }

    /// Prints bool_class value to an output stream.
    friend std::ostream& operator<<(std::ostream& os, bool_class v) {
        return os << (v._value ? "true" : "false");
    }
};

template<typename Tag>
const bool_class<Tag> bool_class<Tag>::yes { true };
template<typename Tag>
const bool_class<Tag> bool_class<Tag>::no { false };

/// @}

}

// For IDEs that don't see SEASTAR_API_LEVEL, generate a nice default
#ifndef SEASTAR_API_LEVEL
#define SEASTAR_API_LEVEL 2
#endif

#if SEASTAR_API_LEVEL >= 2

#define SEASTAR_INCLUDE_API_V2 inline
#define SEASTAR_INCLUDE_API_V1

#else

#define SEASTAR_INCLUDE_API_V2
#define SEASTAR_INCLUDE_API_V1 inline

#endif

namespace seastar {

// iostream.hh
template <class CharType> class input_stream;
template <class CharType> class output_stream;

// reactor.hh
SEASTAR_INCLUDE_API_V2 namespace api_v2 { class server_socket; }

#if SEASTAR_API_LEVEL <= 1

SEASTAR_INCLUDE_API_V1 namespace api_v1 { class server_socket; }

#endif

class connected_socket;
class socket_address;
struct listen_options;
enum class transport;

// file.hh
class file;
struct file_open_options;
struct stat_data;

// Networking API

/// \defgroup networking-module Networking
///
/// Seastar provides a simple networking API, backed by two
/// TCP/IP stacks: the POSIX stack, utilizing the kernel's
/// BSD socket APIs, and the native stack, implement fully
/// within seastar and able to drive network cards directly.
/// The native stack supports zero-copy on both transmit
/// and receive, and is implemented using seastar's high
/// performance, lockless sharded design.  The network stack
/// can be selected with the \c \--network-stack command-line
/// parameter.

/// \addtogroup networking-module
/// @{

/// Listen for connections on a given port
///
/// Starts listening on a given address for incoming connections.
///
/// \param sa socket address to listen on
///
/// \return \ref server_socket object ready to accept connections.
///
/// \see listen(socket_address sa, listen_options opts)
server_socket listen(socket_address sa);

/// Listen for connections on a given port
///
/// Starts listening on a given address for incoming connections.
///
/// \param sa socket address to listen on
/// \param opts options controlling the listen operation
///
/// \return \ref server_socket object ready to accept connections.
///
/// \see listen(socket_address sa)
server_socket listen(socket_address sa, listen_options opts);

/// Establishes a connection to a given address
///
/// Attempts to connect to the given address.
///
/// \param sa socket address to connect to
///
/// \return a \ref connected_socket object, or an exception
future<connected_socket> connect(socket_address sa);

/// Establishes a connection to a given address
///
/// Attempts to connect to the given address with a defined local endpoint
///
/// \param sa socket address to connect to
/// \param local socket address for local endpoint
/// \param proto transport protocol (TCP or SCTP)
///
/// \return a \ref connected_socket object, or an exception
future<connected_socket> connect(socket_address sa, socket_address local, transport proto);

/// @}

/// \defgroup fileio-module File Input/Output
///
/// Seastar provides a file API to deal with persistent storage.
/// Unlike most file APIs, seastar offers unbuffered file I/O
/// (similar to, and based on, \c O_DIRECT).  Unbuffered I/O means
/// that the application is required to do its own caching, but
/// delivers better performance if this caching is done correctly.
///
/// For random I/O or sequential unbuffered I/O, the \ref file
/// class provides a set of methods for reading, writing, discarding,
/// or otherwise manipulating a file.  For buffered sequential I/O,
/// see \ref make_file_input_stream() and \ref make_file_output_stream().

/// \addtogroup fileio-module
/// @{

/// Opens or creates a file.  The "dma" in the name refers to the fact
/// that data transfers are unbuffered and uncached.
///
/// \param name  the name of the file to open or create
/// \param flags various flags controlling the open process
/// \return a \ref file object, as a future
///
/// \note
/// The file name is not guaranteed to be stable on disk, unless the
/// containing directory is sync'ed.
///
/// \relates file
future<file> open_file_dma(sstring name, open_flags flags);

/// Opens or creates a file.  The "dma" in the name refers to the fact
/// that data transfers are unbuffered and uncached.
///
/// \param name  the name of the file to open or create
/// \param flags various flags controlling the open process
/// \param options options for opening the file
/// \return a \ref file object, as a future
///
/// \note
/// The file name is not guaranteed to be stable on disk, unless the
/// containing directory is sync'ed.
///
/// \relates file
future<file> open_file_dma(sstring name, open_flags flags, file_open_options options);

/// Checks if a given directory supports direct io
///
/// Seastar bypasses the Operating System caches and issues direct io to the
/// underlying block devices. Projects using seastar should check if the directory
/// lies in a filesystem that support such operations. This function can be used
/// to do that.
///
/// It will return if direct io can be used, or throw an std::system_error
/// exception, with the EINVAL error code.
///
/// A std::system_error with the respective error code is also thrown if \ref path is
/// not a directory.
///
/// \param path the directory we need to verify.
future<> check_direct_io_support(sstring path);

/// Opens a directory.
///
/// \param name name of the directory to open
///
/// \return a \ref file object representing a directory.  The only
///    legal operations are \ref file::list_directory(),
///    \ref file::fsync(), and \ref file::close().
///
/// \relates file
future<file> open_directory(sstring name);

/// Creates a new directory.
///
/// \param name name of the directory to create
/// \param permissions optional file permissions of the directory to create.
///
/// \note
/// The directory is not guaranteed to be stable on disk, unless the
/// containing directory is sync'ed.
future<> make_directory(sstring name, file_permissions permissions = file_permissions::default_dir_permissions);

/// Ensures a directory exists
///
/// Checks whether a directory exists, and if not, creates it.  Only
/// the last component of the directory name is created.
///
/// \param name name of the directory to potentially create
/// \param permissions optional file permissions of the directory to create.
///
/// \note
/// The directory is not guaranteed to be stable on disk, unless the
/// containing directory is sync'ed.
/// If the directory exists, the provided permissions are not applied.
future<> touch_directory(sstring name, file_permissions permissions = file_permissions::default_dir_permissions);

/// Recursively ensures a directory exists
///
/// Checks whether each component of a directory exists, and if not, creates it.
///
/// \param name name of the directory to potentially create
/// \param permissions optional file permissions of the directory to create.
///
/// \note
/// This function fsyncs each component created, and is therefore guaranteed to be stable on disk.
/// The provided permissions are applied only on the last component in the path, if it needs to be created,
/// if intermediate directories do not exist, they are created with the default_dir_permissions.
/// If any directory exists, the provided permissions are not applied.
future<> recursive_touch_directory(sstring name, file_permissions permissions = file_permissions::default_dir_permissions);

/// Synchronizes a directory to disk
///
/// Makes sure the modifications in a directory are synchronized in disk.
/// This is useful, for instance, after creating or removing a file inside the
/// directory.
///
/// \param name name of the directory to potentially create
future<> sync_directory(sstring name);


/// Removes (unlinks) a file or an empty directory
///
/// \param name name of the file or the directory to remove
///
/// \note
/// The removal is not guaranteed to be stable on disk, unless the
/// containing directory is sync'ed.
future<> remove_file(sstring name);

/// Renames (moves) a file.
///
/// \param old_name existing file name
/// \param new_name new file name
///
/// \note
/// The rename is not guaranteed to be stable on disk, unless the
/// both containing directories are sync'ed.
future<> rename_file(sstring old_name, sstring new_name);

struct follow_symlink_tag { };
using follow_symlink = bool_class<follow_symlink_tag>;

/// Return stat information about a file.
///
/// \param name name of the file to return its stat information
/// \param follow_symlink follow symbolic links.
///
/// \return stat_data of the file identified by name.
/// If name identifies a symbolic link then stat_data is returned either for the target of the link,
/// with follow_symlink::yes, or for the link itself, with follow_symlink::no.
future<stat_data> file_stat(sstring name, follow_symlink fs = follow_symlink::yes);

/// Return the size of a file.
///
/// \param name name of the file to return the size
///
/// Note that file_size of a symlink is NOT the size of the symlink -
/// which is the length of the pathname it contains -
/// but rather the size of the file to which it points.
future<uint64_t> file_size(sstring name);

/// Check file access.
///
/// \param name name of the file to check
/// \param flags bit pattern containing type of access to check (read/write/execute or exists).
///
/// If only access_flags::exists is queried, returns true if the file exists, or false otherwise.
/// Throws a compat::filesystem::filesystem_error exception if any error other than ENOENT is encountered.
///
/// If any of the access_flags (read/write/execute) is set, returns true if the file exists and is
/// accessible with the requested flags, or false if the file exists and is not accessible
/// as queried.
/// Throws a compat::filesystem::filesystem_error exception if any error other than EACCES is encountered.
/// Note that if any path component leading to the file is not searchable, the file is considered inaccessible
/// with the requested mode and false will be returned.
future<bool> file_accessible(sstring name, access_flags flags);

/// check if a file exists.
///
/// \param name name of the file to check
future<bool> file_exists(sstring name);

/// Creates a hard link for a file
///
/// \param oldpath existing file name
/// \param newpath name of link
///
future<> link_file(sstring oldpath, sstring newpath);

/// Changes the permissions mode of a file or directory
///
/// \param name name of the file ot directory to change
/// \param permissions permissions to set
///
future<> chmod(sstring name, file_permissions permissions);

/// Return information about the filesystem where a file is located.
///
/// \param name name of the file to inspect
future<fs_type> file_system_at(sstring name);

/// Return space available to unprivileged users in filesystem where a file is located, in bytes.
///
/// \param name name of the file to inspect
future<uint64_t> fs_avail(sstring name);

/// Return free space in filesystem where a file is located, in bytes.
///
/// \param name name of the file to inspect
future<uint64_t> fs_free(sstring name);
/// @}

}

namespace seastar {

namespace net {

enum class ip_protocol_num : uint8_t {
    icmp = 1, tcp = 6, udp = 17, unused = 255
};

enum class eth_protocol_num : uint16_t {
    ipv4 = 0x0800, arp = 0x0806, ipv6 = 0x86dd
};

const uint8_t eth_hdr_len = 14;
const uint8_t tcp_hdr_len_min = 20;
const uint8_t ipv4_hdr_len_min = 20;
const uint8_t ipv6_hdr_len_min = 40;
const uint16_t ip_packet_len_max = 65535;

}

}

namespace seastar {

namespace net {

struct fragment {
    char* base;
    size_t size;
};

struct offload_info {
    ip_protocol_num protocol = ip_protocol_num::unused;
    bool needs_csum = false;
    uint8_t ip_hdr_len = 20;
    uint8_t tcp_hdr_len = 20;
    uint8_t udp_hdr_len = 8;
    bool needs_ip_csum = false;
    bool reassembled = false;
    uint16_t tso_seg_size = 0;
    // HW stripped VLAN header (CPU order)
    compat::optional<uint16_t> vlan_tci;
};

// Zero-copy friendly packet class
//
// For implementing zero-copy, we need a flexible destructor that can
// destroy packet data in different ways: decrementing a reference count,
// or calling a free()-like function.
//
// Moreover, we need different destructors for each set of fragments within
// a single fragment. For example, a header and trailer might need delete[]
// to be called, while the internal data needs a reference count to be
// released.  Matters are complicated in that fragments can be split
// (due to virtual/physical translation).
//
// To implement this, we associate each packet with a single destructor,
// but allow composing a packet from another packet plus a fragment to
// be added, with its own destructor, causing the destructors to be chained.
//
// The downside is that the data needed for the destructor is duplicated,
// if it is already available in the fragment itself.
//
// As an optimization, when we allocate small fragments, we allocate some
// extra space, so prepending to the packet does not require extra
// allocations.  This is useful when adding headers.
//
class packet final {
    // enough for lots of headers, not quite two cache lines:
    static constexpr size_t internal_data_size = 128 - 16;
    static constexpr size_t default_nr_frags = 4;

    struct pseudo_vector {
        fragment* _start;
        fragment* _finish;
        pseudo_vector(fragment* start, size_t nr)
            : _start(start), _finish(_start + nr) {}
        fragment* begin() { return _start; }
        fragment* end() { return _finish; }
        fragment& operator[](size_t idx) { return _start[idx]; }
    };

    struct impl {
        // when destroyed, virtual destructor will reclaim resources
        deleter _deleter;
        unsigned _len = 0;
        uint16_t _nr_frags = 0;
        uint16_t _allocated_frags;
        offload_info _offload_info;
        compat::optional<uint32_t> _rss_hash;
        char _data[internal_data_size]; // only _frags[0] may use
        unsigned _headroom = internal_data_size; // in _data
        // FIXME: share _data/_frags space

        fragment _frags[];

        impl(size_t nr_frags = default_nr_frags);
        impl(const impl&) = delete;
        impl(fragment frag, size_t nr_frags = default_nr_frags);

        pseudo_vector fragments() { return { _frags, _nr_frags }; }

        static std::unique_ptr<impl> allocate(size_t nr_frags) {
            nr_frags = std::max(nr_frags, default_nr_frags);
            return std::unique_ptr<impl>(new (nr_frags) impl(nr_frags));
        }

        static std::unique_ptr<impl> copy(impl* old, size_t nr) {
            auto n = allocate(nr);
            n->_deleter = std::move(old->_deleter);
            n->_len = old->_len;
            n->_nr_frags = old->_nr_frags;
            n->_headroom = old->_headroom;
            n->_offload_info = old->_offload_info;
            n->_rss_hash = old->_rss_hash;
            std::copy(old->_frags, old->_frags + old->_nr_frags, n->_frags);
            old->copy_internal_fragment_to(n.get());
            return n;
        }

        static std::unique_ptr<impl> copy(impl* old) {
            return copy(old, old->_nr_frags);
        }

        static std::unique_ptr<impl> allocate_if_needed(std::unique_ptr<impl> old, size_t extra_frags) {
            if (old->_allocated_frags >= old->_nr_frags + extra_frags) {
                return old;
            }
            return copy(old.get(), std::max<size_t>(old->_nr_frags + extra_frags, 2 * old->_nr_frags));
        }
        void* operator new(size_t size, size_t nr_frags = default_nr_frags) {
            assert(nr_frags == uint16_t(nr_frags));
            return ::operator new(size + nr_frags * sizeof(fragment));
        }
        // Matching the operator new above
        void operator delete(void* ptr, size_t nr_frags) {
            return ::operator delete(ptr);
        }
        // Since the above "placement delete" hides the global one, expose it
        void operator delete(void* ptr) {
            return ::operator delete(ptr);
        }

        bool using_internal_data() const {
            return _nr_frags
                    && _frags[0].base >= _data
                    && _frags[0].base < _data + internal_data_size;
        }

        void unuse_internal_data() {
            if (!using_internal_data()) {
                return;
            }
            auto buf = static_cast<char*>(::malloc(_frags[0].size));
            if (!buf) {
                throw std::bad_alloc();
            }
            deleter d = make_free_deleter(buf);
            std::copy(_frags[0].base, _frags[0].base + _frags[0].size, buf);
            _frags[0].base = buf;
            d.append(std::move(_deleter));
            _deleter = std::move(d);
            _headroom = internal_data_size;
        }
        void copy_internal_fragment_to(impl* to) {
            if (!using_internal_data()) {
                return;
            }
            to->_frags[0].base = to->_data + _headroom;
            std::copy(_frags[0].base, _frags[0].base + _frags[0].size,
                    to->_frags[0].base);
        }
    };
    packet(std::unique_ptr<impl>&& impl) : _impl(std::move(impl)) {}
    std::unique_ptr<impl> _impl;
public:
    static packet from_static_data(const char* data, size_t len) {
        return {fragment{const_cast<char*>(data), len}, deleter()};
    }

    // build empty packet
    packet();
    // build empty packet with nr_frags allocated
    packet(size_t nr_frags);
    // move existing packet
    packet(packet&& x) noexcept;
    // copy data into packet
    packet(const char* data, size_t len);
    // copy data into packet
    packet(fragment frag);
    // zero-copy single fragment
    packet(fragment frag, deleter del);
    // zero-copy multiple fragments
    packet(std::vector<fragment> frag, deleter del);
    // build packet with iterator
    template <typename Iterator>
    packet(Iterator begin, Iterator end, deleter del);
    // append fragment (copying new fragment)
    packet(packet&& x, fragment frag);
    // prepend fragment (copying new fragment, with header optimization)
    packet(fragment frag, packet&& x);
    // prepend fragment (zero-copy)
    packet(fragment frag, deleter del, packet&& x);
    // append fragment (zero-copy)
    packet(packet&& x, fragment frag, deleter d);
    // append temporary_buffer (zero-copy)
    packet(packet&& x, temporary_buffer<char> buf);
    // create from temporary_buffer (zero-copy)
    packet(temporary_buffer<char> buf);
    // append deleter
    packet(packet&& x, deleter d);

    packet& operator=(packet&& x) {
        if (this != &x) {
            this->~packet();
            new (this) packet(std::move(x));
        }
        return *this;
    }

    unsigned len() const { return _impl->_len; }
    unsigned memory() const { return len() +  sizeof(packet::impl); }

    fragment frag(unsigned idx) const { return _impl->_frags[idx]; }
    fragment& frag(unsigned idx) { return _impl->_frags[idx]; }

    unsigned nr_frags() const { return _impl->_nr_frags; }
    pseudo_vector fragments() const { return { _impl->_frags, _impl->_nr_frags }; }
    fragment* fragment_array() const { return _impl->_frags; }

    // share packet data (reference counted, non COW)
    packet share();
    packet share(size_t offset, size_t len);

    void append(packet&& p);

    void trim_front(size_t how_much);
    void trim_back(size_t how_much);

    // get a header pointer, linearizing if necessary
    template <typename Header>
    Header* get_header(size_t offset = 0);

    // get a header pointer, linearizing if necessary
    char* get_header(size_t offset, size_t size);

    // prepend a header (default-initializing it)
    template <typename Header>
    Header* prepend_header(size_t extra_size = 0);

    // prepend a header (uninitialized!)
    char* prepend_uninitialized_header(size_t size);

    packet free_on_cpu(unsigned cpu, std::function<void()> cb = []{});

    void linearize() { return linearize(0, len()); }

    void reset() { _impl.reset(); }

    void reserve(int n_frags) {
        if (n_frags > _impl->_nr_frags) {
            auto extra = n_frags - _impl->_nr_frags;
            _impl = impl::allocate_if_needed(std::move(_impl), extra);
        }
    }
    compat::optional<uint32_t> rss_hash() {
        return _impl->_rss_hash;
    }
    compat::optional<uint32_t> set_rss_hash(uint32_t hash) {
        return _impl->_rss_hash = hash;
    }
    // Call `func` for each fragment, avoiding data copies when possible
    // `func` is called with a temporary_buffer<char> parameter
    template <typename Func>
    void release_into(Func&& func) {
        unsigned idx = 0;
        if (_impl->using_internal_data()) {
            auto&& f = frag(idx++);
            func(temporary_buffer<char>(f.base, f.size));
        }
        while (idx < nr_frags()) {
            auto&& f = frag(idx++);
            func(temporary_buffer<char>(f.base, f.size, _impl->_deleter.share()));
        }
    }
    std::vector<temporary_buffer<char>> release() {
        std::vector<temporary_buffer<char>> ret;
        ret.reserve(_impl->_nr_frags);
        release_into([&ret] (temporary_buffer<char>&& frag) {
            ret.push_back(std::move(frag));
        });
        return ret;
    }
    explicit operator bool() {
        return bool(_impl);
    }
    static packet make_null_packet() {
        return net::packet(nullptr);
    }
private:
    void linearize(size_t at_frag, size_t desired_size);
    bool allocate_headroom(size_t size);
public:
    struct offload_info offload_info() const { return _impl->_offload_info; }
    struct offload_info& offload_info_ref() { return _impl->_offload_info; }
    void set_offload_info(struct offload_info oi) { _impl->_offload_info = oi; }
};

std::ostream& operator<<(std::ostream& os, const packet& p);

inline
packet::packet(packet&& x) noexcept
    : _impl(std::move(x._impl)) {
}

inline
packet::impl::impl(size_t nr_frags)
    : _len(0), _allocated_frags(nr_frags) {
}

inline
packet::impl::impl(fragment frag, size_t nr_frags)
    : _len(frag.size), _allocated_frags(nr_frags) {
    assert(_allocated_frags > _nr_frags);
    if (frag.size <= internal_data_size) {
        _headroom -= frag.size;
        _frags[0] = { _data + _headroom, frag.size };
    } else {
        auto buf = static_cast<char*>(::malloc(frag.size));
        if (!buf) {
            throw std::bad_alloc();
        }
        deleter d = make_free_deleter(buf);
        _frags[0] = { buf, frag.size };
        _deleter.append(std::move(d));
    }
    std::copy(frag.base, frag.base + frag.size, _frags[0].base);
    ++_nr_frags;
}

inline
packet::packet()
    : _impl(impl::allocate(1)) {
}

inline
packet::packet(size_t nr_frags)
    : _impl(impl::allocate(nr_frags)) {
}

inline
packet::packet(fragment frag) : _impl(new impl(frag)) {
}

inline
packet::packet(const char* data, size_t size) : packet(fragment{const_cast<char*>(data), size}) {
}

inline
packet::packet(fragment frag, deleter d)
    : _impl(impl::allocate(1)) {
    _impl->_deleter = std::move(d);
    _impl->_frags[_impl->_nr_frags++] = frag;
    _impl->_len = frag.size;
}

inline
packet::packet(std::vector<fragment> frag, deleter d)
    : _impl(impl::allocate(frag.size())) {
    _impl->_deleter = std::move(d);
    std::copy(frag.begin(), frag.end(), _impl->_frags);
    _impl->_nr_frags = frag.size();
    _impl->_len = 0;
    for (auto&& f : _impl->fragments()) {
        _impl->_len += f.size;
    }
}

template <typename Iterator>
inline
packet::packet(Iterator begin, Iterator end, deleter del) {
    unsigned nr_frags = 0, len = 0;
    nr_frags = std::distance(begin, end);
    std::for_each(begin, end, [&] (const fragment& frag) { len += frag.size; });
    _impl = impl::allocate(nr_frags);
    _impl->_deleter = std::move(del);
    _impl->_len = len;
    _impl->_nr_frags = nr_frags;
    std::copy(begin, end, _impl->_frags);
}

inline
packet::packet(packet&& x, fragment frag)
    : _impl(impl::allocate_if_needed(std::move(x._impl), 1)) {
    _impl->_len += frag.size;
    std::unique_ptr<char[]> buf(new char[frag.size]);
    std::copy(frag.base, frag.base + frag.size, buf.get());
    _impl->_frags[_impl->_nr_frags++] = {buf.get(), frag.size};
    _impl->_deleter = make_deleter(std::move(_impl->_deleter), [buf = buf.release()] {
        delete[] buf;
    });
}

inline
bool
packet::allocate_headroom(size_t size) {
    if (_impl->_headroom >= size) {
        _impl->_len += size;
        if (!_impl->using_internal_data()) {
            _impl = impl::allocate_if_needed(std::move(_impl), 1);
            std::copy_backward(_impl->_frags, _impl->_frags + _impl->_nr_frags,
                    _impl->_frags + _impl->_nr_frags + 1);
            _impl->_frags[0] = { _impl->_data + internal_data_size, 0 };
            ++_impl->_nr_frags;
        }
        _impl->_headroom -= size;
        _impl->_frags[0].base -= size;
        _impl->_frags[0].size += size;
        return true;
    } else {
        return false;
    }
}


inline
packet::packet(fragment frag, packet&& x)
    : _impl(std::move(x._impl)) {
    // try to prepend into existing internal fragment
    if (allocate_headroom(frag.size)) {
        std::copy(frag.base, frag.base + frag.size, _impl->_frags[0].base);
        return;
    } else {
        // didn't work out, allocate and copy
        _impl->unuse_internal_data();
        _impl = impl::allocate_if_needed(std::move(_impl), 1);
        _impl->_len += frag.size;
        std::unique_ptr<char[]> buf(new char[frag.size]);
        std::copy(frag.base, frag.base + frag.size, buf.get());
        std::copy_backward(_impl->_frags, _impl->_frags + _impl->_nr_frags,
                _impl->_frags + _impl->_nr_frags + 1);
        ++_impl->_nr_frags;
        _impl->_frags[0] = {buf.get(), frag.size};
        _impl->_deleter = make_deleter(std::move(_impl->_deleter),
                [buf = std::move(buf)] {});
    }
}

inline
packet::packet(packet&& x, fragment frag, deleter d)
    : _impl(impl::allocate_if_needed(std::move(x._impl), 1)) {
    _impl->_len += frag.size;
    _impl->_frags[_impl->_nr_frags++] = frag;
    d.append(std::move(_impl->_deleter));
    _impl->_deleter = std::move(d);
}

inline
packet::packet(packet&& x, deleter d)
    : _impl(std::move(x._impl)) {
    _impl->_deleter.append(std::move(d));
}

inline
packet::packet(packet&& x, temporary_buffer<char> buf)
    : packet(std::move(x), fragment{buf.get_write(), buf.size()}, buf.release()) {
}

inline
packet::packet(temporary_buffer<char> buf)
    : packet(fragment{buf.get_write(), buf.size()}, buf.release()) {}

inline
void packet::append(packet&& p) {
    if (!_impl->_len) {
        *this = std::move(p);
        return;
    }
    _impl = impl::allocate_if_needed(std::move(_impl), p._impl->_nr_frags);
    _impl->_len += p._impl->_len;
    p._impl->unuse_internal_data();
    std::copy(p._impl->_frags, p._impl->_frags + p._impl->_nr_frags,
            _impl->_frags + _impl->_nr_frags);
    _impl->_nr_frags += p._impl->_nr_frags;
    p._impl->_deleter.append(std::move(_impl->_deleter));
    _impl->_deleter = std::move(p._impl->_deleter);
}

inline
char* packet::get_header(size_t offset, size_t size) {
    if (offset + size > _impl->_len) {
        return nullptr;
    }
    size_t i = 0;
    while (i != _impl->_nr_frags && offset >= _impl->_frags[i].size) {
        offset -= _impl->_frags[i++].size;
    }
    if (i == _impl->_nr_frags) {
        return nullptr;
    }
    if (offset + size > _impl->_frags[i].size) {
        linearize(i, offset + size);
    }
    return _impl->_frags[i].base + offset;
}

template <typename Header>
inline
Header* packet::get_header(size_t offset) {
    return reinterpret_cast<Header*>(get_header(offset, sizeof(Header)));
}

inline
void packet::trim_front(size_t how_much) {
    assert(how_much <= _impl->_len);
    _impl->_len -= how_much;
    size_t i = 0;
    while (how_much && how_much >= _impl->_frags[i].size) {
        how_much -= _impl->_frags[i++].size;
    }
    std::copy(_impl->_frags + i, _impl->_frags + _impl->_nr_frags, _impl->_frags);
    _impl->_nr_frags -= i;
    if (!_impl->using_internal_data()) {
        _impl->_headroom = internal_data_size;
    }
    if (how_much) {
        if (_impl->using_internal_data()) {
            _impl->_headroom += how_much;
        }
        _impl->_frags[0].base += how_much;
        _impl->_frags[0].size -= how_much;
    }
}

inline
void packet::trim_back(size_t how_much) {
    assert(how_much <= _impl->_len);
    _impl->_len -= how_much;
    size_t i = _impl->_nr_frags - 1;
    while (how_much && how_much >= _impl->_frags[i].size) {
        how_much -= _impl->_frags[i--].size;
    }
    _impl->_nr_frags = i + 1;
    if (how_much) {
        _impl->_frags[i].size -= how_much;
        if (i == 0 && _impl->using_internal_data()) {
            _impl->_headroom += how_much;
        }
    }
}

template <typename Header>
Header*
packet::prepend_header(size_t extra_size) {
    auto h = prepend_uninitialized_header(sizeof(Header) + extra_size);
    return new (h) Header{};
}

// prepend a header (uninitialized!)
inline
char* packet::prepend_uninitialized_header(size_t size) {
    if (!allocate_headroom(size)) {
        // didn't work out, allocate and copy
        _impl->unuse_internal_data();
        // try again, after unuse_internal_data we may have space after all
        if (!allocate_headroom(size)) {
            // failed
            _impl->_len += size;
            _impl = impl::allocate_if_needed(std::move(_impl), 1);
            std::unique_ptr<char[]> buf(new char[size]);
            std::copy_backward(_impl->_frags, _impl->_frags + _impl->_nr_frags,
                    _impl->_frags + _impl->_nr_frags + 1);
            ++_impl->_nr_frags;
            _impl->_frags[0] = {buf.get(), size};
            _impl->_deleter = make_deleter(std::move(_impl->_deleter),
                    [buf = std::move(buf)] {});
        }
    }
    return _impl->_frags[0].base;
}

inline
packet packet::share() {
    return share(0, _impl->_len);
}

inline
packet packet::share(size_t offset, size_t len) {
    _impl->unuse_internal_data(); // FIXME: eliminate?
    packet n;
    n._impl = impl::allocate_if_needed(std::move(n._impl), _impl->_nr_frags);
    size_t idx = 0;
    while (offset > 0 && offset >= _impl->_frags[idx].size) {
        offset -= _impl->_frags[idx++].size;
    }
    while (n._impl->_len < len) {
        auto& f = _impl->_frags[idx++];
        auto fsize = std::min(len - n._impl->_len, f.size - offset);
        n._impl->_frags[n._impl->_nr_frags++] = { f.base + offset, fsize };
        n._impl->_len += fsize;
        offset = 0;
    }
    n._impl->_offload_info = _impl->_offload_info;
    assert(!n._impl->_deleter);
    n._impl->_deleter = _impl->_deleter.share();
    return n;
}

}

}

namespace seastar {

template <typename CharType>
class scattered_message {
private:
    using fragment = net::fragment;
    using packet = net::packet;
    using char_type = CharType;
    packet _p;
public:
    scattered_message() {}
    scattered_message(scattered_message&&) = default;
    scattered_message(const scattered_message&) = delete;

    void append_static(const char_type* buf, size_t size) {
        if (size) {
            _p = packet(std::move(_p), fragment{(char_type*)buf, size}, deleter());
        }
    }

    template <size_t N>
    void append_static(const char_type(&s)[N]) {
        append_static(s, N - 1);
    }

    void append_static(const char_type* s) {
        append_static(s, strlen(s));
    }

    template <typename size_type, size_type max_size>
    void append_static(const basic_sstring<char_type, size_type, max_size>& s) {
        append_static(s.begin(), s.size());
    }

    void append_static(const compat::string_view& s) {
        append_static(s.data(), s.size());
    }

    template <typename size_type, size_type max_size>
    void append(basic_sstring<char_type, size_type, max_size> s) {
        if (s.size()) {
            _p = packet(std::move(_p), std::move(s).release());
        }
    }

    template <typename size_type, size_type max_size, typename Callback>
    void append(const basic_sstring<char_type, size_type, max_size>& s, Callback callback) {
        if (s.size()) {
            _p = packet(std::move(_p), fragment{s.begin(), s.size()}, make_deleter(std::move(callback)));
        }
    }

    void reserve(int n_frags) {
        _p.reserve(n_frags);
    }

    packet release() && {
        return std::move(_p);
    }

    template <typename Callback>
    void on_delete(Callback callback) {
        _p = packet(std::move(_p), make_deleter(std::move(callback)));
    }

    operator bool() const {
        return _p.len();
    }

    size_t size() {
        return _p.len();
    }
};

}

namespace seastar {

namespace net { class packet; }

class data_source_impl {
public:
    virtual ~data_source_impl() {}
    virtual future<temporary_buffer<char>> get() = 0;
    virtual future<temporary_buffer<char>> skip(uint64_t n);
    virtual future<> close() { return make_ready_future<>(); }
};

class data_source {
    std::unique_ptr<data_source_impl> _dsi;
protected:
    data_source_impl* impl() const { return _dsi.get(); }
public:
    data_source() = default;
    explicit data_source(std::unique_ptr<data_source_impl> dsi) : _dsi(std::move(dsi)) {}
    data_source(data_source&& x) = default;
    data_source& operator=(data_source&& x) = default;
    future<temporary_buffer<char>> get() { return _dsi->get(); }
    future<temporary_buffer<char>> skip(uint64_t n) { return _dsi->skip(n); }
    future<> close() { return _dsi->close(); }
};

class data_sink_impl {
public:
    virtual ~data_sink_impl() {}
    virtual temporary_buffer<char> allocate_buffer(size_t size) {
        return temporary_buffer<char>(size);
    }
    virtual future<> put(net::packet data) = 0;
    virtual future<> put(std::vector<temporary_buffer<char>> data) {
        net::packet p;
        p.reserve(data.size());
        for (auto& buf : data) {
            p = net::packet(std::move(p), net::fragment{buf.get_write(), buf.size()}, buf.release());
        }
        return put(std::move(p));
    }
    virtual future<> put(temporary_buffer<char> buf) {
        return put(net::packet(net::fragment{buf.get_write(), buf.size()}, buf.release()));
    }
    virtual future<> flush() {
        return make_ready_future<>();
    }
    virtual future<> close() = 0;
};

class data_sink {
    std::unique_ptr<data_sink_impl> _dsi;
public:
    data_sink() = default;
    explicit data_sink(std::unique_ptr<data_sink_impl> dsi) : _dsi(std::move(dsi)) {}
    data_sink(data_sink&& x) = default;
    data_sink& operator=(data_sink&& x) = default;
    temporary_buffer<char> allocate_buffer(size_t size) {
        return _dsi->allocate_buffer(size);
    }
    future<> put(std::vector<temporary_buffer<char>> data) {
        return _dsi->put(std::move(data));
    }
    future<> put(temporary_buffer<char> data) {
        return _dsi->put(std::move(data));
    }
    future<> put(net::packet p) {
        return _dsi->put(std::move(p));
    }
    future<> flush() {
        return _dsi->flush();
    }
    future<> close() { return _dsi->close(); }
};

struct continue_consuming {};

template <typename CharType>
class stop_consuming {
public:
    using tmp_buf = temporary_buffer<CharType>;
    explicit stop_consuming(tmp_buf buf) : _buf(std::move(buf)) {}

    tmp_buf& get_buffer() { return _buf; }
    const tmp_buf& get_buffer() const { return _buf; }
private:
    tmp_buf _buf;
};

class skip_bytes {
public:
    explicit skip_bytes(uint64_t v) : _value(v) {}
    uint64_t get_value() const { return _value; }
private:
    uint64_t _value;
};

template <typename CharType>
class consumption_result {
public:
    using stop_consuming_type = stop_consuming<CharType>;
    using consumption_variant = compat::variant<continue_consuming, stop_consuming_type, skip_bytes>;
    using tmp_buf = typename stop_consuming_type::tmp_buf;

    /*[[deprecated]]*/ consumption_result(compat::optional<tmp_buf> opt_buf) {
        if (opt_buf) {
            _result = stop_consuming_type{std::move(opt_buf.value())};
        }
    }

    consumption_result(const continue_consuming&) {}
    consumption_result(stop_consuming_type&& stop) : _result(std::move(stop)) {}
    consumption_result(skip_bytes&& skip) : _result(std::move(skip)) {}

    consumption_variant& get() { return _result; }
    const consumption_variant& get() const { return _result; }

private:
    consumption_variant _result;
};

// Consumer concept, for consume() method
GCC6_CONCEPT(
// The consumer should operate on the data given to it, and
// return a future "consumption result", which can be
//  - continue_consuming, if the consumer has consumed all the input given
// to it and is ready for more
//  - stop_consuming, when the consumer is done (and in that case
// the contained buffer is the unconsumed part of the last data buffer - this
// can also happen to be empty).
//  - skip_bytes, when the consumer has consumed all the input given to it
// and wants to skip before processing the next chunk
//
// For backward compatibility reasons, we also support the deprecated return value
// of type "unconsumed remainder" which can be
//  - empty optional, if the consumer consumed all the input given to it
// and is ready for more
//  - non-empty optional, when the consumer is done (and in that case
// the value is the unconsumed part of the last data buffer - this
// can also happen to be empty).

template <typename Consumer, typename CharType>
concept bool InputStreamConsumer = requires (Consumer c) {
    { c(temporary_buffer<CharType>{}) } -> future<consumption_result<CharType>>;
};

template <typename Consumer, typename CharType>
concept bool ObsoleteInputStreamConsumer = requires (Consumer c) {
    { c(temporary_buffer<CharType>{}) } -> future<compat::optional<temporary_buffer<CharType>>>;
};
)

/// Buffers data from a data_source and provides a stream interface to the user.
///
/// \note All methods must be called sequentially.  That is, no method may be
/// invoked before the previous method's returned future is resolved.
template <typename CharType>
class input_stream final {
    static_assert(sizeof(CharType) == 1, "must buffer stream of bytes");
    data_source _fd;
    temporary_buffer<CharType> _buf;
    bool _eof = false;
private:
    using tmp_buf = temporary_buffer<CharType>;
    size_t available() const { return _buf.size(); }
protected:
    void reset() { _buf = {}; }
    data_source* fd() { return &_fd; }
public:
    using consumption_result_type = consumption_result<CharType>;
    // unconsumed_remainder is mapped for compatibility only; new code should use consumption_result_type
    using unconsumed_remainder = compat::optional<tmp_buf>;
    using char_type = CharType;
    input_stream() = default;
    explicit input_stream(data_source fd) : _fd(std::move(fd)), _buf(0) {}
    input_stream(input_stream&&) = default;
    input_stream& operator=(input_stream&&) = default;
    future<temporary_buffer<CharType>> read_exactly(size_t n);
    template <typename Consumer>
    GCC6_CONCEPT(requires InputStreamConsumer<Consumer, CharType> || ObsoleteInputStreamConsumer<Consumer, CharType>)
    future<> consume(Consumer&& c);
    template <typename Consumer>
    GCC6_CONCEPT(requires InputStreamConsumer<Consumer, CharType> || ObsoleteInputStreamConsumer<Consumer, CharType>)
    future<> consume(Consumer& c);
    bool eof() const { return _eof; }
    /// Returns some data from the stream, or an empty buffer on end of
    /// stream.
    future<tmp_buf> read();
    /// Returns up to n bytes from the stream, or an empty buffer on end of
    /// stream.
    future<tmp_buf> read_up_to(size_t n);
    /// Detaches the \c input_stream from the underlying data source.
    ///
    /// Waits for any background operations (for example, read-ahead) to
    /// complete, so that the any resources the stream is using can be
    /// safely destroyed.  An example is a \ref file resource used by
    /// the stream returned by make_file_input_stream().
    ///
    /// \return a future that becomes ready when this stream no longer
    ///         needs the data source.
    future<> close() {
        return _fd.close();
    }
    /// Ignores n next bytes from the stream.
    future<> skip(uint64_t n);

    /// Detaches the underlying \c data_source from the \c input_stream.
    ///
    /// The intended usage is custom \c data_source_impl implementations
    /// wrapping an existing \c input_stream, therefore it shouldn't be
    /// called on an \c input_stream that was already used.
    /// After calling \c detach() the \c input_stream is in an unusable,
    /// moved-from state.
    ///
    /// \throws std::logic_error if called on a used stream
    ///
    /// \returns the data_source
    data_source detach() &&;
private:
    future<temporary_buffer<CharType>> read_exactly_part(size_t n, tmp_buf buf, size_t completed);
};

/// Facilitates data buffering before it's handed over to data_sink.
///
/// When trim_to_size is true it's guaranteed that data sink will not receive
/// chunks larger than the configured size, which could be the case when a
/// single write call is made with data larger than the configured size.
///
/// The data sink will not receive empty chunks.
///
/// \note All methods must be called sequentially.  That is, no method
/// may be invoked before the previous method's returned future is
/// resolved.
template <typename CharType>
class output_stream final {
    static_assert(sizeof(CharType) == 1, "must buffer stream of bytes");
    data_sink _fd;
    temporary_buffer<CharType> _buf;
    net::packet _zc_bufs = net::packet::make_null_packet(); //zero copy buffers
    size_t _size = 0;
    size_t _begin = 0;
    size_t _end = 0;
    bool _trim_to_size = false;
    bool _batch_flushes = false;
    compat::optional<promise<>> _in_batch;
    bool _flush = false;
    bool _flushing = false;
    std::exception_ptr _ex;
private:
    size_t available() const { return _end - _begin; }
    size_t possibly_available() const { return _size - _begin; }
    future<> split_and_put(temporary_buffer<CharType> buf);
    future<> put(temporary_buffer<CharType> buf);
    void poll_flush();
    future<> zero_copy_put(net::packet p);
    future<> zero_copy_split_and_put(net::packet p);
    [[gnu::noinline]]
    future<> slow_write(const CharType* buf, size_t n);
public:
    using char_type = CharType;
    output_stream() = default;
    output_stream(data_sink fd, size_t size, bool trim_to_size = false, bool batch_flushes = false)
        : _fd(std::move(fd)), _size(size), _trim_to_size(trim_to_size), _batch_flushes(batch_flushes) {}
    output_stream(output_stream&&) = default;
    output_stream& operator=(output_stream&&) = default;
    ~output_stream() { assert(!_in_batch && "Was this stream properly closed?"); }
    future<> write(const char_type* buf, size_t n);
    future<> write(const char_type* buf);

    template <typename StringChar, typename SizeType, SizeType MaxSize, bool NulTerminate>
    future<> write(const basic_sstring<StringChar, SizeType, MaxSize, NulTerminate>& s);
    future<> write(const std::basic_string<char_type>& s);

    future<> write(net::packet p);
    future<> write(scattered_message<char_type> msg);
    future<> write(temporary_buffer<char_type>);
    future<> flush();

    /// Flushes the stream before closing it (and the underlying data sink) to
    /// any further writes.  The resulting future must be waited on before
    /// destroying this object.
    future<> close();

    /// Detaches the underlying \c data_sink from the \c output_stream.
    ///
    /// The intended usage is custom \c data_sink_impl implementations
    /// wrapping an existing \c output_stream, therefore it shouldn't be
    /// called on an \c output_stream that was already used.
    /// After calling \c detach() the \c output_stream is in an unusable,
    /// moved-from state.
    ///
    /// \throws std::logic_error if called on a used stream
    ///
    /// \returns the data_sink
    data_sink detach() &&;
private:
    friend class reactor;
};

/*!
 * \brief copy all the content from the input stream to the output stream
 */
template <typename CharType>
future<> copy(input_stream<CharType>&, output_stream<CharType>&);

}



namespace seastar {


/// \cond internal

namespace internal {


// Given a future type, find the corresponding continuation_base.
template <typename Future>
struct continuation_base_from_future;

template <typename... T>
struct continuation_base_from_future<future<T...>> {
    using type = continuation_base<T...>;
};

template <typename HeldState, typename Future>
class do_with_state final : public continuation_base_from_future<Future>::type {
    HeldState _held;
    typename Future::promise_type _pr;
public:
    explicit do_with_state(HeldState&& held) : _held(std::move(held)) {}
    virtual void run_and_dispose() noexcept override {
        _pr.set_urgent_state(std::move(this->_state));
        delete this;
    }
    HeldState& data() {
        return _held;
    }
    Future get_future() {
        return _pr.get_future();
    }
};

}
/// \endcond

/// \addtogroup future-util
/// @{

/// do_with() holds an object alive for the duration until a future
/// completes, and allow the code involved in making the future
/// complete to have easy access to this object.
///
/// do_with() takes two arguments: The first is an temporary object (rvalue),
/// the second is a function returning a future (a so-called "promise").
/// The function is given (a moved copy of) this temporary object, by
/// reference, and it is ensured that the object will not be destructed until
/// the completion of the future returned by the function.
///
/// do_with() returns a future which resolves to whatever value the given future
/// (returned by the given function) resolves to. This returned value must not
/// contain references to the temporary object, as at that point the temporary
/// is destructed.
///
/// \param rvalue a temporary value to protect while \c f is running
/// \param f a callable, accepting an lvalue reference of the same type
///          as \c rvalue, that will be accessible while \c f runs
/// \return whatever \c f returns
template<typename T, typename F>
inline
auto do_with(T&& rvalue, F&& f) {
    auto task = std::make_unique<internal::do_with_state<T, std::result_of_t<F(T&)>>>(std::forward<T>(rvalue));
    auto fut = f(task->data());
    if (fut.available()) {
        return fut;
    }
    auto ret = task->get_future();
    internal::set_callback(fut, task.release());
    return ret;
}

/// \cond internal
template <typename Tuple, size_t... Idx>
inline
auto
cherry_pick_tuple(std::index_sequence<Idx...>, Tuple&& tuple) {
    return std::make_tuple(std::get<Idx>(std::forward<Tuple>(tuple))...);
}
/// \endcond

/// Executes the function \c func making sure the lock \c lock is taken,
/// and later on properly released.
///
/// \param lock the lock, which is any object having providing a lock() / unlock() semantics.
///        Caller must make sure that it outlives \ref func.
/// \param func function to be executed
/// \returns whatever \c func returns
template<typename Lock, typename Func>
inline
auto with_lock(Lock& lock, Func&& func) {
    return lock.lock().then([func = std::forward<Func>(func)] () mutable {
        return func();
    }).then_wrapped([&lock] (auto&& fut) {
        lock.unlock();
        return std::move(fut);
    });
}

/// Multiple argument variant of \ref do_with(T&& rvalue, F&& f).
///
/// This is the same as \ref do_with(T&& tvalue, F&& f), but accepts
/// two or more rvalue parameters, which are held in memory while
/// \c f executes.  \c f will be called with all arguments as
/// reference parameters.
template <typename T1, typename T2, typename T3_or_F, typename... More>
inline
auto
do_with(T1&& rv1, T2&& rv2, T3_or_F&& rv3, More&&... more) {
    auto all = std::forward_as_tuple(
            std::forward<T1>(rv1),
            std::forward<T2>(rv2),
            std::forward<T3_or_F>(rv3),
            std::forward<More>(more)...);
    constexpr size_t nr = std::tuple_size<decltype(all)>::value - 1;
    using idx = std::make_index_sequence<nr>;
    auto&& just_values = cherry_pick_tuple(idx(), std::move(all));
    auto&& just_func = std::move(std::get<nr>(std::move(all)));
    using value_tuple = std::remove_reference_t<decltype(just_values)>;
    using ret_type = decltype(apply(just_func, just_values));
    auto task = std::make_unique<internal::do_with_state<value_tuple, ret_type>>(std::move(just_values));
    auto fut = apply(just_func, task->data());
    if (fut.available()) {
        return fut;
    }
    auto ret = task->get_future();
    internal::set_callback(fut, task.release());
    return ret;
}

/// @}

}
#include <boost/intrusive/list.hpp>

#include <bitset>

namespace seastar {

namespace bitsets {

static constexpr int ulong_bits = std::numeric_limits<unsigned long>::digits;

/**
 * Returns the number of leading zeros in value's binary representation.
 *
 * If value == 0 the result is undefied. If T is signed and value is negative
 * the result is undefined.
 *
 * The highest value that can be returned is std::numeric_limits<T>::digits - 1,
 * which is returned when value == 1.
 */
template<typename T>
inline size_t count_leading_zeros(T value);

/**
 * Returns the number of trailing zeros in value's binary representation.
 *
 * If value == 0 the result is undefied. If T is signed and value is negative
 * the result is undefined.
 *
 * The highest value that can be returned is std::numeric_limits<T>::digits - 1.
 */
template<typename T>
static inline size_t count_trailing_zeros(T value);

template<>
inline size_t count_leading_zeros<unsigned long>(unsigned long value)
{
    return __builtin_clzl(value);
}

template<>
inline size_t count_leading_zeros<long>(long value)
{
    return __builtin_clzl((unsigned long)value) - 1;
}

template<>
inline size_t count_leading_zeros<long long>(long long value)
{
    return __builtin_clzll((unsigned long long)value) - 1;
}

template<>
inline
size_t count_trailing_zeros<unsigned long>(unsigned long value)
{
    return __builtin_ctzl(value);
}

template<>
inline
size_t count_trailing_zeros<long>(long value)
{
    return __builtin_ctzl((unsigned long)value);
}

/**
 * Returns the index of the first set bit.
 * Result is undefined if bitset.any() == false.
 */
template<size_t N>
static inline size_t get_first_set(const std::bitset<N>& bitset)
{
    static_assert(N <= ulong_bits, "bitset too large");
    return count_trailing_zeros(bitset.to_ulong());
}

/**
 * Returns the index of the last set bit in the bitset.
 * Result is undefined if bitset.any() == false.
 */
template<size_t N>
static inline size_t get_last_set(const std::bitset<N>& bitset)
{
    static_assert(N <= ulong_bits, "bitset too large");
    return ulong_bits - 1 - count_leading_zeros(bitset.to_ulong());
}

template<size_t N>
class set_iterator : public std::iterator<std::input_iterator_tag, int>
{
private:
    void advance()
    {
        if (_bitset.none()) {
            _index = -1;
        } else {
            auto shift = get_first_set(_bitset) + 1;
            _index += shift;
            _bitset >>= shift;
        }
    }
public:
    set_iterator(std::bitset<N> bitset, int offset = 0)
        : _bitset(bitset)
        , _index(offset - 1)
    {
        static_assert(N <= ulong_bits, "This implementation is inefficient for large bitsets");
        _bitset >>= offset;
        advance();
    }

    void operator++()
    {
        advance();
    }

    int operator*() const
    {
        return _index;
    }

    bool operator==(const set_iterator& other) const
    {
        return _index == other._index;
    }

    bool operator!=(const set_iterator& other) const
    {
        return !(*this == other);
    }
private:
    std::bitset<N> _bitset;
    int _index;
};

template<size_t N>
class set_range
{
public:
    using iterator = set_iterator<N>;
    using value_type = int;

    set_range(std::bitset<N> bitset, int offset = 0)
        : _bitset(bitset)
        , _offset(offset)
    {
    }

    iterator begin() const { return iterator(_bitset, _offset); }
    iterator end() const { return iterator(0); }
private:
    std::bitset<N> _bitset;
    int _offset;
};

template<size_t N>
static inline set_range<N> for_each_set(std::bitset<N> bitset, int offset = 0)
{
    return set_range<N>(bitset, offset);
}

}

}

namespace seastar {

/**
 * A data structure designed for holding and expiring timers. It's
 * optimized for timer non-delivery by deferring sorting cost until
 * expiry time. The optimization is based on the observation that in
 * many workloads timers are cancelled or rescheduled before they
 * expire. That's especially the case for TCP timers.
 *
 * The template type "Timer" should have a method named
 * get_timeout() which returns Timer::time_point which denotes
 * timer's expiration.
 */
template<typename Timer, boost::intrusive::list_member_hook<> Timer::*link>
class timer_set {
public:
    using time_point = typename Timer::time_point;
    using timer_list_t = boost::intrusive::list<Timer, boost::intrusive::member_hook<Timer, boost::intrusive::list_member_hook<>, link>>;
private:
    using duration = typename Timer::duration;
    using timestamp_t = typename Timer::duration::rep;

    static constexpr timestamp_t max_timestamp = std::numeric_limits<timestamp_t>::max();
    static constexpr int timestamp_bits = std::numeric_limits<timestamp_t>::digits;

    // The last bucket is reserved for active timers with timeout <= _last.
    static constexpr int n_buckets = timestamp_bits + 1;

    std::array<timer_list_t, n_buckets> _buckets;
    timestamp_t _last;
    timestamp_t _next;

    std::bitset<n_buckets> _non_empty_buckets;
private:
    static timestamp_t get_timestamp(time_point _time_point)
    {
        return _time_point.time_since_epoch().count();
    }

    static timestamp_t get_timestamp(Timer& timer)
    {
        return get_timestamp(timer.get_timeout());
    }

    int get_index(timestamp_t timestamp) const
    {
        if (timestamp <= _last) {
            return n_buckets - 1;
        }

        auto index = bitsets::count_leading_zeros(timestamp ^ _last);
        assert(index < n_buckets - 1);
        return index;
    }

    int get_index(Timer& timer) const
    {
        return get_index(get_timestamp(timer));
    }

    int get_last_non_empty_bucket() const
    {
        return bitsets::get_last_set(_non_empty_buckets);
    }
public:
    timer_set()
        : _last(0)
        , _next(max_timestamp)
        , _non_empty_buckets(0)
    {
    }

    ~timer_set() {
        for (auto&& list : _buckets) {
            while (!list.empty()) {
                auto& timer = *list.begin();
                timer.cancel();
            }
        }
    }

    /**
     * Adds timer to the active set.
     *
     * The value returned by timer.get_timeout() is used as timer's expiry. The result
     * of timer.get_timeout() must not change while the timer is in the active set.
     *
     * Preconditions:
     *  - this timer must not be currently in the active set or in the expired set.
     *
     * Postconditions:
     *  - this timer will be added to the active set until it is expired
     *    by a call to expire() or removed by a call to remove().
     *
     * Returns true if and only if this timer's timeout is less than get_next_timeout().
     * When this function returns true the caller should reschedule expire() to be
     * called at timer.get_timeout() to ensure timers are expired in a timely manner.
     */
    bool insert(Timer& timer)
    {
        auto timestamp = get_timestamp(timer);
        auto index = get_index(timestamp);

        _buckets[index].push_back(timer);
        _non_empty_buckets[index] = true;

        if (timestamp < _next) {
            _next = timestamp;
            return true;
        }
        return false;
    }

    /**
     * Removes timer from the active set.
     *
     * Preconditions:
     *  - timer must be currently in the active set. Note: it must not be in
     *    the expired set.
     *
     * Postconditions:
     *  - timer is no longer in the active set.
     *  - this object will no longer hold any references to this timer.
     */
    void remove(Timer& timer)
    {
        auto index = get_index(timer);
        auto& list = _buckets[index];
        list.erase(list.iterator_to(timer));
        if (list.empty()) {
            _non_empty_buckets[index] = false;
        }
    }

    /**
     * Expires active timers.
     *
     * The time points passed to this function must be monotonically increasing.
     * Use get_next_timeout() to query for the next time point.
     *
     * Preconditions:
     *  - the time_point passed to this function must not be lesser than
     *    the previous one passed to this function.
     *
     * Postconditons:
     *  - all timers from the active set with Timer::get_timeout() <= now are moved
     *    to the expired set.
     */
    timer_list_t expire(time_point now)
    {
        timer_list_t exp;
        auto timestamp = get_timestamp(now);

        if (timestamp < _last) {
            abort();
        }

        auto index = get_index(timestamp);

        for (int i : bitsets::for_each_set(_non_empty_buckets, index + 1)) {
            exp.splice(exp.end(), _buckets[i]);
            _non_empty_buckets[i] = false;
        }

        _last = timestamp;
        _next = max_timestamp;

        auto& list = _buckets[index];
        while (!list.empty()) {
            auto& timer = *list.begin();
            list.pop_front();
            if (timer.get_timeout() <= now) {
                exp.push_back(timer);
            } else {
                insert(timer);
            }
        }

        _non_empty_buckets[index] = !list.empty();

        if (_next == max_timestamp && _non_empty_buckets.any()) {
            for (auto& timer : _buckets[get_last_non_empty_bucket()]) {
                _next = std::min(_next, get_timestamp(timer));
            }
        }
        return exp;
    }

    /**
     * Returns a time point at which expire() should be called
     * in order to ensure timers are expired in a timely manner.
     *
     * Returned values are monotonically increasing.
     */
    time_point get_next_timeout() const
    {
        return time_point(duration(std::max(_last, _next)));
    }

    /**
     * Clears both active and expired timer sets.
     */
    void clear()
    {
        for (int i : bitsets::for_each_set(_non_empty_buckets)) {
            _buckets[i].clear();
        }
    }

    size_t size() const
    {
        size_t res = 0;
        for (int i : bitsets::for_each_set(_non_empty_buckets)) {
            res += _buckets[i].size();
        }
        return res;
    }

    /**
     * Returns true if and only if there are no timers in the active set.
     */
    bool empty() const
    {
        return _non_empty_buckets.none();
    }

    time_point now() {
        return Timer::clock::now();
    }
};
};

namespace seastar {

using steady_clock_type = std::chrono::steady_clock;

template <typename Clock = steady_clock_type>
class timer {
public:
    typedef typename Clock::time_point time_point;
    typedef typename Clock::duration duration;
    typedef Clock clock;
private:
    using callback_t = noncopyable_function<void()>;
    boost::intrusive::list_member_hook<> _link;
    callback_t _callback;
    time_point _expiry;
    compat::optional<duration> _period;
    bool _armed = false;
    bool _queued = false;
    bool _expired = false;
    void readd_periodic();
    void arm_state(time_point until, compat::optional<duration> period) {
        assert(!_armed);
        _period = period;
        _armed = true;
        _expired = false;
        _expiry = until;
        _queued = true;
    }
public:
    timer() = default;
    timer(timer&& t) noexcept : _callback(std::move(t._callback)), _expiry(std::move(t._expiry)), _period(std::move(t._period)),
            _armed(t._armed), _queued(t._queued), _expired(t._expired) {
        _link.swap_nodes(t._link);
        t._queued = false;
        t._armed = false;
    }
    explicit timer(callback_t&& callback) : _callback{std::move(callback)} {
    }
    ~timer();
    void set_callback(callback_t&& callback) {
        _callback = std::move(callback);
    }
    void arm(time_point until, compat::optional<duration> period = {});
    void rearm(time_point until, compat::optional<duration> period = {}) {
        if (_armed) {
            cancel();
        }
        arm(until, period);
    }
    void arm(duration delta) {
        return arm(Clock::now() + delta);
    }
    void arm_periodic(duration delta) {
        arm(Clock::now() + delta, {delta});
    }
    bool armed() const { return _armed; }
    bool cancel();
    time_point get_timeout() {
        return _expiry;
    }
    friend class reactor;
    friend class timer_set<timer, &timer::_link>;
};

extern template class timer<steady_clock_type>;

}

#include <iterator>
#include <vector>

#include <tuple>
#include <utility>

namespace seastar {

/// \cond internal
namespace internal {

template<typename Tuple>
Tuple untuple(Tuple t) {
    return t;
}

template<typename T>
T untuple(std::tuple<T> t) {
    return std::get<0>(std::move(t));
}

template<typename Tuple, typename Function, size_t... I>
void tuple_for_each_helper(Tuple&& t, Function&& f, std::index_sequence<I...>&&) {
    auto ignore_me = { (f(std::get<I>(std::forward<Tuple>(t))), 1)... };
    (void)ignore_me;
}

template<typename Tuple, typename MapFunction, size_t... I>
auto tuple_map_helper(Tuple&& t, MapFunction&& f, std::index_sequence<I...>&&) {
    return std::make_tuple(f(std::get<I>(std::forward<Tuple>(t)))...);
}

template<size_t I, typename IndexSequence>
struct prepend;

template<size_t I, size_t... Is>
struct prepend<I, std::index_sequence<Is...>> {
    using type = std::index_sequence<I, Is...>;
};

template<template<typename> class Filter, typename Tuple, typename IndexSequence>
struct tuple_filter;

template<template<typename> class Filter, typename T, typename... Ts, size_t I, size_t... Is>
struct tuple_filter<Filter, std::tuple<T, Ts...>, std::index_sequence<I, Is...>> {
    using tail = typename tuple_filter<Filter, std::tuple<Ts...>, std::index_sequence<Is...>>::type;
    using type = std::conditional_t<Filter<T>::value, typename prepend<I, tail>::type, tail>;
};

template<template<typename> class Filter>
struct tuple_filter<Filter, std::tuple<>, std::index_sequence<>> {
    using type = std::index_sequence<>;
};

template<typename Tuple, size_t... I>
auto tuple_filter_helper(Tuple&& t, std::index_sequence<I...>&&) {
    return std::make_tuple(std::get<I>(std::forward<Tuple>(t))...);
}

}
/// \endcond

/// \addtogroup utilities
/// @{

/// Applies type transformation to all types in tuple
///
/// Member type `type` is set to a tuple type which is a result of applying
/// transformation `MapClass<T>::type` to each element `T` of the input tuple
/// type.
///
/// \tparam MapClass class template defining type transformation
/// \tparam Tuple input tuple type
template<template<typename> class MapClass, typename Tuple>
struct tuple_map_types;

/// @}

template<template<typename> class MapClass, typename... Elements>
struct tuple_map_types<MapClass, std::tuple<Elements...>> {
    using type = std::tuple<typename MapClass<Elements>::type...>;
};

/// \addtogroup utilities
/// @{

/// Filters elements in tuple by their type
///
/// Returns a tuple containing only those elements which type `T` caused
/// expression `FilterClass<T>::value` to be true.
///
/// \tparam FilterClass class template having an element value set to true for elements that
///                     should be present in the result
/// \param t tuple to filter
/// \return a tuple contaning elements which type passed the test
template<template<typename> class FilterClass, typename... Elements>
auto tuple_filter_by_type(const std::tuple<Elements...>& t) {
    using sequence = typename internal::tuple_filter<FilterClass, std::tuple<Elements...>,
                                                     std::index_sequence_for<Elements...>>::type;
    return internal::tuple_filter_helper(t, sequence());
}
template<template<typename> class FilterClass, typename... Elements>
auto tuple_filter_by_type(std::tuple<Elements...>&& t) {
    using sequence = typename internal::tuple_filter<FilterClass, std::tuple<Elements...>,
                                                     std::index_sequence_for<Elements...>>::type;
    return internal::tuple_filter_helper(std::move(t), sequence());
}

/// Applies function to all elements in tuple
///
/// Applies given function to all elements in the tuple and returns a tuple
/// of results.
///
/// \param t original tuple
/// \param f function to apply
/// \return tuple of results returned by f for each element in t
template<typename Function, typename... Elements>
auto tuple_map(const std::tuple<Elements...>& t, Function&& f) {
    return internal::tuple_map_helper(t, std::forward<Function>(f),
                                      std::index_sequence_for<Elements...>());
}
template<typename Function, typename... Elements>
auto tuple_map(std::tuple<Elements...>&& t, Function&& f) {
    return internal::tuple_map_helper(std::move(t), std::forward<Function>(f),
                                      std::index_sequence_for<Elements...>());
}

/// Iterate over all elements in tuple
///
/// Iterates over given tuple and calls the specified function for each of
/// it elements.
///
/// \param t a tuple to iterate over
/// \param f function to call for each tuple element
template<typename Function, typename... Elements>
void tuple_for_each(const std::tuple<Elements...>& t, Function&& f) {
    return internal::tuple_for_each_helper(t, std::forward<Function>(f),
                                           std::index_sequence_for<Elements...>());
}
template<typename Function, typename... Elements>
void tuple_for_each(std::tuple<Elements...>& t, Function&& f) {
    return internal::tuple_for_each_helper(t, std::forward<Function>(f),
                                           std::index_sequence_for<Elements...>());
}
template<typename Function, typename... Elements>
void tuple_for_each(std::tuple<Elements...>&& t, Function&& f) {
    return internal::tuple_for_each_helper(std::move(t), std::forward<Function>(f),
                                           std::index_sequence_for<Elements...>());
}

/// @}

}

namespace seastar {

/// \cond internal
extern __thread size_t task_quota;
/// \endcond


/// \cond internal
namespace internal {

template <typename Func>
void
schedule_in_group(scheduling_group sg, Func func) {
    schedule(make_task(sg, std::move(func)));
}


}
/// \endcond

/// \addtogroup future-util
/// @{

/// \brief run a callable (with some arbitrary arguments) in a scheduling group
///
/// If the conditions are suitable (see scheduling_group::may_run_immediately()),
/// then the function is run immediately. Otherwise, the function is queued to run
/// when its scheduling group next runs.
///
/// \param sg  scheduling group that controls execution time for the function
/// \param func function to run; must be movable or copyable
/// \param args arguments to the function; may be copied or moved, so use \c std::ref()
///             to force passing references
template <typename Func, typename... Args>
inline
auto
with_scheduling_group(scheduling_group sg, Func func, Args&&... args) {
    using return_type = decltype(func(std::forward<Args>(args)...));
    using futurator = futurize<return_type>;
    if (sg.active()) {
        return futurator::apply(func, std::forward<Args>(args)...);
    } else {
        typename futurator::promise_type pr;
        auto f = pr.get_future();
        internal::schedule_in_group(sg, [pr = std::move(pr), func = std::move(func), args = std::make_tuple(std::forward<Args>(args)...)] () mutable {
            return futurator::apply(func, std::move(args)).forward_to(std::move(pr));
        });
        return f;
    }
}

namespace internal {

template <typename Iterator, typename IteratorCategory>
inline
size_t
iterator_range_estimate_vector_capacity(Iterator begin, Iterator end, IteratorCategory category) {
    // For InputIterators we can't estimate needed capacity
    return 0;
}

template <typename Iterator>
inline
size_t
iterator_range_estimate_vector_capacity(Iterator begin, Iterator end, std::forward_iterator_tag category) {
    // May be linear time below random_access_iterator_tag, but still better than reallocation
    return std::distance(begin, end);
}

}

/// \cond internal

class parallel_for_each_state final : private continuation_base<> {
    std::vector<future<>> _incomplete;
    promise<> _result;
    std::exception_ptr _ex;
private:
    // Wait for one of the futures in _incomplete to complete, and then
    // decide what to do: wait for another one, or deliver _result if all
    // are complete.
    void wait_for_one() noexcept;
    virtual void run_and_dispose() noexcept override;
public:
    parallel_for_each_state(size_t n);
    void add_future(future<>&& f);
    future<> get_future();
};

/// \endcond

/// Run tasks in parallel (iterator version).
///
/// Given a range [\c begin, \c end) of objects, run \c func on each \c *i in
/// the range, and return a future<> that resolves when all the functions
/// complete.  \c func should return a future<> that indicates when it is
/// complete.  All invocations are performed in parallel. This allows the range
/// to refer to stack objects, but means that unlike other loops this cannot
/// check need_preempt and can only be used with small ranges.
///
/// \param begin an \c InputIterator designating the beginning of the range
/// \param end an \c InputIterator designating the end of the range
/// \param func Function to apply to each element in the range (returning
///             a \c future<>)
/// \return a \c future<> that resolves when all the function invocations
///         complete.  If one or more return an exception, the return value
///         contains one of the exceptions.
template <typename Iterator, typename Func>
GCC6_CONCEPT( requires requires (Func f, Iterator i) { { f(*i++) } -> future<>; } )
inline
future<>
parallel_for_each(Iterator begin, Iterator end, Func&& func) noexcept {
    parallel_for_each_state* s = nullptr;
    // Process all elements, giving each future the following treatment:
    //   - available, not failed: do nothing
    //   - available, failed: collect exception in ex
    //   - not available: collect in s (allocating it if needed)
    while (begin != end) {
        auto f = futurize_apply(std::forward<Func>(func), *begin++);
        if (!f.available() || f.failed()) {
            if (!s) {
                using itraits = std::iterator_traits<Iterator>;
                auto n = (internal::iterator_range_estimate_vector_capacity(begin, end, typename itraits::iterator_category()) + 1);
                s = new parallel_for_each_state(n);
            }
            s->add_future(std::move(f));
        }
    }
    // If any futures were not available, hand off to parallel_for_each_state::start().
    // Otherwise we can return a result immediately.
    if (s) {
        // s->get_future() takes ownership of s (and chains it to one of the futures it contains)
        // so this isn't a leak
        return s->get_future();
    }
    return make_ready_future<>();
}

/// Run tasks in parallel (range version).
///
/// Given a \c range of objects, apply \c func to each object
/// in the range, and return a future<> that resolves when all
/// the functions complete.  \c func should return a future<> that indicates
/// when it is complete.  All invocations are performed in parallel. This allows
/// the range to refer to stack objects, but means that unlike other loops this
/// cannot check need_preempt and can only be used with small ranges.
///
/// \param range A range of objects to iterate run \c func on
/// \param func  A callable, accepting reference to the range's
///              \c value_type, and returning a \c future<>.
/// \return a \c future<> that becomes ready when the entire range
///         was processed.  If one or more of the invocations of
///         \c func returned an exceptional future, then the return
///         value will contain one of those exceptions.
template <typename Range, typename Func>
GCC6_CONCEPT( requires requires (Func f, Range r) { { f(*r.begin()) } -> future<>; } )
inline
future<>
parallel_for_each(Range&& range, Func&& func) {
    return parallel_for_each(std::begin(range), std::end(range),
            std::forward<Func>(func));
}

// The AsyncAction concept represents an action which can complete later than
// the actual function invocation. It is represented by a function which
// returns a future which resolves when the action is done.

struct stop_iteration_tag { };
using stop_iteration = bool_class<stop_iteration_tag>;

/// \cond internal

namespace internal {

template <typename AsyncAction>
class repeater final : public continuation_base<stop_iteration> {
    using futurator = futurize<std::result_of_t<AsyncAction()>>;
    promise<> _promise;
    AsyncAction _action;
public:
    explicit repeater(AsyncAction action) : _action(std::move(action)) {}
    repeater(stop_iteration si, AsyncAction action) : repeater(std::move(action)) {
        _state.set(std::make_tuple(si));
    }
    future<> get_future() { return _promise.get_future(); }
    virtual void run_and_dispose() noexcept override {
        if (_state.failed()) {
            _promise.set_exception(std::move(_state).get_exception());
            delete this;
            return;
        } else {
            if (std::get<0>(_state.get()) == stop_iteration::yes) {
                _promise.set_value();
                delete this;
                return;
            }
            _state = {};
        }
        try {
            do {
                auto f = futurator::apply(_action);
                if (!f.available()) {
                    internal::set_callback(f, this);
                    return;
                }
                if (f.get0() == stop_iteration::yes) {
                    _promise.set_value();
                    delete this;
                    return;
                }
            } while (!need_preempt());
        } catch (...) {
            _promise.set_exception(std::current_exception());
            delete this;
            return;
        }
        _state.set(stop_iteration::no);
        schedule(this);
    }
};

}

/// \endcond

/// Invokes given action until it fails or the function requests iteration to stop by returning
/// \c stop_iteration::yes.
///
/// \param action a callable taking no arguments, returning a future<stop_iteration>.  Will
///               be called again as soon as the future resolves, unless the
///               future fails, action throws, or it resolves with \c stop_iteration::yes.
///               If \c action is an r-value it can be moved in the middle of iteration.
/// \return a ready future if we stopped successfully, or a failed future if
///         a call to to \c action failed.
template<typename AsyncAction>
GCC6_CONCEPT( requires seastar::ApplyReturns<AsyncAction, stop_iteration> || seastar::ApplyReturns<AsyncAction, future<stop_iteration>> )
inline
future<> repeat(AsyncAction action) noexcept {
    using futurator = futurize<std::result_of_t<AsyncAction()>>;
    static_assert(std::is_same<future<stop_iteration>, typename futurator::type>::value, "bad AsyncAction signature");
    try {
        do {
            // Do not type-erase here in case this is a short repeat()
            auto f = futurator::apply(action);

            if (!f.available()) {
              return [&] () noexcept {
                memory::disable_failure_guard dfg;
                auto repeater = new internal::repeater<AsyncAction>(std::move(action));
                auto ret = repeater->get_future();
                internal::set_callback(f, repeater);
                return ret;
              }();
            }

            if (f.get0() == stop_iteration::yes) {
                return make_ready_future<>();
            }
        } while (!need_preempt());

        auto repeater = new internal::repeater<AsyncAction>(stop_iteration::no, std::move(action));
        auto ret = repeater->get_future();
        schedule(repeater);
        return ret;
    } catch (...) {
        return make_exception_future(std::current_exception());
    }
}

/// \cond internal

template <typename T>
struct repeat_until_value_type_helper;

/// \endcond

/// Type helper for repeat_until_value()
template <typename T>
struct repeat_until_value_type_helper<future<compat::optional<T>>> {
    /// The type of the value we are computing
    using value_type = T;
    /// Type used by \c AsyncAction while looping
    using optional_type = compat::optional<T>;
    /// Return type of repeat_until_value()
    using future_type = future<value_type>;
};

/// Return value of repeat_until_value()
template <typename AsyncAction>
using repeat_until_value_return_type
        = typename repeat_until_value_type_helper<typename futurize<std::result_of_t<AsyncAction()>>::type>::future_type;

namespace internal {

template <typename AsyncAction, typename T>
class repeat_until_value_state final : public continuation_base<compat::optional<T>> {
    using futurator = futurize<std::result_of_t<AsyncAction()>>;
    promise<T> _promise;
    AsyncAction _action;
public:
    explicit repeat_until_value_state(AsyncAction action) : _action(std::move(action)) {}
    repeat_until_value_state(compat::optional<T> st, AsyncAction action) : repeat_until_value_state(std::move(action)) {
        this->_state.set(std::make_tuple(std::move(st)));
    }
    future<T> get_future() { return _promise.get_future(); }
    virtual void run_and_dispose() noexcept override {
        if (this->_state.failed()) {
            _promise.set_exception(std::move(this->_state).get_exception());
            delete this;
            return;
        } else {
            auto v = std::get<0>(std::move(this->_state).get());
            if (v) {
                _promise.set_value(std::move(*v));
                delete this;
                return;
            }
            this->_state = {};
        }
        try {
            do {
                auto f = futurator::apply(_action);
                if (!f.available()) {
                    internal::set_callback(f, this);
                    return;
                }
                auto ret = f.get0();
                if (ret) {
                    _promise.set_value(std::make_tuple(std::move(*ret)));
                    delete this;
                    return;
                }
            } while (!need_preempt());
        } catch (...) {
            _promise.set_exception(std::current_exception());
            delete this;
            return;
        }
        this->_state.set(compat::nullopt);
        schedule(this);
    }
};

}

/// Invokes given action until it fails or the function requests iteration to stop by returning
/// an engaged \c future<compat::optional<T>> or compat::optional<T>.  The value is extracted
/// from the \c optional, and returned, as a future, from repeat_until_value().
///
/// \param action a callable taking no arguments, returning a future<compat::optional<T>>
///               or compat::optional<T>.  Will be called again as soon as the future
///               resolves, unless the future fails, action throws, or it resolves with
///               an engaged \c optional.  If \c action is an r-value it can be moved
///               in the middle of iteration.
/// \return a ready future if we stopped successfully, or a failed future if
///         a call to to \c action failed.  The \c optional's value is returned.
template<typename AsyncAction>
GCC6_CONCEPT( requires requires (AsyncAction aa) {
    bool(futurize<std::result_of_t<AsyncAction()>>::apply(aa).get0());
    futurize<std::result_of_t<AsyncAction()>>::apply(aa).get0().value();
} )
repeat_until_value_return_type<AsyncAction>
repeat_until_value(AsyncAction action) noexcept {
    using futurator = futurize<std::result_of_t<AsyncAction()>>;
    using type_helper = repeat_until_value_type_helper<typename futurator::type>;
    // the "T" in the documentation
    using value_type = typename type_helper::value_type;
    using optional_type = typename type_helper::optional_type;
    do {
        auto f = futurator::apply(action);

        if (!f.available()) {
          return [&] () noexcept {
            memory::disable_failure_guard dfg;
            auto state = new internal::repeat_until_value_state<AsyncAction, value_type>(std::move(action));
            auto ret = state->get_future();
            internal::set_callback(f, state);
            return ret;
          }();
        }

        if (f.failed()) {
            return make_exception_future<value_type>(f.get_exception());
        }

        optional_type&& optional = std::move(f).get0();
        if (optional) {
            return make_ready_future<value_type>(std::move(optional.value()));
        }
    } while (!need_preempt());

    try {
        auto state = new internal::repeat_until_value_state<AsyncAction, value_type>(compat::nullopt, std::move(action));
        auto f = state->get_future();
        schedule(state);
        return f;
    } catch (...) {
        return make_exception_future<value_type>(std::current_exception());
    }
}

namespace internal {

template <typename StopCondition, typename AsyncAction>
class do_until_state final : public continuation_base<> {
    promise<> _promise;
    StopCondition _stop;
    AsyncAction _action;
public:
    explicit do_until_state(StopCondition stop, AsyncAction action) : _stop(std::move(stop)), _action(std::move(action)) {}
    future<> get_future() { return _promise.get_future(); }
    virtual void run_and_dispose() noexcept override {
        if (_state.available()) {
            if (_state.failed()) {
                _promise.set_urgent_state(std::move(_state));
                delete this;
                return;
            }
            _state = {}; // allow next cycle to overrun state
        }
        try {
            do {
                if (_stop()) {
                    _promise.set_value();
                    delete this;
                    return;
                }
                auto f = _action();
                if (!f.available()) {
                    internal::set_callback(f, this);
                    return;
                }
                if (f.failed()) {
                    f.forward_to(std::move(_promise));
                    delete this;
                    return;
                }
            } while (!need_preempt());
        } catch (...) {
            _promise.set_exception(std::current_exception());
            delete this;
            return;
        }
        schedule(this);
    }
};

}

/// Invokes given action until it fails or given condition evaluates to true.
///
/// \param stop_cond a callable taking no arguments, returning a boolean that
///                  evalutes to true when you don't want to call \c action
///                  any longer
/// \param action a callable taking no arguments, returning a future<>.  Will
///               be called again as soon as the future resolves, unless the
///               future fails, or \c stop_cond returns \c true.
/// \return a ready future if we stopped successfully, or a failed future if
///         a call to to \c action failed.
template<typename AsyncAction, typename StopCondition>
GCC6_CONCEPT( requires seastar::ApplyReturns<StopCondition, bool> && seastar::ApplyReturns<AsyncAction, future<>> )
inline
future<> do_until(StopCondition stop_cond, AsyncAction action) noexcept {
    using namespace internal;
    using futurator = futurize<void>;
    do {
        if (stop_cond()) {
            return make_ready_future<>();
        }
        auto f = futurator::apply(action);
        if (!f.available()) {
          return [&] () noexcept {
            memory::disable_failure_guard dfg;
            auto task = new do_until_state<StopCondition, AsyncAction>(std::move(stop_cond), std::move(action));
            auto ret = task->get_future();
            internal::set_callback(f, task);
            return ret;
          }();
        }
        if (f.failed()) {
            return f;
        }
    } while (!need_preempt());

    auto task = new do_until_state<StopCondition, AsyncAction>(std::move(stop_cond), std::move(action));
    auto f = task->get_future();
    schedule(task);
    return f;
}

/// Invoke given action until it fails.
///
/// Calls \c action repeatedly until it returns a failed future.
///
/// \param action a callable taking no arguments, returning a \c future<>
///        that becomes ready when you wish it to be called again.
/// \return a future<> that will resolve to the first failure of \c action
template<typename AsyncAction>
GCC6_CONCEPT( requires seastar::ApplyReturns<AsyncAction, future<>> )
inline
future<> keep_doing(AsyncAction action) {
    return repeat([action = std::move(action)] () mutable {
        return action().then([] {
            return stop_iteration::no;
        });
    });
}

namespace internal {
template <typename Iterator, typename AsyncAction>
class do_for_each_state final : public continuation_base<> {
    Iterator _begin;
    Iterator _end;
    AsyncAction _action;
    promise<> _pr;

public:
    do_for_each_state(Iterator begin, Iterator end, AsyncAction action, future<> first_unavailable)
        : _begin(std::move(begin)), _end(std::move(end)), _action(std::move(action)) {
        internal::set_callback(first_unavailable, this);
    }
    virtual void run_and_dispose() noexcept override {
        std::unique_ptr<do_for_each_state> zis(this);
        if (_state.failed()) {
            _pr.set_urgent_state(std::move(_state));
            return;
        }
        while (_begin != _end) {
            auto f = futurize<void>::apply(_action, *_begin++);
            if (f.failed()) {
                f.forward_to(std::move(_pr));
                return;
            }
            if (!f.available() || need_preempt()) {
                _state = {};
                internal::set_callback(f, this);
                zis.release();
                return;
            }
        }
        _pr.set_value();
    }
    future<> get_future() {
        return _pr.get_future();
    }
};
}

/// Call a function for each item in a range, sequentially (iterator version).
///
/// For each item in a range, call a function, waiting for the previous
/// invocation to complete before calling the next one.
///
/// \param begin an \c InputIterator designating the beginning of the range
/// \param end an \c InputIterator designating the endof the range
/// \param action a callable, taking a reference to objects from the range
///               as a parameter, and returning a \c future<> that resolves
///               when it is acceptable to process the next item.
/// \return a ready future on success, or the first failed future if
///         \c action failed.
template<typename Iterator, typename AsyncAction>
GCC6_CONCEPT( requires requires (Iterator i, AsyncAction aa) {
    { futurize_apply(aa, *i) } -> future<>;
} )
inline
future<> do_for_each(Iterator begin, Iterator end, AsyncAction action) {
    while (begin != end) {
        auto f = futurize<void>::apply(action, *begin++);
        if (f.failed()) {
            return f;
        }
        if (!f.available() || need_preempt()) {
            auto* s = new internal::do_for_each_state<Iterator, AsyncAction>{
                std::move(begin), std::move(end), std::move(action), std::move(f)};
            return s->get_future();
        }
    }
    return make_ready_future<>();
}

/// Call a function for each item in a range, sequentially (range version).
///
/// For each item in a range, call a function, waiting for the previous
/// invocation to complete before calling the next one.
///
/// \param range an \c Range object designating input values
/// \param action a callable, taking a reference to objects from the range
///               as a parameter, and returning a \c future<> that resolves
///               when it is acceptable to process the next item.
/// \return a ready future on success, or the first failed future if
///         \c action failed.
template<typename Container, typename AsyncAction>
GCC6_CONCEPT( requires requires (Container c, AsyncAction aa) {
    { futurize_apply(aa, *c.begin()) } -> future<>;
} )
inline
future<> do_for_each(Container& c, AsyncAction action) {
    return do_for_each(std::begin(c), std::end(c), std::move(action));
}

/// \cond internal
namespace internal {

template<typename... Futures>
struct identity_futures_tuple {
    using future_type = future<std::tuple<Futures...>>;
    using promise_type = typename future_type::promise_type;

    static void set_promise(promise_type& p, std::tuple<Futures...> futures) {
        p.set_value(std::move(futures));
    }

    static future_type make_ready_future(std::tuple<Futures...> futures) {
        return futurize<future_type>::from_tuple(std::move(futures));
    }
};

// Given a future type, find the continuation_base corresponding to that future
template <typename Future>
struct continuation_base_for_future;

template <typename... T>
struct continuation_base_for_future<future<T...>> {
    using type = continuation_base<T...>;
};

template <typename Future>
using continuation_base_for_future_t = typename continuation_base_for_future<Future>::type;

class when_all_state_base;

// If the future is ready, return true
// if the future is not ready, chain a continuation to it, and return false
using when_all_process_element_func = bool (*)(void* future, void* continuation, when_all_state_base* wasb);

struct when_all_process_element {
    when_all_process_element_func func;
    void* future;
};

class when_all_state_base {
    size_t _nr_remain;
    const when_all_process_element* _processors;
    void* _continuation;
public:
    virtual ~when_all_state_base() {}
    when_all_state_base(size_t nr_remain, const when_all_process_element* processors, void* continuation)
            : _nr_remain(nr_remain), _processors(processors), _continuation(continuation) {
    }
    void complete_one() {
        // We complete in reverse order; if the futures happen to complete
        // in order, then waiting for the last one will find the rest ready
        --_nr_remain;
        while (_nr_remain) {
            bool ready = process_one(_nr_remain - 1);
            if (!ready) {
                return;
            }
            --_nr_remain;
        }
        if (!_nr_remain) {
            delete this;
        }
    }
    void do_wait_all() {
        ++_nr_remain; // fake pending completion for complete_one()
        complete_one();
    }
    bool process_one(size_t idx) {
        auto p = _processors[idx];
        return p.func(p.future, _continuation, this);
    }
};

template <typename Future>
class when_all_state_component final : public continuation_base_for_future_t<Future> {
    when_all_state_base* _base;
    Future* _final_resting_place;
public:
    static bool process_element_func(void* future, void* continuation, when_all_state_base* wasb) {
        auto f = reinterpret_cast<Future*>(future);
        if (f->available()) {
            return true;
        } else {
            auto c = new (continuation) when_all_state_component(wasb, f);
            set_callback(*f, c);
            return false;
        }
    }
    when_all_state_component(when_all_state_base *base, Future* future) : _base(base), _final_resting_place(future) {}
    virtual void run_and_dispose() noexcept override {
        using futurator = futurize<Future>;
        if (__builtin_expect(this->_state.failed(), false)) {
            *_final_resting_place = futurator::make_exception_future(std::move(this->_state).get_exception());
        } else {
            *_final_resting_place = futurator::from_tuple(std::move(this->_state).get_value());
        }
        auto base = _base;
        this->~when_all_state_component();
        base->complete_one();
    }
};

#if __cpp_fold_expressions >= 201603
// This optimization requires C++17
#  define SEASTAR__WAIT_ALL__AVOID_ALLOCATION_WHEN_ALL_READY
#endif

template<typename ResolvedTupleTransform, typename... Futures>
class when_all_state : public when_all_state_base {
    static constexpr size_t nr = sizeof...(Futures);
    using type = std::tuple<Futures...>;
    type tuple;
    // We only schedule one continuation at a time, and store it in _cont.
    // This way, if while the future we wait for completes, some other futures
    // also complete, we won't need to schedule continuations for them.
    std::aligned_union_t<1, when_all_state_component<Futures>...> _cont;
    when_all_process_element _processors[nr];
public:
    typename ResolvedTupleTransform::promise_type p;
    when_all_state(Futures&&... t) : when_all_state_base(nr, _processors, &_cont), tuple(std::make_tuple(std::move(t)...)) {
        init_element_processors(std::make_index_sequence<nr>());
    }
    virtual ~when_all_state() {
        ResolvedTupleTransform::set_promise(p, std::move(tuple));
    }
private:
    template <size_t... Idx>
    void init_element_processors(std::index_sequence<Idx...>) {
        auto ignore = {
            0,
            (_processors[Idx] = when_all_process_element{
                when_all_state_component<std::tuple_element_t<Idx, type>>::process_element_func,
                &std::get<Idx>(tuple)
             }, 0)...
        };
        (void)ignore;
    }
public:
    static typename ResolvedTupleTransform::future_type wait_all(Futures&&... futures) {
#ifdef SEASTAR__WAIT_ALL__AVOID_ALLOCATION_WHEN_ALL_READY
        if ((futures.available() && ...)) {
            return ResolvedTupleTransform::make_ready_future(std::make_tuple(std::move(futures)...));
        }
#endif
        auto state = [&] () noexcept {
            memory::disable_failure_guard dfg;
            return new when_all_state(std::move(futures)...);
        }();
        auto ret = state->p.get_future();
        state->do_wait_all();
        return ret;
    }
};

}
/// \endcond

GCC6_CONCEPT(

/// \cond internal
namespace impl {


// Want: folds

template <typename T>
struct is_tuple_of_futures : std::false_type {
};

template <>
struct is_tuple_of_futures<std::tuple<>> : std::true_type {
};

template <typename... T, typename... Rest>
struct is_tuple_of_futures<std::tuple<future<T...>, Rest...>> : is_tuple_of_futures<std::tuple<Rest...>> {
};

}
/// \endcond

template <typename... Futs>
concept bool AllAreFutures = impl::is_tuple_of_futures<std::tuple<Futs...>>::value;

)

template<typename Fut, std::enable_if_t<is_future<Fut>::value, int> = 0>
auto futurize_apply_if_func(Fut&& fut) {
    return std::forward<Fut>(fut);
}

template<typename Func, std::enable_if_t<!is_future<Func>::value, int> = 0>
auto futurize_apply_if_func(Func&& func) {
    return futurize_apply(std::forward<Func>(func));
}

template <typename... Futs>
GCC6_CONCEPT( requires seastar::AllAreFutures<Futs...> )
inline
future<std::tuple<Futs...>>
when_all_impl(Futs&&... futs) {
    namespace si = internal;
    using state = si::when_all_state<si::identity_futures_tuple<Futs...>, Futs...>;
    return state::wait_all(std::forward<Futs>(futs)...);
}

/// Wait for many futures to complete, capturing possible errors (variadic version).
///
/// Each future can be passed directly, or a function that returns a
/// future can be given instead.
///
/// If any function throws, an exceptional future is created for it.
///
/// Returns a tuple of futures so individual values or exceptions can be
/// examined.
///
/// \param fut_or_funcs futures or functions that return futures
/// \return an \c std::tuple<> of all futures returned; when ready,
///         all contained futures will be ready as well.
template <typename... FutOrFuncs>
inline auto when_all(FutOrFuncs&&... fut_or_funcs) {
    return when_all_impl(futurize_apply_if_func(std::forward<FutOrFuncs>(fut_or_funcs))...);
}

/// \cond internal
namespace internal {

template<typename Future>
struct identity_futures_vector {
    using future_type = future<std::vector<Future>>;
    static future_type run(std::vector<Future> futures) {
        return make_ready_future<std::vector<Future>>(std::move(futures));
    }
};

// Internal function for when_all().
template <typename ResolvedVectorTransform, typename Future>
inline
typename ResolvedVectorTransform::future_type
complete_when_all(std::vector<Future>&& futures, typename std::vector<Future>::iterator pos) {
    // If any futures are already ready, skip them.
    while (pos != futures.end() && pos->available()) {
        ++pos;
    }
    // Done?
    if (pos == futures.end()) {
        return ResolvedVectorTransform::run(std::move(futures));
    }
    // Wait for unready future, store, and continue.
    return pos->then_wrapped([futures = std::move(futures), pos] (auto fut) mutable {
        *pos++ = std::move(fut);
        return complete_when_all<ResolvedVectorTransform>(std::move(futures), pos);
    });
}

template<typename ResolvedVectorTransform, typename FutureIterator>
inline auto
do_when_all(FutureIterator begin, FutureIterator end) {
    using itraits = std::iterator_traits<FutureIterator>;
    std::vector<typename itraits::value_type> ret;
    ret.reserve(iterator_range_estimate_vector_capacity(begin, end, typename itraits::iterator_category()));
    // Important to invoke the *begin here, in case it's a function iterator,
    // so we launch all computation in parallel.
    std::move(begin, end, std::back_inserter(ret));
    return complete_when_all<ResolvedVectorTransform>(std::move(ret), ret.begin());
}

}
/// \endcond

/// Wait for many futures to complete, capturing possible errors (iterator version).
///
/// Given a range of futures as input, wait for all of them
/// to resolve (either successfully or with an exception), and return
/// them as a \c std::vector so individual values or exceptions can be examined.
///
/// \param begin an \c InputIterator designating the beginning of the range of futures
/// \param end an \c InputIterator designating the end of the range of futures
/// \return an \c std::vector<> of all the futures in the input; when
///         ready, all contained futures will be ready as well.
template <typename FutureIterator>
GCC6_CONCEPT( requires requires (FutureIterator i) { { *i++ }; requires is_future<std::remove_reference_t<decltype(*i)>>::value; } )
inline
future<std::vector<typename std::iterator_traits<FutureIterator>::value_type>>
when_all(FutureIterator begin, FutureIterator end) {
    namespace si = internal;
    using itraits = std::iterator_traits<FutureIterator>;
    using result_transform = si::identity_futures_vector<typename itraits::value_type>;
    return si::do_when_all<result_transform>(std::move(begin), std::move(end));
}

template <typename T, bool IsFuture>
struct reducer_with_get_traits;

template <typename T>
struct reducer_with_get_traits<T, false> {
    using result_type = decltype(std::declval<T>().get());
    using future_type = future<result_type>;
    static future_type maybe_call_get(future<> f, lw_shared_ptr<T> r) {
        return f.then([r = std::move(r)] () mutable {
            return make_ready_future<result_type>(std::move(*r).get());
        });
    }
};

template <typename T>
struct reducer_with_get_traits<T, true> {
    using future_type = decltype(std::declval<T>().get());
    static future_type maybe_call_get(future<> f, lw_shared_ptr<T> r) {
        return f.then([r = std::move(r)] () mutable {
            return r->get();
        }).then_wrapped([r] (future_type f) {
            return f;
        });
    }
};

template <typename T, typename V = void>
struct reducer_traits {
    using future_type = future<>;
    static future_type maybe_call_get(future<> f, lw_shared_ptr<T> r) {
        return f.then([r = std::move(r)] {});
    }
};

template <typename T>
struct reducer_traits<T, decltype(std::declval<T>().get(), void())> : public reducer_with_get_traits<T, is_future<std::result_of_t<decltype(&T::get)(T)>>::value> {};

// @Mapper is a callable which transforms values from the iterator range
// into a future<T>. @Reducer is an object which can be called with T as
// parameter and yields a future<>. It may have a get() method which returns
// a value of type U which holds the result of reduction. This value is wrapped
// in a future and returned by this function. If the reducer has no get() method
// then this function returns future<>.
//
// TODO: specialize for non-deferring reducer
template <typename Iterator, typename Mapper, typename Reducer>
inline
auto
map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Reducer&& r)
    -> typename reducer_traits<Reducer>::future_type
{
    auto r_ptr = make_lw_shared(std::forward<Reducer>(r));
    future<> ret = make_ready_future<>();
    using futurator = futurize<decltype(mapper(*begin))>;
    while (begin != end) {
        ret = futurator::apply(mapper, *begin++).then_wrapped([ret = std::move(ret), r_ptr] (auto f) mutable {
            return ret.then_wrapped([f = std::move(f), r_ptr] (auto rf) mutable {
                if (rf.failed()) {
                    f.ignore_ready_future();
                    return std::move(rf);
                } else {
                    return futurize<void>::apply(*r_ptr, std::move(f.get()));
                }
            });
        });
    }
    return reducer_traits<Reducer>::maybe_call_get(std::move(ret), r_ptr);
}

/// Asynchronous map/reduce transformation.
///
/// Given a range of objects, an asynchronous unary function
/// operating on these objects, an initial value, and a
/// binary function for reducing, map_reduce() will
/// transform each object in the range, then apply
/// the the reducing function to the result.
///
/// Example:
///
/// Calculate the total size of several files:
///
/// \code
///  map_reduce(files.begin(), files.end(),
///             std::mem_fn(file::size),
///             size_t(0),
///             std::plus<size_t>())
/// \endcode
///
/// Requirements:
///    - Iterator: an InputIterator.
///    - Mapper: unary function taking Iterator::value_type and producing a future<...>.
///    - Initial: any value type
///    - Reduce: a binary function taking two Initial values and returning an Initial
///
/// Return type:
///    - future<Initial>
///
/// \param begin beginning of object range to operate on
/// \param end end of object range to operate on
/// \param mapper map function to call on each object, returning a future
/// \param initial initial input value to reduce function
/// \param reduce binary function for merging two result values from \c mapper
///
/// \return equivalent to \c reduce(reduce(initial, mapper(obj0)), mapper(obj1)) ...
template <typename Iterator, typename Mapper, typename Initial, typename Reduce>
GCC6_CONCEPT( requires requires (Iterator i, Mapper mapper, Initial initial, Reduce reduce) {
     *i++;
     { i != i} -> bool;
     mapper(*i);
     requires is_future<decltype(mapper(*i))>::value;
     { reduce(std::move(initial), mapper(*i).get0()) } -> Initial;
} )
inline
future<Initial>
map_reduce(Iterator begin, Iterator end, Mapper&& mapper, Initial initial, Reduce reduce) {
    struct state {
        Initial result;
        Reduce reduce;
    };
    auto s = make_lw_shared(state{std::move(initial), std::move(reduce)});
    future<> ret = make_ready_future<>();
    using futurator = futurize<decltype(mapper(*begin))>;
    while (begin != end) {
        ret = futurator::apply(mapper, *begin++).then_wrapped([s = s.get(), ret = std::move(ret)] (auto f) mutable {
            try {
                s->result = s->reduce(std::move(s->result), std::move(f.get0()));
                return std::move(ret);
            } catch (...) {
                return std::move(ret).then_wrapped([ex = std::current_exception()] (auto f) {
                    f.ignore_ready_future();
                    return make_exception_future<>(ex);
                });
            }
        });
    }
    return ret.then([s] {
        return make_ready_future<Initial>(std::move(s->result));
    });
}

/// Asynchronous map/reduce transformation (range version).
///
/// Given a range of objects, an asynchronous unary function
/// operating on these objects, an initial value, and a
/// binary function for reducing, map_reduce() will
/// transform each object in the range, then apply
/// the the reducing function to the result.
///
/// Example:
///
/// Calculate the total size of several files:
///
/// \code
///  std::vector<file> files = ...;
///  map_reduce(files,
///             std::mem_fn(file::size),
///             size_t(0),
///             std::plus<size_t>())
/// \endcode
///
/// Requirements:
///    - Iterator: an InputIterator.
///    - Mapper: unary function taking Iterator::value_type and producing a future<...>.
///    - Initial: any value type
///    - Reduce: a binary function taking two Initial values and returning an Initial
///
/// Return type:
///    - future<Initial>
///
/// \param range object range to operate on
/// \param mapper map function to call on each object, returning a future
/// \param initial initial input value to reduce function
/// \param reduce binary function for merging two result values from \c mapper
///
/// \return equivalent to \c reduce(reduce(initial, mapper(obj0)), mapper(obj1)) ...
template <typename Range, typename Mapper, typename Initial, typename Reduce>
GCC6_CONCEPT( requires requires (Range range, Mapper mapper, Initial initial, Reduce reduce) {
     std::begin(range);
     std::end(range);
     mapper(*std::begin(range));
     requires is_future<std::remove_reference_t<decltype(mapper(*std::begin(range)))>>::value;
     { reduce(std::move(initial), mapper(*std::begin(range)).get0()) } -> Initial;
} )
inline
future<Initial>
map_reduce(Range&& range, Mapper&& mapper, Initial initial, Reduce reduce) {
    return map_reduce(std::begin(range), std::end(range), std::forward<Mapper>(mapper),
            std::move(initial), std::move(reduce));
}

// Implements @Reducer concept. Calculates the result by
// adding elements to the accumulator.
template <typename Result, typename Addend = Result>
class adder {
private:
    Result _result;
public:
    future<> operator()(const Addend& value) {
        _result += value;
        return make_ready_future<>();
    }
    Result get() && {
        return std::move(_result);
    }
};

inline
future<> now() {
    return make_ready_future<>();
}

// Returns a future which is not ready but is scheduled to resolve soon.
future<> later();

class timed_out_error : public std::exception {
public:
    virtual const char* what() const noexcept {
        return "timedout";
    }
};

struct default_timeout_exception_factory {
    static auto timeout() {
        return timed_out_error();
    }
};

/// \brief Wait for either a future, or a timeout, whichever comes first
///
/// When timeout is reached the returned future resolves with an exception
/// produced by ExceptionFactory::timeout(). By default it is \ref timed_out_error exception.
///
/// Note that timing out doesn't cancel any tasks associated with the original future.
/// It also doesn't cancel the callback registerred on it.
///
/// \param f future to wait for
/// \param timeout time point after which the returned future should be failed
///
/// \return a future which will be either resolved with f or a timeout exception
template<typename ExceptionFactory = default_timeout_exception_factory, typename Clock, typename Duration, typename... T>
future<T...> with_timeout(std::chrono::time_point<Clock, Duration> timeout, future<T...> f) {
    if (f.available()) {
        return f;
    }
    auto pr = std::make_unique<promise<T...>>();
    auto result = pr->get_future();
    timer<Clock> timer([&pr = *pr] {
        pr.set_exception(std::make_exception_ptr(ExceptionFactory::timeout()));
    });
    timer.arm(timeout);
    // Future is returned indirectly.
    (void)f.then_wrapped([pr = std::move(pr), timer = std::move(timer)] (auto&& f) mutable {
        if (timer.cancel()) {
            f.forward_to(std::move(*pr));
        } else {
            f.ignore_ready_future();
        }
    });
    return result;
}

namespace internal {

template<typename Future>
struct future_has_value {
    enum {
        value = !std::is_same<std::decay_t<Future>, future<>>::value
    };
};

template<typename Tuple>
struct tuple_to_future;

template<typename... Elements>
struct tuple_to_future<std::tuple<Elements...>> {
    using type = future<Elements...>;
    using promise_type = promise<Elements...>;

    static auto make_ready(std::tuple<Elements...> t) {
        auto create_future = [] (auto&&... args) {
            return make_ready_future<Elements...>(std::move(args)...);
        };
        return apply(create_future, std::move(t));
    }

    static auto make_failed(std::exception_ptr excp) {
        return seastar::make_exception_future<Elements...>(std::move(excp));
    }
};

template<typename... Futures>
class extract_values_from_futures_tuple {
    static auto transform(std::tuple<Futures...> futures) {
        auto prepare_result = [] (auto futures) {
            auto fs = tuple_filter_by_type<internal::future_has_value>(std::move(futures));
            return tuple_map(std::move(fs), [] (auto&& e) {
                return internal::untuple(e.get());
            });
        };

        using tuple_futurizer = internal::tuple_to_future<decltype(prepare_result(std::move(futures)))>;

        std::exception_ptr excp;
        tuple_for_each(futures, [&excp] (auto& f) {
            if (!excp) {
                if (f.failed()) {
                    excp = f.get_exception();
                }
            } else {
                f.ignore_ready_future();
            }
        });
        if (excp) {
            return tuple_futurizer::make_failed(std::move(excp));
        }

        return tuple_futurizer::make_ready(prepare_result(std::move(futures)));
    }
public:
    using future_type = decltype(transform(std::declval<std::tuple<Futures...>>()));
    using promise_type = typename future_type::promise_type;

    static void set_promise(promise_type& p, std::tuple<Futures...> tuple) {
        transform(std::move(tuple)).forward_to(std::move(p));
    }

    static future_type make_ready_future(std::tuple<Futures...> tuple) {
        return transform(std::move(tuple));
    }
};

template<typename Future>
struct extract_values_from_futures_vector {
    using value_type = decltype(untuple(std::declval<typename Future::value_type>()));

    using future_type = future<std::vector<value_type>>;

    static future_type run(std::vector<Future> futures) {
        std::vector<value_type> values;
        values.reserve(futures.size());

        std::exception_ptr excp;
        for (auto&& f : futures) {
            if (!excp) {
                if (f.failed()) {
                    excp = f.get_exception();
                } else {
                    values.emplace_back(untuple(f.get()));
                }
            } else {
                f.ignore_ready_future();
            }
        }
        if (excp) {
            return seastar::make_exception_future<std::vector<value_type>>(std::move(excp));
        }
        return make_ready_future<std::vector<value_type>>(std::move(values));
    }
};

template<>
struct extract_values_from_futures_vector<future<>> {
    using future_type = future<>;

    static future_type run(std::vector<future<>> futures) {
        std::exception_ptr excp;
        for (auto&& f : futures) {
            if (!excp) {
                if (f.failed()) {
                    excp = f.get_exception();
                }
            } else {
                f.ignore_ready_future();
            }
        }
        if (excp) {
            return seastar::make_exception_future<>(std::move(excp));
        }
        return make_ready_future<>();
    }
};

}

template<typename... Futures>
GCC6_CONCEPT( requires seastar::AllAreFutures<Futures...> )
inline auto when_all_succeed_impl(Futures&&... futures) {
    using state = internal::when_all_state<internal::extract_values_from_futures_tuple<Futures...>, Futures...>;
    return state::wait_all(std::forward<Futures>(futures)...);
}

/// Wait for many futures to complete (variadic version).
///
/// Each future can be passed directly, or a function that returns a
/// future can be given instead.
///
/// If any function throws, or if the returned future fails, one of
/// the exceptions is returned by this function as a failed future.
///
/// \param fut_or_funcs futures or functions that return futures
/// \return future containing values of futures returned by funcs
template <typename... FutOrFuncs>
inline auto when_all_succeed(FutOrFuncs&&... fut_or_funcs) {
    return when_all_succeed_impl(futurize_apply_if_func(std::forward<FutOrFuncs>(fut_or_funcs))...);
}

/// Wait for many futures to complete (iterator version).
///
/// Given a range of futures as input, wait for all of them
/// to resolve, and return a future containing a vector of values of the
/// original futures.
/// In case any of the given futures fails one of the exceptions is returned
/// by this function as a failed future.
/// \param begin an \c InputIterator designating the beginning of the range of futures
/// \param end an \c InputIterator designating the end of the range of futures
/// \return an \c std::vector<> of all the valus in the input
template <typename FutureIterator, typename = typename std::iterator_traits<FutureIterator>::value_type>
GCC6_CONCEPT( requires requires (FutureIterator i) {
     *i++;
     { i != i } -> bool;
     requires is_future<std::remove_reference_t<decltype(*i)>>::value;
} )
inline auto
when_all_succeed(FutureIterator begin, FutureIterator end) {
    using itraits = std::iterator_traits<FutureIterator>;
    using result_transform = internal::extract_values_from_futures_vector<typename itraits::value_type>;
    return internal::do_when_all<result_transform>(std::move(begin), std::move(end));
}

}

/// @}

#include <boost/version.hpp>

#if (BOOST_VERSION < 105800)

#error "Boost version >= 1.58 is required for using variant visitation helpers."
#error "Earlier versions lack support for return value deduction and move-only return values"

#endif

namespace seastar {

/// \cond internal
namespace internal {

#if __cplusplus >= 201703L // C++17

template<typename... Args>
struct variant_visitor : Args... {
    variant_visitor(Args&&... a) : Args(std::move(a))... {}
    using Args::operator()...;
};

template<typename... Args> variant_visitor(Args&&...) -> variant_visitor<Args...>;

#else

template <typename... Args>
struct variant_visitor;

template <typename FuncObj, typename... Args>
struct variant_visitor<FuncObj, Args...> : FuncObj, variant_visitor<Args...>
{
    variant_visitor(FuncObj&& func_obj, Args&&... args)
        : FuncObj(std::move(func_obj))
        , variant_visitor<Args...>(std::move(args)...) {}

    using FuncObj::operator();
    using variant_visitor<Args...>::operator();
};

template <typename FuncObj>
struct variant_visitor<FuncObj> : FuncObj
{
    variant_visitor(FuncObj&& func_obj) : FuncObj(std::forward<FuncObj>(func_obj)) {}

    using FuncObj::operator();
};

#endif

}
/// \endcond

/// \addtogroup utilities
/// @{

/// Creates a visitor from function objects.
///
/// Returns a visitor object comprised of the provided function objects. Can be
/// used with std::variant, boost::variant or any other custom variant
/// implementation.
///
/// \param args function objects each accepting one or some types stored in the variant as input
template <typename... Args>
auto make_visitor(Args&&... args)
{
    return internal::variant_visitor<Args...>(std::forward<Args>(args)...);
}

/// Applies a static visitor comprised of supplied lambdas to a variant.
/// Note that the lambdas should cover all the types that the variant can possibly hold.
///
/// Returns the common type of return types of all lambdas.
///
/// \tparam Variant the type of a variant
/// \tparam Args types of lambda objects
/// \param variant the variant object
/// \param args lambda objects each accepting one or some types stored in the variant as input
/// \return
template <typename Variant, typename... Args>
inline auto visit(Variant&& variant, Args&&... args)
{
    static_assert(sizeof...(Args) > 0, "At least one lambda must be provided for visitation");
#ifdef SEASTAR_USE_STD_OPTIONAL_VARIANT_STRINGVIEW
    return std::visit(
#else
    return boost::apply_visitor(
#endif
        make_visitor(std::forward<Args>(args)...),
        variant);
}

#ifdef SEASTAR_USE_STD_OPTIONAL_VARIANT_STRINGVIEW

namespace internal {
template<typename... Args>
struct castable_variant {
    compat::variant<Args...> var;

    template<typename... SuperArgs>
    operator compat::variant<SuperArgs...>() && {
        return std::visit([] (auto&& x) {
            return std::variant<SuperArgs...>(std::move(x));
        }, var);
    }
};
}

template<typename... Args>
internal::castable_variant<Args...> variant_cast(compat::variant<Args...>&& var) {
    return {std::move(var)};
}

template<typename... Args>
internal::castable_variant<Args...> variant_cast(const compat::variant<Args...>& var) {
    return {var};
}

#else

template<typename Variant>
Variant variant_cast(Variant&& var) {
    return std::forward<Variant>(var);
}

#endif

/// @}

}

namespace seastar {

inline future<temporary_buffer<char>> data_source_impl::skip(uint64_t n)
{
    return do_with(uint64_t(n), [this] (uint64_t& n) {
        return repeat_until_value([&] {
            return get().then([&] (temporary_buffer<char> buffer) -> compat::optional<temporary_buffer<char>> {
                if (buffer.size() >= n) {
                    buffer.trim_front(n);
                    return SEASTAR_COPY_ELISION(buffer);
                }
                n -= buffer.size();
                return { };
            });
        });
    });
}

template<typename CharType>
inline
future<> output_stream<CharType>::write(const char_type* buf) {
    return write(buf, strlen(buf));
}

template<typename CharType>
template<typename StringChar, typename SizeType, SizeType MaxSize, bool NulTerminate>
inline
future<> output_stream<CharType>::write(const basic_sstring<StringChar, SizeType, MaxSize, NulTerminate>& s) {
    return write(reinterpret_cast<const CharType *>(s.c_str()), s.size());
}

template<typename CharType>
inline
future<> output_stream<CharType>::write(const std::basic_string<CharType>& s) {
    return write(s.c_str(), s.size());
}

template<typename CharType>
future<> output_stream<CharType>::write(scattered_message<CharType> msg) {
    return write(std::move(msg).release());
}

template<typename CharType>
future<>
output_stream<CharType>::zero_copy_put(net::packet p) {
    // if flush is scheduled, disable it, so it will not try to write in parallel
    _flush = false;
    if (_flushing) {
        // flush in progress, wait for it to end before continuing
        return _in_batch.value().get_future().then([this, p = std::move(p)] () mutable {
            return _fd.put(std::move(p));
        });
    } else {
        return _fd.put(std::move(p));
    }
}

// Writes @p in chunks of _size length. The last chunk is buffered if smaller.
template <typename CharType>
future<>
output_stream<CharType>::zero_copy_split_and_put(net::packet p) {
    return repeat([this, p = std::move(p)] () mutable {
        if (p.len() < _size) {
            if (p.len()) {
                _zc_bufs = std::move(p);
            } else {
                _zc_bufs = net::packet::make_null_packet();
            }
            return make_ready_future<stop_iteration>(stop_iteration::yes);
        }
        auto chunk = p.share(0, _size);
        p.trim_front(_size);
        return zero_copy_put(std::move(chunk)).then([] {
            return stop_iteration::no;
        });
    });
}

template<typename CharType>
future<> output_stream<CharType>::write(net::packet p) {
    static_assert(std::is_same<CharType, char>::value, "packet works on char");

    if (p.len() != 0) {
        assert(!_end && "Mixing buffered writes and zero-copy writes not supported yet");

        if (_zc_bufs) {
            _zc_bufs.append(std::move(p));
        } else {
            _zc_bufs = std::move(p);
        }

        if (_zc_bufs.len() >= _size) {
            if (_trim_to_size) {
                return zero_copy_split_and_put(std::move(_zc_bufs));
            } else {
                return zero_copy_put(std::move(_zc_bufs));
            }
        }
    }
    return make_ready_future<>();
}

template<typename CharType>
future<> output_stream<CharType>::write(temporary_buffer<CharType> p) {
    if (p.empty()) {
        return make_ready_future<>();
    }
    assert(!_end && "Mixing buffered writes and zero-copy writes not supported yet");

    return write(net::packet(std::move(p)));
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read_exactly_part(size_t n, tmp_buf out, size_t completed) {
    if (available()) {
        auto now = std::min(n - completed, available());
        std::copy(_buf.get(), _buf.get() + now, out.get_write() + completed);
        _buf.trim_front(now);
        completed += now;
    }
    if (completed == n) {
        return make_ready_future<tmp_buf>(std::move(out));
    }

    // _buf is now empty
    return _fd.get().then([this, n, out = std::move(out), completed] (auto buf) mutable {
        if (buf.size() == 0) {
            _eof = true;
            return make_ready_future<tmp_buf>(std::move(buf));
        }
        _buf = std::move(buf);
        return this->read_exactly_part(n, std::move(out), completed);
    });
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read_exactly(size_t n) {
    if (_buf.size() == n) {
        // easy case: steal buffer, return to caller
        return make_ready_future<tmp_buf>(std::move(_buf));
    } else if (_buf.size() > n) {
        // buffer large enough, share it with caller
        auto front = _buf.share(0, n);
        _buf.trim_front(n);
        return make_ready_future<tmp_buf>(std::move(front));
    } else if (_buf.size() == 0) {
        // buffer is empty: grab one and retry
        return _fd.get().then([this, n] (auto buf) mutable {
            if (buf.size() == 0) {
                _eof = true;
                return make_ready_future<tmp_buf>(std::move(buf));
            }
            _buf = std::move(buf);
            return this->read_exactly(n);
        });
    } else {
        // buffer too small: start copy/read loop
        tmp_buf b(n);
        return read_exactly_part(n, std::move(b), 0);
    }
}

template <typename CharType>
template <typename Consumer>
GCC6_CONCEPT(requires InputStreamConsumer<Consumer, CharType> || ObsoleteInputStreamConsumer<Consumer, CharType>)
future<>
input_stream<CharType>::consume(Consumer&& consumer) {
    return repeat([consumer = std::move(consumer), this] () mutable {
        if (_buf.empty() && !_eof) {
            return _fd.get().then([this] (tmp_buf buf) {
                _buf = std::move(buf);
                _eof = _buf.empty();
                return make_ready_future<stop_iteration>(stop_iteration::no);
            });
        }
        return consumer(std::move(_buf)).then([this] (consumption_result_type result) {
            return seastar::visit(result.get(), [this] (const continue_consuming&) {
               // If we're here, consumer consumed entire buffer and is ready for
                // more now. So we do not return, and rather continue the loop.
                //
                // If we're at eof, we should stop.
                return make_ready_future<stop_iteration>(stop_iteration(this->_eof));
            }, [this] (stop_consuming<CharType>& stop) {
                // consumer is done
                this->_buf = std::move(stop.get_buffer());
                return make_ready_future<stop_iteration>(stop_iteration::yes);
            }, [this] (const skip_bytes& skip) {
                return this->_fd.skip(skip.get_value()).then([this](tmp_buf buf) {
                    if (!buf.empty()) {
                        this->_buf = std::move(buf);
                    }
                    return make_ready_future<stop_iteration>(stop_iteration::no);
                });
            });
        });
    });
}

template <typename CharType>
template <typename Consumer>
GCC6_CONCEPT(requires InputStreamConsumer<Consumer, CharType> || ObsoleteInputStreamConsumer<Consumer, CharType>)
future<>
input_stream<CharType>::consume(Consumer& consumer) {
    return consume(std::ref(consumer));
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read_up_to(size_t n) {
    using tmp_buf = temporary_buffer<CharType>;
    if (_buf.empty()) {
        if (_eof) {
            return make_ready_future<tmp_buf>();
        } else {
            return _fd.get().then([this, n] (tmp_buf buf) {
                _eof = buf.empty();
                _buf = std::move(buf);
                return read_up_to(n);
            });
        }
    } else if (_buf.size() <= n) {
        // easy case: steal buffer, return to caller
        return make_ready_future<tmp_buf>(std::move(_buf));
    } else {
        // buffer is larger than n, so share its head with a caller
        auto front = _buf.share(0, n);
        _buf.trim_front(n);
        return make_ready_future<tmp_buf>(std::move(front));
    }
}

template <typename CharType>
future<temporary_buffer<CharType>>
input_stream<CharType>::read() {
    using tmp_buf = temporary_buffer<CharType>;
    if (_eof) {
        return make_ready_future<tmp_buf>();
    }
    if (_buf.empty()) {
        return _fd.get().then([this] (tmp_buf buf) {
            _eof = buf.empty();
            return make_ready_future<tmp_buf>(std::move(buf));
        });
    } else {
        return make_ready_future<tmp_buf>(std::move(_buf));
    }
}

template <typename CharType>
future<>
input_stream<CharType>::skip(uint64_t n) {
    auto skip_buf = std::min(n, _buf.size());
    _buf.trim_front(skip_buf);
    n -= skip_buf;
    if (!n) {
        return make_ready_future<>();
    }
    return _fd.skip(n).then([this] (temporary_buffer<CharType> buffer) {
        _buf = std::move(buffer);
    });
}

template <typename CharType>
data_source
input_stream<CharType>::detach() && {
    if (_buf) {
        throw std::logic_error("detach() called on a used input_stream");
    }

    return std::move(_fd);
}

// Writes @buf in chunks of _size length. The last chunk is buffered if smaller.
template <typename CharType>
future<>
output_stream<CharType>::split_and_put(temporary_buffer<CharType> buf) {
    assert(_end == 0);

    return repeat([this, buf = std::move(buf)] () mutable {
        if (buf.size() < _size) {
            if (!_buf) {
                _buf = _fd.allocate_buffer(_size);
            }
            std::copy(buf.get(), buf.get() + buf.size(), _buf.get_write());
            _end = buf.size();
            return make_ready_future<stop_iteration>(stop_iteration::yes);
        }
        auto chunk = buf.share(0, _size);
        buf.trim_front(_size);
        return put(std::move(chunk)).then([] {
            return stop_iteration::no;
        });
    });
}

template <typename CharType>
future<>
output_stream<CharType>::write(const char_type* buf, size_t n) {
    if (__builtin_expect(!_buf || n > _size - _end, false)) {
        return slow_write(buf, n);
    }
    std::copy_n(buf, n, _buf.get_write() + _end);
    _end += n;
    return make_ready_future<>();
}

template <typename CharType>
future<>
output_stream<CharType>::slow_write(const char_type* buf, size_t n) {
    assert(!_zc_bufs && "Mixing buffered writes and zero-copy writes not supported yet");
    auto bulk_threshold = _end ? (2 * _size - _end) : _size;
    if (n >= bulk_threshold) {
        if (_end) {
            auto now = _size - _end;
            std::copy(buf, buf + now, _buf.get_write() + _end);
            _end = _size;
            temporary_buffer<char> tmp = _fd.allocate_buffer(n - now);
            std::copy(buf + now, buf + n, tmp.get_write());
            _buf.trim(_end);
            _end = 0;
            return put(std::move(_buf)).then([this, tmp = std::move(tmp)]() mutable {
                if (_trim_to_size) {
                    return split_and_put(std::move(tmp));
                } else {
                    return put(std::move(tmp));
                }
            });
        } else {
            temporary_buffer<char> tmp = _fd.allocate_buffer(n);
            std::copy(buf, buf + n, tmp.get_write());
            if (_trim_to_size) {
                return split_and_put(std::move(tmp));
            } else {
                return put(std::move(tmp));
            }
        }
    }

    if (!_buf) {
        _buf = _fd.allocate_buffer(_size);
    }

    auto now = std::min(n, _size - _end);
    std::copy(buf, buf + now, _buf.get_write() + _end);
    _end += now;
    if (now == n) {
        return make_ready_future<>();
    } else {
        temporary_buffer<char> next = _fd.allocate_buffer(_size);
        std::copy(buf + now, buf + n, next.get_write());
        _end = n - now;
        std::swap(next, _buf);
        return put(std::move(next));
    }
}

template <typename CharType>
future<>
output_stream<CharType>::flush() {
    if (!_batch_flushes) {
        if (_end) {
            _buf.trim(_end);
            _end = 0;
            return put(std::move(_buf)).then([this] {
                return _fd.flush();
            });
        } else if (_zc_bufs) {
            return zero_copy_put(std::move(_zc_bufs)).then([this] {
                return _fd.flush();
            });
        }
    } else {
        if (_ex) {
            // flush is a good time to deliver outstanding errors
            return make_exception_future<>(std::move(_ex));
        } else {
            _flush = true;
            if (!_in_batch) {
                add_to_flush_poller(this);
                _in_batch = promise<>();
            }
        }
    }
    return make_ready_future<>();
}

void add_to_flush_poller(output_stream<char>* x);

template <typename CharType>
future<>
output_stream<CharType>::put(temporary_buffer<CharType> buf) {
    // if flush is scheduled, disable it, so it will not try to write in parallel
    _flush = false;
    if (_flushing) {
        // flush in progress, wait for it to end before continuing
        return _in_batch.value().get_future().then([this, buf = std::move(buf)] () mutable {
            return _fd.put(std::move(buf));
        });
    } else {
        return _fd.put(std::move(buf));
    }
}

template <typename CharType>
void
output_stream<CharType>::poll_flush() {
    if (!_flush) {
        // flush was canceled, do nothing
        _flushing = false;
        _in_batch.value().set_value();
        _in_batch = compat::nullopt;
        return;
    }

    auto f = make_ready_future();
    _flush = false;
    _flushing = true; // make whoever wants to write into the fd to wait for flush to complete

    if (_end) {
        // send whatever is in the buffer right now
        _buf.trim(_end);
        _end = 0;
        f = _fd.put(std::move(_buf));
    } else if(_zc_bufs) {
        f = _fd.put(std::move(_zc_bufs));
    }

    // FIXME: future is discarded
    (void)f.then([this] {
        return _fd.flush();
    }).then_wrapped([this] (future<> f) {
        try {
            f.get();
        } catch (...) {
            _ex = std::current_exception();
        }
        // if flush() was called while flushing flush once more
        poll_flush();
    });
}

template <typename CharType>
future<>
output_stream<CharType>::close() {
    return flush().finally([this] {
        if (_in_batch) {
            return _in_batch.value().get_future();
        } else {
            return make_ready_future();
        }
    }).then([this] {
        // report final exception as close error
        if (_ex) {
            std::rethrow_exception(_ex);
        }
    }).finally([this] {
        return _fd.close();
    });
}

template <typename CharType>
data_sink
output_stream<CharType>::detach() && {
    if (_buf) {
        throw std::logic_error("detach() called on a used output_stream");
    }

    return std::move(_fd);
}

namespace internal {

/// \cond internal
template <typename CharType>
struct stream_copy_consumer {
private:
    output_stream<CharType>& _os;
    using unconsumed_remainder = compat::optional<temporary_buffer<CharType>>;
public:
    stream_copy_consumer(output_stream<CharType>& os) : _os(os) {
    }
    future<unconsumed_remainder> operator()(temporary_buffer<CharType> data) {
        if (data.empty()) {
            return make_ready_future<unconsumed_remainder>(std::move(data));
        }
        return _os.write(data.get(), data.size()).then([] () {
            return make_ready_future<unconsumed_remainder>();
        });
    }
};
/// \endcond

}

extern template struct internal::stream_copy_consumer<char>;

template <typename CharType>
future<> copy(input_stream<CharType>& in, output_stream<CharType>& out) {
    return in.consume(internal::stream_copy_consumer<CharType>(out));
}

extern template future<> copy<char>(input_stream<char>&, output_stream<char>&);
}
#include <stdlib.h>
#include <memory>
#include <stdexcept>

namespace seastar {

namespace internal {
void* allocate_aligned_buffer_impl(size_t size, size_t align);
}

struct free_deleter {
    void operator()(void* p) { ::free(p); }
};

template <typename CharType>
inline
std::unique_ptr<CharType[], free_deleter> allocate_aligned_buffer(size_t size, size_t align) {
    static_assert(sizeof(CharType) == 1, "must allocate byte type");
    void* ret = internal::allocate_aligned_buffer_impl(size, align);
    return std::unique_ptr<CharType[], free_deleter>(reinterpret_cast<CharType *>(ret));
}


}

#include <cstddef>

namespace seastar {

// Platform-dependent cache line size for alignment and padding purposes.
static constexpr size_t cache_line_size =
#if defined(__x86_64__) || defined(__i386__)
    64;
#elif defined(__s390x__) || defined(__zarch__)
    256;
#elif defined(__PPC64__)
    128;
#elif defined(__aarch64__)
    128; // from Linux, may vary among different microarchitetures?
#else
#error "cache_line_size not defined for this architecture"
#endif

}
// A fixed capacity double-ended queue container that can be efficiently
// extended (and shrunk) from both ends.  Implementation is a single
// storage vector.
//
// Similar to libstdc++'s std::deque, except that it uses a single level
// store, and so is more efficient for simple stored items.

#include <type_traits>
#include <utility>


/// \file

namespace seastar {

/// A fixed-capacity container (like boost::static_vector) that can insert
/// and remove at both ends (like std::deque). Does not allocate.
///
/// Does not perform overflow checking when size exceeds capacity.
///
/// \tparam T type of objects stored in the container; must be noexcept move enabled
/// \tparam Capacity maximum number of objects that can be stored in the container; must be a power of 2
template <typename T, size_t Capacity>
class circular_buffer_fixed_capacity {
    size_t _begin = 0;
    size_t _end = 0;
    union maybe_storage {
        T data;
        maybe_storage() noexcept {}
        ~maybe_storage() {}
    };
    maybe_storage _storage[Capacity];
private:
    static size_t mask(size_t idx) { return idx % Capacity; }
    T* obj(size_t idx) { return &_storage[mask(idx)].data; }
    const T* obj(size_t idx) const { return &_storage[mask(idx)].data; }
public:
    static_assert((Capacity & (Capacity - 1)) == 0, "capacity must be a power of two");
    static_assert(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_move_assignable<T>::value,
            "circular_buffer_fixed_capacity only supports nothrow-move value types");
    using value_type = T;
    using size_type = size_t;
    using reference = T&;
    using pointer = T*;
    using const_reference = const T&;
    using const_pointer = const T*;
    using difference_type = ssize_t;
public:
    template <typename ValueType>
    class cbiterator {
        using holder = std::conditional_t<std::is_const<ValueType>::value, const maybe_storage, maybe_storage>;
        holder* _start;
        size_t _idx;
    private:
        cbiterator(holder* start, size_t idx) noexcept : _start(start), _idx(idx) {}
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = ValueType;
        using difference_type = ssize_t;
        using pointer = ValueType*;
        using reference = ValueType&;
    public:
        cbiterator();
        ValueType& operator*() const { return _start[mask(_idx)].data; }
        ValueType* operator->() const { return &operator*(); }
        // prefix
        cbiterator& operator++() {
            ++_idx;
            return *this;
        }
        // postfix
        cbiterator operator++(int) {
            auto v = *this;
            ++_idx;
            return v;
        }
        // prefix
        cbiterator& operator--() {
            --_idx;
            return *this;
        }
        // postfix
        cbiterator operator--(int) {
            auto v = *this;
            --_idx;
            return v;
        }
        cbiterator operator+(difference_type n) const {
            return cbiterator{_start, _idx + n};
        }
        friend cbiterator operator+(difference_type n, cbiterator i) {
            return i + n;
        }
        cbiterator operator-(difference_type n) const {
            return cbiterator{_start, _idx - n};
        }
        cbiterator& operator+=(difference_type n) {
            _idx += n;
            return *this;
        }
        cbiterator& operator-=(difference_type n) {
            _idx -= n;
            return *this;
        }
        bool operator==(const cbiterator& rhs) const {
            return _idx == rhs._idx;
        }
        bool operator!=(const cbiterator& rhs) const {
            return _idx != rhs._idx;
        }
        bool operator<(const cbiterator& rhs) const {
            return ssize_t(_idx - rhs._idx) < 0;
        }
        bool operator>(const cbiterator& rhs) const {
            return ssize_t(_idx - rhs._idx) > 0;
        }
        bool operator<=(const cbiterator& rhs) const {
            return ssize_t(_idx - rhs._idx) <= 0;
        }
        bool operator>=(const cbiterator& rhs) const {
            return ssize_t(_idx - rhs._idx) >= 0;
        }
        difference_type operator-(const cbiterator& rhs) const {
            return _idx - rhs._idx;
        }
        friend class circular_buffer_fixed_capacity;
    };
public:
    using iterator = cbiterator<T>;
    using const_iterator = cbiterator<const T>;
public:
    circular_buffer_fixed_capacity() = default;
    circular_buffer_fixed_capacity(circular_buffer_fixed_capacity&& x) noexcept;
    ~circular_buffer_fixed_capacity();
    circular_buffer_fixed_capacity& operator=(circular_buffer_fixed_capacity&& x) noexcept;
    void push_front(const T& data);
    void push_front(T&& data);
    template <typename... A>
    T& emplace_front(A&&... args);
    void push_back(const T& data);
    void push_back(T&& data);
    template <typename... A>
    T& emplace_back(A&&... args);
    T& front();
    T& back();
    void pop_front();
    void pop_back();
    bool empty() const;
    size_t size() const;
    size_t capacity() const;
    T& operator[](size_t idx);
    void clear();
    iterator begin() {
        return iterator(_storage, _begin);
    }
    const_iterator begin() const {
        return const_iterator(_storage, _begin);
    }
    iterator end() {
        return iterator(_storage, _end);
    }
    const_iterator end() const {
        return const_iterator(_storage, _end);
    }
    const_iterator cbegin() const {
        return const_iterator(_storage, _begin);
    }
    const_iterator cend() const {
        return const_iterator(_storage, _end);
    }
    iterator erase(iterator first, iterator last);
};

template <typename T, size_t Capacity>
inline
bool
circular_buffer_fixed_capacity<T, Capacity>::empty() const {
    return _begin == _end;
}

template <typename T, size_t Capacity>
inline
size_t
circular_buffer_fixed_capacity<T, Capacity>::size() const {
    return _end - _begin;
}

template <typename T, size_t Capacity>
inline
size_t
circular_buffer_fixed_capacity<T, Capacity>::capacity() const {
    return Capacity;
}

template <typename T, size_t Capacity>
inline
circular_buffer_fixed_capacity<T, Capacity>::circular_buffer_fixed_capacity(circular_buffer_fixed_capacity&& x) noexcept
        : _begin(std::exchange(x._begin, 0)), _end(std::exchange(x._end, 0)) {
    for (auto i = _begin; i != _end; ++i) {
        new (&_storage[i].data) T(std::move(x._storage[i].data));
    }
}

template <typename T, size_t Capacity>
inline
circular_buffer_fixed_capacity<T, Capacity>&
circular_buffer_fixed_capacity<T, Capacity>::operator=(circular_buffer_fixed_capacity&& x) noexcept {
    if (this != &x) {
        this->~circular_buffer_fixed_capacity();
        new (this) circular_buffer_fixed_capacity(std::move(x));
    }
    return *this;
}

template <typename T, size_t Capacity>
inline
circular_buffer_fixed_capacity<T, Capacity>::~circular_buffer_fixed_capacity() {
    for (auto i = _begin; i != _end; ++i) {
        _storage[i].data.~T();
    }
}

template <typename T, size_t Capacity>
inline
void
circular_buffer_fixed_capacity<T, Capacity>::push_front(const T& data) {
    new (obj(_begin - 1)) T(data);
    --_begin;
}

template <typename T, size_t Capacity>
inline
void
circular_buffer_fixed_capacity<T, Capacity>::push_front(T&& data) {
    new (obj(_begin - 1)) T(std::move(data));
    --_begin;
}

template <typename T, size_t Capacity>
template <typename... Args>
inline
T&
circular_buffer_fixed_capacity<T, Capacity>::emplace_front(Args&&... args) {
    auto p = new (obj(_begin - 1)) T(std::forward<Args>(args)...);
    --_begin;
    return *p;
}

template <typename T, size_t Capacity>
inline
void
circular_buffer_fixed_capacity<T, Capacity>::push_back(const T& data) {
    new (obj(_end)) T(data);
    ++_end;
}

template <typename T, size_t Capacity>
inline
void
circular_buffer_fixed_capacity<T, Capacity>::push_back(T&& data) {
    new (obj(_end)) T(std::move(data));
    ++_end;
}

template <typename T, size_t Capacity>
template <typename... Args>
inline
T&
circular_buffer_fixed_capacity<T, Capacity>::emplace_back(Args&&... args) {
    auto p = new (obj(_end)) T(std::forward<Args>(args)...);
    ++_end;
    return *p;
}

template <typename T, size_t Capacity>
inline
T&
circular_buffer_fixed_capacity<T, Capacity>::front() {
    return *obj(_begin);
}

template <typename T, size_t Capacity>
inline
T&
circular_buffer_fixed_capacity<T, Capacity>::back() {
    return *obj(_end - 1);
}

template <typename T, size_t Capacity>
inline
void
circular_buffer_fixed_capacity<T, Capacity>::pop_front() {
    obj(_begin)->~T();
    ++_begin;
}

template <typename T, size_t Capacity>
inline
void
circular_buffer_fixed_capacity<T, Capacity>::pop_back() {
    obj(_end - 1)->~T();
    --_end;
}

template <typename T, size_t Capacity>
inline
T&
circular_buffer_fixed_capacity<T, Capacity>::operator[](size_t idx) {
    return *obj(_begin + idx);
}

template <typename T, size_t Capacity>
inline
typename circular_buffer_fixed_capacity<T, Capacity>::iterator
circular_buffer_fixed_capacity<T, Capacity>::erase(iterator first, iterator last) {
    static_assert(std::is_nothrow_move_assignable<T>::value, "erase() assumes move assignment does not throw");
    if (first == last) {
        return last;
    }
    // Move to the left or right depending on which would result in least amount of moves.
    // This also guarantees that iterators will be stable when removing from either front or back.
    if (std::distance(begin(), first) < std::distance(last, end())) {
        auto new_start = std::move_backward(begin(), first, last);
        auto i = begin();
        while (i < new_start) {
            *i++.~T();
        }
        _begin = new_start.idx;
        return last;
    } else {
        auto new_end = std::move(last, end(), first);
        auto i = new_end;
        auto e = end();
        while (i < e) {
            *i++.~T();
        }
        _end = new_end.idx;
        return first;
    }
}

template <typename T, size_t Capacity>
inline
void
circular_buffer_fixed_capacity<T, Capacity>::clear() {
    for (auto i = _begin; i != _end; ++i) {
        obj(i)->~T();
    }
    _begin = _end = 0;
}

}


#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unordered_map>
#include <cassert>
#include <unistd.h>
#include <vector>
#include <queue>
#include <thread>
#include <system_error>
#include <chrono>
#include <ratio>
#include <stack>
#include <boost/next_prior.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/optional.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/container/static_vector.hpp>
#include <set>

namespace seastar {

/// Configuration structure for reactor
///
/// This structure provides configuration items for the reactor. It is typically
/// provided by \ref app_template, not the user.
struct reactor_config {
    std::chrono::duration<double> task_quota{0.5e-3}; ///< default time between polls
    /// \brief Handle SIGINT/SIGTERM by calling reactor::stop()
    ///
    /// When true, Seastar will set up signal handlers for SIGINT/SIGTERM that call
    /// reactor::stop(). The reactor will then execute callbacks installed by
    /// reactor::at_exit().
    ///
    /// When false, Seastar will not set up signal handlers for SIGINT/SIGTERM
    /// automatically. The default behavior (terminate the program) will be kept.
    /// You can adjust the behavior of SIGINT/SIGTERM by installing signal handlers
    /// via reactor::handle_signal().
    bool auto_handle_sigint_sigterm = true;  ///< automatically terminate on SIGINT/SIGTERM
};

}

#include <endian.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <signal.h>

namespace seastar {

namespace internal {

namespace linux_abi {

using aio_context_t = unsigned long;

enum class iocb_cmd : uint16_t {
    PREAD = 0,
    PWRITE = 1,
    FSYNC = 2,
    FDSYNC = 3,
    POLL = 5,
    NOOP = 6,
    PREADV = 7,
    PWRITEV = 8,
};

struct io_event {
    uint64_t data;
    uint64_t obj;
    int64_t res;
    int64_t res2;
};

constexpr int IOCB_FLAG_RESFD = 1;

struct iocb {
        uint64_t   aio_data;

#if __BYTE_ORDER == __LITTLE_ENDIAN
        uint32_t   aio_key;
        int32_t aio_rw_flags;
#elif __BYTE_ORDER == __BIG_ENDIAN
        int32_t aio_rw_flags;
        uint32_t   aio_key;
#else
#error bad byteorder
#endif

        iocb_cmd   aio_lio_opcode;
        int16_t   aio_reqprio;
        uint32_t   aio_fildes;

        uint64_t   aio_buf;
        uint64_t   aio_nbytes;
        int64_t   aio_offset;

        uint64_t   aio_reserved2;

        uint32_t   aio_flags;

        uint32_t   aio_resfd;
};

struct aio_sigset {
    const sigset_t *sigmask;
    size_t sigsetsize;
};

}

linux_abi::iocb make_read_iocb(int fd, uint64_t offset, void* buffer, size_t len);
linux_abi::iocb make_write_iocb(int fd, uint64_t offset, const void* buffer, size_t len);
linux_abi::iocb make_readv_iocb(int fd, uint64_t offset, const ::iovec* iov, size_t niov);
linux_abi::iocb make_writev_iocb(int fd, uint64_t offset, const ::iovec* iov, size_t niov);
linux_abi::iocb make_poll_iocb(int fd, uint32_t events);

void set_user_data(linux_abi::iocb& iocb, void* data);
void* get_user_data(const linux_abi::iocb& iocb);
void set_nowait(linux_abi::iocb& iocb, bool nowait);

void set_eventfd_notification(linux_abi::iocb& iocb, int eventfd);

linux_abi::iocb* get_iocb(const linux_abi::io_event& ioev);

int io_setup(int nr_events, linux_abi::aio_context_t* io_context);
int io_destroy(linux_abi::aio_context_t io_context);
int io_submit(linux_abi::aio_context_t io_context, long nr, linux_abi::iocb** iocbs);
int io_cancel(linux_abi::aio_context_t io_context, linux_abi::iocb* iocb, linux_abi::io_event* result);
int io_getevents(linux_abi::aio_context_t io_context, long min_nr, long nr, linux_abi::io_event* events, const ::timespec* timeout,
        bool force_syscall = false);
int io_pgetevents(linux_abi::aio_context_t io_context, long min_nr, long nr, linux_abi::io_event* events, const ::timespec* timeout, const sigset_t* sigmask,
        bool force_syscall = false);

void setup_aio_context(size_t nr, linux_abi::aio_context_t* io_context);

}

namespace internal {

inline
linux_abi::iocb
make_read_iocb(int fd, uint64_t offset, void* buffer, size_t len) {
    linux_abi::iocb iocb{};
    iocb.aio_lio_opcode = linux_abi::iocb_cmd::PREAD;
    iocb.aio_fildes = fd;
    iocb.aio_offset = offset;
    iocb.aio_buf = reinterpret_cast<uintptr_t>(buffer);
    iocb.aio_nbytes = len;
    return iocb;
}

inline
linux_abi::iocb
make_write_iocb(int fd, uint64_t offset, const void* buffer, size_t len) {
    linux_abi::iocb iocb{};
    iocb.aio_lio_opcode = linux_abi::iocb_cmd::PWRITE;
    iocb.aio_fildes = fd;
    iocb.aio_offset = offset;
    iocb.aio_buf = reinterpret_cast<uintptr_t>(buffer);
    iocb.aio_nbytes = len;
    return iocb;
}

inline
linux_abi::iocb
make_readv_iocb(int fd, uint64_t offset, const ::iovec* iov, size_t niov) {
    linux_abi::iocb iocb{};
    iocb.aio_lio_opcode = linux_abi::iocb_cmd::PREADV;
    iocb.aio_fildes = fd;
    iocb.aio_offset = offset;
    iocb.aio_buf = reinterpret_cast<uintptr_t>(iov);
    iocb.aio_nbytes = niov;
    return iocb;
}

inline
linux_abi::iocb
make_writev_iocb(int fd, uint64_t offset, const ::iovec* iov, size_t niov) {
    linux_abi::iocb iocb{};
    iocb.aio_lio_opcode = linux_abi::iocb_cmd::PWRITEV;
    iocb.aio_fildes = fd;
    iocb.aio_offset = offset;
    iocb.aio_buf = reinterpret_cast<uintptr_t>(iov);
    iocb.aio_nbytes = niov;
    return iocb;
}

inline
linux_abi::iocb
make_poll_iocb(int fd, uint32_t events) {
    linux_abi::iocb iocb{};
    iocb.aio_lio_opcode = linux_abi::iocb_cmd::POLL;
    iocb.aio_fildes = fd;
    iocb.aio_buf = events;
    return iocb;
}

inline
linux_abi::iocb
make_fdsync_iocb(int fd) {
    linux_abi::iocb iocb{};
    iocb.aio_lio_opcode = linux_abi::iocb_cmd::FDSYNC;
    iocb.aio_fildes = fd;
    return iocb;
}

inline
void
set_user_data(linux_abi::iocb& iocb, void* data) {
    iocb.aio_data = reinterpret_cast<uintptr_t>(data);
}

inline
void*
get_user_data(const linux_abi::iocb& iocb) {
    return reinterpret_cast<void*>(uintptr_t(iocb.aio_data));
}

inline
void
set_eventfd_notification(linux_abi::iocb& iocb, int eventfd) {
    iocb.aio_flags |= linux_abi::IOCB_FLAG_RESFD;
    iocb.aio_resfd = eventfd;
}

inline
linux_abi::iocb*
get_iocb(const linux_abi::io_event& ev) {
    return reinterpret_cast<linux_abi::iocb*>(uintptr_t(ev.obj));
}

inline
void
set_nowait(linux_abi::iocb& iocb, bool nowait) {
#ifdef RWF_NOWAIT
    if (nowait) {
        iocb.aio_rw_flags |= RWF_NOWAIT;
    } else {
        iocb.aio_rw_flags &= ~RWF_NOWAIT;
    }
#endif
}

}


}


namespace seastar {

/// Determines whether seastar should throw or abort when operation made by
/// seastar fails because the target file descriptor is not valid. This is
/// detected when underlying system calls return EBADF or ENOTSOCK.
/// The default behavior is to throw std::system_error.
void set_abort_on_ebadf(bool do_abort);

/// Queries the current setting for seastar's behavior on invalid file descriptor access.
/// See set_abort_on_ebadf().
bool is_abort_on_ebadf_enabled();

}
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <sys/ioctl.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <sys/mman.h>
#include <pthread.h>

#include <iosfwd>
#include <array>
#include <sys/un.h>

#include <string>

namespace seastar {

/*!
    A helper struct for creating/manipulating UNIX-domain sockets.

    A UNIX-domain socket is either named or unnamed. If named, the name is either
    a path in the filesystem namespace, or an abstract-domain identifier. Abstract-domain
    names start with a null byte, and may contain non-printable characters.

    std::string() can hold a sequence of arbitrary bytes, and has a length() attribute
    that does not rely on using strlen(). Thus it is used here to hold the address.
 */
struct unix_domain_addr {
    const std::string name;
    const int path_count;  //  either name.length() or name.length()+1. See path_length_aux() below.

    explicit unix_domain_addr(const std::string& fn) : name{fn}, path_count{path_length_aux()} {}

    explicit unix_domain_addr(const char* fn) : name{fn}, path_count{path_length_aux()} {}

    int path_length() const { return path_count; }

    //  the following holds:
    //  for abstract name: name.length() == number of meaningful bytes, including the null in name[0].
    //  for filesystem path: name.length() does not count the implicit terminating null.
    //  Here we tweak the outside-visible length of the address.
    int path_length_aux() const {
        auto pl = (int)name.length();
        if (!pl || (name[0] == '\0')) {
            // unnamed, or abstract-namespace
            return pl;
        }
        return 1 + pl;
    }

    const char* path_bytes() const { return name.c_str(); }

    bool operator==(const unix_domain_addr& a) const {
        return name == a.name;
    }
    bool operator!=(const unix_domain_addr& a) const {
        return !(*this == a);
    }
};

std::ostream& operator<<(std::ostream&, const unix_domain_addr&);

} // namespace seastar

namespace seastar {

namespace net {
class inet_address;
}

struct ipv4_addr;
struct ipv6_addr;

class socket_address {
public:
    socklen_t addr_length; ///!< actual size of the relevant 'u' member
    union {
        ::sockaddr_storage sas;
        ::sockaddr sa;
        ::sockaddr_in in;
        ::sockaddr_in6 in6;
        ::sockaddr_un un;
    } u;
    socket_address(const sockaddr_in& sa) : addr_length{sizeof(::sockaddr_in)} {
        u.in = sa;
    }
    socket_address(const sockaddr_in6& sa) : addr_length{sizeof(::sockaddr_in6)} {
        u.in6 = sa;
    }
    socket_address(uint16_t);
    socket_address(ipv4_addr);
    socket_address(const ipv6_addr&);
    socket_address(const ipv6_addr&, uint32_t scope);
    socket_address(const net::inet_address&, uint16_t p = 0);
    explicit socket_address(const unix_domain_addr&);
    /** creates an uninitialized socket_address. this can be written into, or used as
     *  "unspecified" for such addresses as bind(addr) or local address in socket::connect
     *  (i.e. system picks)
     */
    socket_address();

    ::sockaddr& as_posix_sockaddr() { return u.sa; }
    ::sockaddr_in& as_posix_sockaddr_in() { return u.in; }
    ::sockaddr_in6& as_posix_sockaddr_in6() { return u.in6; }
    const ::sockaddr& as_posix_sockaddr() const { return u.sa; }
    const ::sockaddr_in& as_posix_sockaddr_in() const { return u.in; }
    const ::sockaddr_in6& as_posix_sockaddr_in6() const { return u.in6; }

    socket_address(uint32_t, uint16_t p = 0);

    socklen_t length() const { return addr_length; };

    bool is_af_unix() const {
        return u.sa.sa_family == AF_UNIX;
    }

    bool is_unspecified() const;

    sa_family_t family() const {
        return u.sa.sa_family;
    }

    net::inet_address addr() const;
    ::in_port_t port() const;
    bool is_wildcard() const;

    bool operator==(const socket_address&) const;
    bool operator!=(const socket_address& a) const {
        return !(*this == a);
    }
};

std::ostream& operator<<(std::ostream&, const socket_address&);

enum class transport {
    TCP = IPPROTO_TCP,
    SCTP = IPPROTO_SCTP
};

struct ipv4_addr {
    uint32_t ip;
    uint16_t port;

    ipv4_addr() : ip(0), port(0) {}
    ipv4_addr(uint32_t ip, uint16_t port) : ip(ip), port(port) {}
    ipv4_addr(uint16_t port) : ip(0), port(port) {}
    ipv4_addr(const std::string &addr);
    ipv4_addr(const std::string &addr, uint16_t port);
    ipv4_addr(const net::inet_address&, uint16_t);
    ipv4_addr(const socket_address &);
    ipv4_addr(const ::in_addr&, uint16_t = 0);

    bool is_ip_unspecified() const {
        return ip == 0;
    }
    bool is_port_unspecified() const {
        return port == 0;
    }
};

struct ipv6_addr {
    using ipv6_bytes = std::array<uint8_t, 16>;

    ipv6_bytes ip;
    uint16_t port;

    ipv6_addr(const ipv6_bytes&, uint16_t port = 0);
    ipv6_addr(uint16_t port = 0);
    ipv6_addr(const std::string&);
    ipv6_addr(const std::string&, uint16_t port);
    ipv6_addr(const net::inet_address&, uint16_t = 0);
    ipv6_addr(const ::in6_addr&, uint16_t = 0);
    ipv6_addr(const ::sockaddr_in6&);
    ipv6_addr(const socket_address&);

    bool is_ip_unspecified() const;
    bool is_port_unspecified() const {
        return port == 0;
    }
};

std::ostream& operator<<(std::ostream&, const ipv4_addr&);
std::ostream& operator<<(std::ostream&, const ipv6_addr&);

inline bool operator==(const ipv4_addr &lhs, const ipv4_addr& rhs) {
    return lhs.ip == rhs.ip && lhs.port == rhs.port;
}

}

namespace std {
template<>
struct hash<seastar::socket_address> {
    size_t operator()(const seastar::socket_address&) const;
};
template<>
struct hash<seastar::ipv4_addr> {
    size_t operator()(const seastar::ipv4_addr&) const;
};
template<>
struct hash<seastar::unix_domain_addr> {
    size_t operator()(const seastar::unix_domain_addr&) const;
};
template<>
struct hash<::sockaddr_un> {
    size_t operator()(const ::sockaddr_un&) const;
};

template <>
struct hash<seastar::transport> {
    size_t operator()(seastar::transport tr) const {
        return static_cast<size_t>(tr);
    }
};

}

namespace seastar {

/// \file
/// \defgroup posix-support POSIX Support
///
/// Mostly-internal APIs to provide C++ glue for the underlying POSIX platform;
/// but can be used by the application when they don't block.
///
/// \addtogroup posix-support
/// @{

inline void throw_system_error_on(bool condition, const char* what_arg = "");

template <typename T>
inline void throw_kernel_error(T r);

struct mmap_deleter {
    size_t _size;
    void operator()(void* ptr) const;
};

using mmap_area = std::unique_ptr<char[], mmap_deleter>;

mmap_area mmap_anonymous(void* addr, size_t length, int prot, int flags);

class file_desc {
    int _fd;
public:
    file_desc() = delete;
    file_desc(const file_desc&) = delete;
    file_desc(file_desc&& x) : _fd(x._fd) { x._fd = -1; }
    ~file_desc() { if (_fd != -1) { ::close(_fd); } }
    void operator=(const file_desc&) = delete;
    file_desc& operator=(file_desc&& x) {
        if (this != &x) {
            std::swap(_fd, x._fd);
            if (x._fd != -1) {
                x.close();
            }
        }
        return *this;
    }
    void close() {
        assert(_fd != -1);
        auto r = ::close(_fd);
        throw_system_error_on(r == -1, "close");
        _fd = -1;
    }
    int get() const { return _fd; }

    static file_desc from_fd(int fd) {
        return file_desc(fd);
    }

    static file_desc open(sstring name, int flags, mode_t mode = 0) {
        int fd = ::open(name.c_str(), flags, mode);
        throw_system_error_on(fd == -1, "open");
        return file_desc(fd);
    }
    static file_desc socket(int family, int type, int protocol = 0) {
        int fd = ::socket(family, type, protocol);
        throw_system_error_on(fd == -1, "socket");
        return file_desc(fd);
    }
    static file_desc eventfd(unsigned initval, int flags) {
        int fd = ::eventfd(initval, flags);
        throw_system_error_on(fd == -1, "eventfd");
        return file_desc(fd);
    }
    static file_desc epoll_create(int flags = 0) {
        int fd = ::epoll_create1(flags);
        throw_system_error_on(fd == -1, "epoll_create1");
        return file_desc(fd);
    }
    static file_desc timerfd_create(int clockid, int flags) {
        int fd = ::timerfd_create(clockid, flags);
        throw_system_error_on(fd == -1, "timerfd_create");
        return file_desc(fd);
    }
    static file_desc temporary(sstring directory);
    file_desc dup() const {
        int fd = ::dup(get());
        throw_system_error_on(fd == -1, "dup");
        return file_desc(fd);
    }
    file_desc accept(socket_address& sa, int flags = 0) {
        auto ret = ::accept4(_fd, &sa.as_posix_sockaddr(), &sa.addr_length, flags);
        throw_system_error_on(ret == -1, "accept4");
        return file_desc(ret);
    }
    // return nullopt if no connection is availbale to be accepted
    compat::optional<file_desc> try_accept(socket_address& sa, int flags = 0) {
        auto ret = ::accept4(_fd, &sa.as_posix_sockaddr(), &sa.addr_length, flags);
        if (ret == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(ret == -1, "accept4");
        return file_desc(ret);
    }
    void shutdown(int how) {
        auto ret = ::shutdown(_fd, how);
        if (ret == -1 && errno != ENOTCONN) {
            throw_system_error_on(ret == -1, "shutdown");
        }
    }
    void truncate(size_t size) {
        auto ret = ::ftruncate(_fd, size);
        throw_system_error_on(ret, "ftruncate");
    }
    int ioctl(int request) {
        return ioctl(request, 0);
    }
    int ioctl(int request, int value) {
        int r = ::ioctl(_fd, request, value);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    int ioctl(int request, unsigned int value) {
        int r = ::ioctl(_fd, request, value);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int ioctl(int request, X& data) {
        int r = ::ioctl(_fd, request, &data);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int ioctl(int request, X&& data) {
        int r = ::ioctl(_fd, request, &data);
        throw_system_error_on(r == -1, "ioctl");
        return r;
    }
    template <class X>
    int setsockopt(int level, int optname, X&& data) {
        int r = ::setsockopt(_fd, level, optname, &data, sizeof(data));
        throw_system_error_on(r == -1, "setsockopt");
        return r;
    }
    int setsockopt(int level, int optname, const char* data) {
        int r = ::setsockopt(_fd, level, optname, data, strlen(data) + 1);
        throw_system_error_on(r == -1, "setsockopt");
        return r;
    }
    template <typename Data>
    Data getsockopt(int level, int optname) {
        Data data;
        socklen_t len = sizeof(data);
        memset(&data, 0, len);
        int r = ::getsockopt(_fd, level, optname, &data, &len);
        throw_system_error_on(r == -1, "getsockopt");
        return data;
    }
    int getsockopt(int level, int optname, char* data, socklen_t len) {
        int r = ::getsockopt(_fd, level, optname, data, &len);
        throw_system_error_on(r == -1, "getsockopt");
        return r;
    }
    size_t size() {
        struct stat buf;
        auto r = ::fstat(_fd, &buf);
        throw_system_error_on(r == -1, "fstat");
        return buf.st_size;
    }
    boost::optional<size_t> read(void* buffer, size_t len) {
        auto r = ::read(_fd, buffer, len);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "read");
        return { size_t(r) };
    }
    boost::optional<ssize_t> recv(void* buffer, size_t len, int flags) {
        auto r = ::recv(_fd, buffer, len, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "recv");
        return { ssize_t(r) };
    }
    boost::optional<size_t> recvmsg(msghdr* mh, int flags) {
        auto r = ::recvmsg(_fd, mh, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "recvmsg");
        return { size_t(r) };
    }
    boost::optional<size_t> send(const void* buffer, size_t len, int flags) {
        auto r = ::send(_fd, buffer, len, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "send");
        return { size_t(r) };
    }
    boost::optional<size_t> sendto(socket_address& addr, const void* buf, size_t len, int flags) {
        auto r = ::sendto(_fd, buf, len, flags, &addr.u.sa, addr.length());
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "sendto");
        return { size_t(r) };
    }
    boost::optional<size_t> sendmsg(const msghdr* msg, int flags) {
        auto r = ::sendmsg(_fd, msg, flags);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "sendmsg");
        return { size_t(r) };
    }
    void bind(sockaddr& sa, socklen_t sl) {
        auto r = ::bind(_fd, &sa, sl);
        throw_system_error_on(r == -1, "bind");
    }
    void connect(sockaddr& sa, socklen_t sl) {
        auto r = ::connect(_fd, &sa, sl);
        if (r == -1 && errno == EINPROGRESS) {
            return;
        }
        throw_system_error_on(r == -1, "connect");
    }
    socket_address get_address() {
        socket_address addr;
        auto r = ::getsockname(_fd, &addr.u.sa, &addr.addr_length);
        throw_system_error_on(r == -1, "getsockname");
        return addr;
    }
    void listen(int backlog) {
        auto fd = ::listen(_fd, backlog);
        throw_system_error_on(fd == -1, "listen");
    }
    boost::optional<size_t> write(const void* buf, size_t len) {
        auto r = ::write(_fd, buf, len);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "write");
        return { size_t(r) };
    }
    boost::optional<size_t> writev(const iovec *iov, int iovcnt) {
        auto r = ::writev(_fd, iov, iovcnt);
        if (r == -1 && errno == EAGAIN) {
            return {};
        }
        throw_system_error_on(r == -1, "writev");
        return { size_t(r) };
    }
    size_t pread(void* buf, size_t len, off_t off) {
        auto r = ::pread(_fd, buf, len, off);
        throw_system_error_on(r == -1, "pread");
        return size_t(r);
    }
    void timerfd_settime(int flags, const itimerspec& its) {
        auto r = ::timerfd_settime(_fd, flags, &its, NULL);
        throw_system_error_on(r == -1, "timerfd_settime");
    }

    mmap_area map(size_t size, unsigned prot, unsigned flags, size_t offset,
            void* addr = nullptr) {
        void *x = mmap(addr, size, prot, flags, _fd, offset);
        throw_system_error_on(x == MAP_FAILED, "mmap");
        return mmap_area(static_cast<char*>(x), mmap_deleter{size});
    }

    mmap_area map_shared_rw(size_t size, size_t offset) {
        return map(size, PROT_READ | PROT_WRITE, MAP_SHARED, offset);
    }

    mmap_area map_shared_ro(size_t size, size_t offset) {
        return map(size, PROT_READ, MAP_SHARED, offset);
    }

    mmap_area map_private_rw(size_t size, size_t offset) {
        return map(size, PROT_READ | PROT_WRITE, MAP_PRIVATE, offset);
    }

    mmap_area map_private_ro(size_t size, size_t offset) {
        return map(size, PROT_READ, MAP_PRIVATE, offset);
    }

private:
    file_desc(int fd) : _fd(fd) {}
 };


namespace posix {

/// Converts a duration value to a `timespec`
///
/// \param d a duration value to convert to the POSIX `timespec` format
/// \return `d` as a `timespec` value
template <typename Rep, typename Period>
struct timespec
to_timespec(std::chrono::duration<Rep, Period> d) {
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    struct timespec ts {};
    ts.tv_sec = ns / 1000000000;
    ts.tv_nsec = ns % 1000000000;
    return ts;
}

/// Converts a relative start time and an interval to an `itimerspec`
///
/// \param base First expiration of the timer, relative to the current time
/// \param interval period for re-arming the timer
/// \return `base` and `interval` converted to an `itimerspec`
template <typename Rep1, typename Period1, typename Rep2, typename Period2>
struct itimerspec
to_relative_itimerspec(std::chrono::duration<Rep1, Period1> base, std::chrono::duration<Rep2, Period2> interval) {
    struct itimerspec its {};
    its.it_interval = to_timespec(interval);
    its.it_value = to_timespec(base);
    return its;
}


/// Converts a time_point and a duration to an `itimerspec`
///
/// \param base  base time for the timer; must use the same clock as the timer
/// \param interval period for re-arming the timer
/// \return `base` and `interval` converted to an `itimerspec`
template <typename Clock, class Duration, class Rep, class Period>
struct itimerspec
to_absolute_itimerspec(std::chrono::time_point<Clock, Duration> base, std::chrono::duration<Rep, Period> interval) {
    return to_relative_itimerspec(base.time_since_epoch(), interval);
}

}

class posix_thread {
public:
    class attr;
private:
    // must allocate, since this class is moveable
    std::unique_ptr<std::function<void ()>> _func;
    pthread_t _pthread;
    bool _valid = true;
    mmap_area _stack;
private:
    static void* start_routine(void* arg) noexcept;
public:
    posix_thread(std::function<void ()> func);
    posix_thread(attr a, std::function<void ()> func);
    posix_thread(posix_thread&& x);
    ~posix_thread();
    void join();
public:
    class attr {
    public:
        struct stack_size { size_t size = 0; };
        attr() = default;
        template <typename... A>
        attr(A... a) {
            set(std::forward<A>(a)...);
        }
        void set() {}
        template <typename A, typename... Rest>
        void set(A a, Rest... rest) {
            set(std::forward<A>(a));
            set(std::forward<Rest>(rest)...);
        }
        void set(stack_size ss) { _stack_size = ss; }
    private:
        stack_size _stack_size;
        friend class posix_thread;
    };
};


inline
void throw_system_error_on(bool condition, const char* what_arg) {
    if (condition) {
        if ((errno == EBADF || errno == ENOTSOCK) && is_abort_on_ebadf_enabled()) {
            abort();
        }
        throw std::system_error(errno, std::system_category(), what_arg);
    }
}

template <typename T>
inline
void throw_kernel_error(T r) {
    static_assert(std::is_signed<T>::value, "kernel error variables must be signed");
    if (r < 0) {
        auto ec = -r;
        if ((ec == EBADF || ec == ENOTSOCK) && is_abort_on_ebadf_enabled()) {
            abort();
        }
        throw std::system_error(-r, std::system_category());
    }
}

template <typename T>
inline
void throw_pthread_error(T r) {
    if (r != 0) {
        throw std::system_error(r, std::system_category());
    }
}

inline
sigset_t make_sigset_mask(int signo) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, signo);
    return set;
}

inline
sigset_t make_full_sigset_mask() {
    sigset_t set;
    sigfillset(&set);
    return set;
}

inline
sigset_t make_empty_sigset_mask() {
    sigset_t set;
    sigemptyset(&set);
    return set;
}

inline
void pin_this_thread(unsigned cpu_id) {
    cpu_set_t cs;
    CPU_ZERO(&cs);
    CPU_SET(cpu_id, &cs);
    auto r = pthread_setaffinity_np(pthread_self(), sizeof(cs), &cs);
    assert(r == 0);
    (void)r;
}

/// @}

}

#include <vector>

namespace seastar {

inline
bool is_ip_unspecified(const ipv4_addr& addr) {
    return addr.is_ip_unspecified();
}

inline
bool is_port_unspecified(const ipv4_addr& addr) {
    return addr.is_port_unspecified();
}

inline
socket_address make_ipv4_address(const ipv4_addr& addr) {
    return socket_address(addr);
}

inline
socket_address make_ipv4_address(uint32_t ip, uint16_t port) {
    return make_ipv4_address(ipv4_addr(ip, port));
}

namespace net {

// see linux tcp(7) for parameter explanation
struct tcp_keepalive_params {
    std::chrono::seconds idle; // TCP_KEEPIDLE
    std::chrono::seconds interval; // TCP_KEEPINTVL
    unsigned count; // TCP_KEEPCNT
};

// see linux sctp(7) for parameter explanation
struct sctp_keepalive_params {
    std::chrono::seconds interval; // spp_hbinterval
    unsigned count; // spp_pathmaxrt
};

using keepalive_params = compat::variant<tcp_keepalive_params, sctp_keepalive_params>;

/// \cond internal
class connected_socket_impl;
class socket_impl;

#if SEASTAR_API_LEVEL <= 1

SEASTAR_INCLUDE_API_V1 namespace api_v1 { class server_socket_impl; }

#endif

SEASTAR_INCLUDE_API_V2 namespace api_v2 { class server_socket_impl; }
class udp_channel_impl;
class get_impl;
/// \endcond

class udp_datagram_impl {
public:
    virtual ~udp_datagram_impl() {};
    virtual socket_address get_src() = 0;
    virtual socket_address get_dst() = 0;
    virtual uint16_t get_dst_port() = 0;
    virtual packet& get_data() = 0;
};

class udp_datagram final {
private:
    std::unique_ptr<udp_datagram_impl> _impl;
public:
    udp_datagram(std::unique_ptr<udp_datagram_impl>&& impl) : _impl(std::move(impl)) {};
    socket_address get_src() { return _impl->get_src(); }
    socket_address get_dst() { return _impl->get_dst(); }
    uint16_t get_dst_port() { return _impl->get_dst_port(); }
    packet& get_data() { return _impl->get_data(); }
};

class udp_channel {
private:
    std::unique_ptr<udp_channel_impl> _impl;
public:
    udp_channel();
    udp_channel(std::unique_ptr<udp_channel_impl>);
    ~udp_channel();

    udp_channel(udp_channel&&);
    udp_channel& operator=(udp_channel&&);

    socket_address local_address() const;

    future<udp_datagram> receive();
    future<> send(const socket_address& dst, const char* msg);
    future<> send(const socket_address& dst, packet p);
    bool is_closed() const;
    /// Causes a pending receive() to complete (possibly with an exception)
    void shutdown_input();
    /// Causes a pending send() to complete (possibly with an exception)
    void shutdown_output();
    /// Close the channel and releases all resources.
    ///
    /// Must be called only when there are no unfinished send() or receive() calls. You
    /// can force pending calls to complete soon by calling shutdown_input() and
    /// shutdown_output().
    void close();
};

class network_interface_impl;

} /* namespace net */

/// \addtogroup networking-module
/// @{

/// A TCP (or other stream-based protocol) connection.
///
/// A \c connected_socket represents a full-duplex stream between
/// two endpoints, a local endpoint and a remote endpoint.
class connected_socket {
    friend class net::get_impl;
    std::unique_ptr<net::connected_socket_impl> _csi;
public:
    /// Constructs a \c connected_socket not corresponding to a connection
    connected_socket();
    ~connected_socket();

    /// \cond internal
    explicit connected_socket(std::unique_ptr<net::connected_socket_impl> csi);
    /// \endcond
    /// Moves a \c connected_socket object.
    connected_socket(connected_socket&& cs) noexcept;
    /// Move-assigns a \c connected_socket object.
    connected_socket& operator=(connected_socket&& cs) noexcept;
    /// Gets the input stream.
    ///
    /// Gets an object returning data sent from the remote endpoint.
    input_stream<char> input();
    /// Gets the output stream.
    ///
    /// Gets an object that sends data to the remote endpoint.
    /// \param buffer_size how much data to buffer
    output_stream<char> output(size_t buffer_size = 8192);
    /// Sets the TCP_NODELAY option (disabling Nagle's algorithm)
    void set_nodelay(bool nodelay);
    /// Gets the TCP_NODELAY option (Nagle's algorithm)
    ///
    /// \return whether the nodelay option is enabled or not
    bool get_nodelay() const;
    /// Sets SO_KEEPALIVE option (enable keepalive timer on a socket)
    void set_keepalive(bool keepalive);
    /// Gets O_KEEPALIVE option
    /// \return whether the keepalive option is enabled or not
    bool get_keepalive() const;
    /// Sets TCP keepalive parameters
    void set_keepalive_parameters(const net::keepalive_params& p);
    /// Get TCP keepalive parameters
    net::keepalive_params get_keepalive_parameters() const;

    /// Disables output to the socket.
    ///
    /// Current or future writes that have not been successfully flushed
    /// will immediately fail with an error.  This is useful to abort
    /// operations on a socket that is not making progress due to a
    /// peer failure.
    void shutdown_output();
    /// Disables input from the socket.
    ///
    /// Current or future reads will immediately fail with an error.
    /// This is useful to abort operations on a socket that is not making
    /// progress due to a peer failure.
    void shutdown_input();
};
/// @}

/// \addtogroup networking-module
/// @{

/// The seastar socket.
///
/// A \c socket that allows a connection to be established between
/// two endpoints.
class socket {
    std::unique_ptr<net::socket_impl> _si;
public:
    ~socket();

    /// \cond internal
    explicit socket(std::unique_ptr<net::socket_impl> si);
    /// \endcond
    /// Moves a \c seastar::socket object.
    socket(socket&&) noexcept;
    /// Move-assigns a \c seastar::socket object.
    socket& operator=(socket&&) noexcept;

    /// Attempts to establish the connection.
    ///
    /// \return a \ref connected_socket representing the connection.
    future<connected_socket> connect(socket_address sa, socket_address local = {}, transport proto = transport::TCP);

    /// Sets SO_REUSEADDR option (enable reuseaddr option on a socket)
    void set_reuseaddr(bool reuseaddr);
    /// Gets O_REUSEADDR option
    /// \return whether the reuseaddr option is enabled or not
    bool get_reuseaddr() const;
    /// Stops any in-flight connection attempt.
    ///
    /// Cancels the connection attempt if it's still in progress, and
    /// terminates the connection if it has already been established.
    void shutdown();
};

/// @}

/// \addtogroup networking-module
/// @{

/// The result of an server_socket::accept() call
struct accept_result {
    connected_socket connection;  ///< The newly-accepted connection
    socket_address remote_address;  ///< The address of the peer that connected to us
};

SEASTAR_INCLUDE_API_V2 namespace api_v2 {

/// A listening socket, waiting to accept incoming network connections.
class server_socket {
    std::unique_ptr<net::api_v2::server_socket_impl> _ssi;
    bool _aborted = false;
public:
    enum class load_balancing_algorithm {
        // This algorithm tries to distribute all connections equally between all shards.
        // It does this by sending new connections to a shard with smallest amount of connections.
        connection_distribution,
        // This algorithm distributes new connection based on peer's tcp port. Destination shard
        // is calculated as a port number modulo number of shards. This allows a client to connect
        // to a specific shard in a server given it knows how many shards server has by choosing
        // src port number accordingly.
        port,
        // This algorithm distributes all new connections to listen_options::fixed_cpu shard only.
        fixed,
        default_ = connection_distribution
    };
    /// Constructs a \c server_socket not corresponding to a connection
    server_socket();
    /// \cond internal
    explicit server_socket(std::unique_ptr<net::api_v2::server_socket_impl> ssi);
    /// \endcond
    /// Moves a \c server_socket object.
    server_socket(server_socket&& ss) noexcept;
    ~server_socket();
    /// Move-assigns a \c server_socket object.
    server_socket& operator=(server_socket&& cs) noexcept;

    /// Accepts the next connection to successfully connect to this socket.
    ///
    /// \return an accept_result representing the connection and
    ///         the socket_address of the remote endpoint.
    ///
    /// \see listen(socket_address sa)
    /// \see listen(socket_address sa, listen_options opts)
    future<accept_result> accept();

    /// Stops any \ref accept() in progress.
    ///
    /// Current and future \ref accept() calls will terminate immediately
    /// with an error.
    void abort_accept();

    /// Local bound address
    socket_address local_address() const;
};

}

#if SEASTAR_API_LEVEL <= 1

SEASTAR_INCLUDE_API_V1 namespace api_v1 {

class server_socket {
    api_v2::server_socket _impl;
private:
    static api_v2::server_socket make_v2_server_socket(std::unique_ptr<net::api_v1::server_socket_impl>);
public:
    using load_balancing_algorithm = api_v2::server_socket::load_balancing_algorithm;
    server_socket();
    explicit server_socket(std::unique_ptr<net::api_v1::server_socket_impl> ssi);
    explicit server_socket(std::unique_ptr<net::api_v2::server_socket_impl> ssi);
    server_socket(server_socket&& ss) noexcept;
    server_socket(api_v2::server_socket&& ss);
    ~server_socket();
    operator api_v2::server_socket() &&;
    server_socket& operator=(server_socket&& cs) noexcept;
    future<connected_socket, socket_address> accept();
    void abort_accept();
    socket_address local_address() const;
};

}

#endif

/// @}

struct listen_options {
    bool reuse_address = false;
    server_socket::load_balancing_algorithm lba = server_socket::load_balancing_algorithm::default_;
    transport proto = transport::TCP;
    int listen_backlog = 100;
    unsigned fixed_cpu = 0u;
    void set_fixed_cpu(unsigned cpu) {
        lba = server_socket::load_balancing_algorithm::fixed;
        fixed_cpu = cpu;
    }
};

class network_interface {
private:
    shared_ptr<net::network_interface_impl> _impl;
public:
    network_interface(shared_ptr<net::network_interface_impl>);
    network_interface(network_interface&&);

    network_interface& operator=(network_interface&&);

    uint32_t index() const;
    uint32_t mtu() const;

    const sstring& name() const;
    const sstring& display_name() const;
    const std::vector<net::inet_address>& addresses() const;
    const std::vector<uint8_t> hardware_address() const;

    bool is_loopback() const;
    bool is_virtual() const;
    bool is_up() const;
    bool supports_ipv6() const;
};

class network_stack {
public:
    virtual ~network_stack() {}
    virtual server_socket listen(socket_address sa, listen_options opts) = 0;
    // FIXME: local parameter assumes ipv4 for now, fix when adding other AF
    future<connected_socket> connect(socket_address sa, socket_address = {}, transport proto = transport::TCP);
    virtual ::seastar::socket socket() = 0;
    virtual net::udp_channel make_udp_channel(const socket_address& = {}) = 0;
    virtual future<> initialize() {
        return make_ready_future();
    }
    virtual bool has_per_core_namespace() = 0;
    // NOTE: this is not a correct query approach.
    // This question should be per NIC, but we have no such
    // abstraction, so for now this is "stack-wide"
    virtual bool supports_ipv6() const {
        return false;
    }

    /**
     * Returns available network interfaces. This represents a
     * snapshot of interfaces available at call time, hence the
     * return by value.
     */
    virtual std::vector<network_interface> network_interfaces();
};

}

#include <utility>

namespace seastar {

template <typename T, typename Alloc>
inline
void
transfer_pass1(Alloc& a, T* from, T* to,
        typename std::enable_if<std::is_nothrow_move_constructible<T>::value>::type* = nullptr) {
    a.construct(to, std::move(*from));
    a.destroy(from);
}

template <typename T, typename Alloc>
inline
void
transfer_pass2(Alloc& a, T* from, T* to,
        typename std::enable_if<std::is_nothrow_move_constructible<T>::value>::type* = nullptr) {
}

template <typename T, typename Alloc>
inline
void
transfer_pass1(Alloc& a, T* from, T* to,
        typename std::enable_if<!std::is_nothrow_move_constructible<T>::value>::type* = nullptr) {
    a.construct(to, *from);
}

template <typename T, typename Alloc>
inline
void
transfer_pass2(Alloc& a, T* from, T* to,
        typename std::enable_if<!std::is_nothrow_move_constructible<T>::value>::type* = nullptr) {
    a.destroy(from);
}

}


#include <limits>

namespace seastar {

inline
constexpr unsigned count_leading_zeros(unsigned x) {
    return __builtin_clz(x);
}

inline
constexpr unsigned count_leading_zeros(unsigned long x) {
    return __builtin_clzl(x);
}

inline
constexpr unsigned count_leading_zeros(unsigned long long x) {
    return __builtin_clzll(x);
}

inline
constexpr unsigned count_trailing_zeros(unsigned x) {
    return __builtin_ctz(x);
}

inline
constexpr unsigned count_trailing_zeros(unsigned long x) {
    return __builtin_ctzl(x);
}

inline
constexpr unsigned count_trailing_zeros(unsigned long long x) {
    return __builtin_ctzll(x);
}

template<typename T>
//requires stdx::is_integral_v<T>
inline constexpr unsigned log2ceil(T n) {
    if (n == 1) {
        return 0;
    }
    return std::numeric_limits<T>::digits - count_leading_zeros(n - 1);
}

template<typename T>
//requires stdx::is_integral_v<T>
inline constexpr unsigned log2floor(T n) {
    return std::numeric_limits<T>::digits - count_leading_zeros(n) - 1;
}

}

namespace seastar {

/// A growable double-ended queue container that can be efficiently
/// extended (and shrunk) from both ends. Implementation is a single
/// storage vector.
///
/// Similar to libstdc++'s std::deque, except that it uses a single
/// level store, and so is more efficient for simple stored items.
/// Similar to boost::circular_buffer_space_optimized, except it uses
/// uninitialized storage for unoccupied elements (and thus move/copy
/// constructors instead of move/copy assignments, which are less
/// efficient).
///
/// The storage of the circular_buffer is expanded automatically in
/// exponential increments.
/// When adding new elements:
/// * if size + 1 > capacity: all iterators and references are
///     invalidated,
/// * otherwise only the begin() or end() iterator is invalidated:
///     * push_front() and emplace_front() will invalidate begin() and
///     * push_back() and emplace_back() will invalidate end().
///
/// Removing elements never invalidates any references and only
/// invalidates begin() or end() iterators:
///     * pop_front() will invalidate begin() and
///     * pop_back() will invalidate end().
///
/// reserve() may also invalidate all iterators and references.
template <typename T, typename Alloc = std::allocator<T>>
class circular_buffer {
    struct impl : Alloc {
        T* storage = nullptr;
        // begin, end interpreted (mod capacity)
        size_t begin = 0;
        size_t end = 0;
        size_t capacity = 0;
    };
    impl _impl;
public:
    using value_type = T;
    using size_type = size_t;
    using reference = T&;
    using pointer = T*;
    using const_reference = const T&;
    using const_pointer = const T*;
public:
    circular_buffer() = default;
    circular_buffer(circular_buffer&& X) noexcept;
    circular_buffer(const circular_buffer& X) = delete;
    ~circular_buffer();
    circular_buffer& operator=(const circular_buffer&) = delete;
    circular_buffer& operator=(circular_buffer&& b) noexcept;
    void push_front(const T& data);
    void push_front(T&& data);
    template <typename... A>
    void emplace_front(A&&... args);
    void push_back(const T& data);
    void push_back(T&& data);
    template <typename... A>
    void emplace_back(A&&... args);
    T& front();
    const T& front() const;
    T& back();
    const T& back() const;
    void pop_front();
    void pop_back();
    bool empty() const;
    size_t size() const;
    size_t capacity() const;
    void reserve(size_t);
    void clear();
    T& operator[](size_t idx);
    const T& operator[](size_t idx) const;
    template <typename Func>
    void for_each(Func func);
    // access an element, may return wrong or destroyed element
    // only useful if you do not rely on data accuracy (e.g. prefetch)
    T& access_element_unsafe(size_t idx);
private:
    void expand();
    void expand(size_t);
    void maybe_expand(size_t nr = 1);
    size_t mask(size_t idx) const;

    template<typename CB, typename ValueType>
    struct cbiterator : std::iterator<std::random_access_iterator_tag, ValueType> {
        typedef std::iterator<std::random_access_iterator_tag, ValueType> super_t;

        ValueType& operator*() const { return cb->_impl.storage[cb->mask(idx)]; }
        ValueType* operator->() const { return &cb->_impl.storage[cb->mask(idx)]; }
        // prefix
        cbiterator<CB, ValueType>& operator++() {
            idx++;
            return *this;
        }
        // postfix
        cbiterator<CB, ValueType> operator++(int unused) {
            auto v = *this;
            idx++;
            return v;
        }
        // prefix
        cbiterator<CB, ValueType>& operator--() {
            idx--;
            return *this;
        }
        // postfix
        cbiterator<CB, ValueType> operator--(int unused) {
            auto v = *this;
            idx--;
            return v;
        }
        cbiterator<CB, ValueType> operator+(typename super_t::difference_type n) const {
            return cbiterator<CB, ValueType>(cb, idx + n);
        }
        cbiterator<CB, ValueType> operator-(typename super_t::difference_type n) const {
            return cbiterator<CB, ValueType>(cb, idx - n);
        }
        cbiterator<CB, ValueType>& operator+=(typename super_t::difference_type n) {
            idx += n;
            return *this;
        }
        cbiterator<CB, ValueType>& operator-=(typename super_t::difference_type n) {
            idx -= n;
            return *this;
        }
        bool operator==(const cbiterator<CB, ValueType>& rhs) const {
            return idx == rhs.idx;
        }
        bool operator!=(const cbiterator<CB, ValueType>& rhs) const {
            return idx != rhs.idx;
        }
        bool operator<(const cbiterator<CB, ValueType>& rhs) const {
            return idx < rhs.idx;
        }
        bool operator>(const cbiterator<CB, ValueType>& rhs) const {
            return idx > rhs.idx;
        }
        bool operator>=(const cbiterator<CB, ValueType>& rhs) const {
            return idx >= rhs.idx;
        }
        bool operator<=(const cbiterator<CB, ValueType>& rhs) const {
            return idx <= rhs.idx;
        }
       typename super_t::difference_type operator-(const cbiterator<CB, ValueType>& rhs) const {
            return idx - rhs.idx;
        }
    private:
        CB* cb;
        size_t idx;
        cbiterator<CB, ValueType>(CB* b, size_t i) : cb(b), idx(i) {}
        friend class circular_buffer;
    };
    friend class iterator;

public:
    typedef cbiterator<circular_buffer, T> iterator;
    typedef cbiterator<const circular_buffer, const T> const_iterator;

    iterator begin() {
        return iterator(this, _impl.begin);
    }
    const_iterator begin() const {
        return const_iterator(this, _impl.begin);
    }
    iterator end() {
        return iterator(this, _impl.end);
    }
    const_iterator end() const {
        return const_iterator(this, _impl.end);
    }
    const_iterator cbegin() const {
        return const_iterator(this, _impl.begin);
    }
    const_iterator cend() const {
        return const_iterator(this, _impl.end);
    }
    iterator erase(iterator first, iterator last);
};

template <typename T, typename Alloc>
inline
size_t
circular_buffer<T, Alloc>::mask(size_t idx) const {
    return idx & (_impl.capacity - 1);
}

template <typename T, typename Alloc>
inline
bool
circular_buffer<T, Alloc>::empty() const {
    return _impl.begin == _impl.end;
}

template <typename T, typename Alloc>
inline
size_t
circular_buffer<T, Alloc>::size() const {
    return _impl.end - _impl.begin;
}

template <typename T, typename Alloc>
inline
size_t
circular_buffer<T, Alloc>::capacity() const {
    return _impl.capacity;
}

template <typename T, typename Alloc>
inline
void
circular_buffer<T, Alloc>::reserve(size_t size) {
    if (capacity() < size) {
        // Make sure that the new capacity is a power of two.
        expand(size_t(1) << log2ceil(size));
    }
}

template <typename T, typename Alloc>
inline
void
circular_buffer<T, Alloc>::clear() {
    erase(begin(), end());
}

template <typename T, typename Alloc>
inline
circular_buffer<T, Alloc>::circular_buffer(circular_buffer&& x) noexcept
    : _impl(std::move(x._impl)) {
    x._impl = {};
}

template <typename T, typename Alloc>
inline
circular_buffer<T, Alloc>& circular_buffer<T, Alloc>::operator=(circular_buffer&& x) noexcept {
    if (this != &x) {
        this->~circular_buffer();
        new (this) circular_buffer(std::move(x));
    }
    return *this;
}

template <typename T, typename Alloc>
template <typename Func>
inline
void
circular_buffer<T, Alloc>::for_each(Func func) {
    auto s = _impl.storage;
    auto m = _impl.capacity - 1;
    for (auto i = _impl.begin; i != _impl.end; ++i) {
        func(s[i & m]);
    }
}

template <typename T, typename Alloc>
inline
circular_buffer<T, Alloc>::~circular_buffer() {
    for_each([this] (T& obj) {
        _impl.destroy(&obj);
    });
    _impl.deallocate(_impl.storage, _impl.capacity);
}

template <typename T, typename Alloc>
void
circular_buffer<T, Alloc>::expand() {
    expand(std::max<size_t>(_impl.capacity * 2, 1));
}

template <typename T, typename Alloc>
void
circular_buffer<T, Alloc>::expand(size_t new_cap) {
    auto new_storage = _impl.allocate(new_cap);
    auto p = new_storage;
    try {
        for_each([this, &p] (T& obj) {
            transfer_pass1(_impl, &obj, p);
            p++;
        });
    } catch (...) {
        while (p != new_storage) {
            _impl.destroy(--p);
        }
        _impl.deallocate(new_storage, new_cap);
        throw;
    }
    p = new_storage;
    for_each([this, &p] (T& obj) {
        transfer_pass2(_impl, &obj, p++);
    });
    std::swap(_impl.storage, new_storage);
    std::swap(_impl.capacity, new_cap);
    _impl.begin = 0;
    _impl.end = p - _impl.storage;
    _impl.deallocate(new_storage, new_cap);
}

template <typename T, typename Alloc>
inline
void
circular_buffer<T, Alloc>::maybe_expand(size_t nr) {
    if (_impl.end - _impl.begin + nr > _impl.capacity) {
        expand();
    }
}

template <typename T, typename Alloc>
inline
void
circular_buffer<T, Alloc>::push_front(const T& data) {
    maybe_expand();
    auto p = &_impl.storage[mask(_impl.begin - 1)];
    _impl.construct(p, data);
    --_impl.begin;
}

template <typename T, typename Alloc>
inline
void
circular_buffer<T, Alloc>::push_front(T&& data) {
    maybe_expand();
    auto p = &_impl.storage[mask(_impl.begin - 1)];
    _impl.construct(p, std::move(data));
    --_impl.begin;
}

template <typename T, typename Alloc>
template <typename... Args>
inline
void
circular_buffer<T, Alloc>::emplace_front(Args&&... args) {
    maybe_expand();
    auto p = &_impl.storage[mask(_impl.begin - 1)];
    _impl.construct(p, std::forward<Args>(args)...);
    --_impl.begin;
}

template <typename T, typename Alloc>
inline
void
circular_buffer<T, Alloc>::push_back(const T& data) {
    maybe_expand();
    auto p = &_impl.storage[mask(_impl.end)];
    _impl.construct(p, data);
    ++_impl.end;
}

template <typename T, typename Alloc>
inline
void
circular_buffer<T, Alloc>::push_back(T&& data) {
    maybe_expand();
    auto p = &_impl.storage[mask(_impl.end)];
    _impl.construct(p, std::move(data));
    ++_impl.end;
}

template <typename T, typename Alloc>
template <typename... Args>
inline
void
circular_buffer<T, Alloc>::emplace_back(Args&&... args) {
    maybe_expand();
    auto p = &_impl.storage[mask(_impl.end)];
    _impl.construct(p, std::forward<Args>(args)...);
    ++_impl.end;
}

template <typename T, typename Alloc>
inline
T&
circular_buffer<T, Alloc>::front() {
    return _impl.storage[mask(_impl.begin)];
}

template <typename T, typename Alloc>
inline
const T&
circular_buffer<T, Alloc>::front() const {
    return _impl.storage[mask(_impl.begin)];
}

template <typename T, typename Alloc>
inline
T&
circular_buffer<T, Alloc>::back() {
    return _impl.storage[mask(_impl.end - 1)];
}

template <typename T, typename Alloc>
inline
const T&
circular_buffer<T, Alloc>::back() const {
    return _impl.storage[mask(_impl.end - 1)];
}

template <typename T, typename Alloc>
inline
void
circular_buffer<T, Alloc>::pop_front() {
    _impl.destroy(&front());
    ++_impl.begin;
}

template <typename T, typename Alloc>
inline
void
circular_buffer<T, Alloc>::pop_back() {
    _impl.destroy(&back());
    --_impl.end;
}

template <typename T, typename Alloc>
inline
T&
circular_buffer<T, Alloc>::operator[](size_t idx) {
    return _impl.storage[mask(_impl.begin + idx)];
}

template <typename T, typename Alloc>
inline
const T&
circular_buffer<T, Alloc>::operator[](size_t idx) const {
    return _impl.storage[mask(_impl.begin + idx)];
}

template <typename T, typename Alloc>
inline
T&
circular_buffer<T, Alloc>::access_element_unsafe(size_t idx) {
    return _impl.storage[mask(_impl.begin + idx)];
}

template <typename T, typename Alloc>
inline
typename circular_buffer<T, Alloc>::iterator
circular_buffer<T, Alloc>::erase(iterator first, iterator last) {
    static_assert(std::is_nothrow_move_assignable<T>::value, "erase() assumes move assignment does not throw");
    if (first == last) {
        return last;
    }
    // Move to the left or right depending on which would result in least amount of moves.
    // This also guarantees that iterators will be stable when removing from either front or back.
    if (std::distance(begin(), first) < std::distance(last, end())) {
        auto new_start = std::move_backward(begin(), first, last);
        auto i = begin();
        while (i < new_start) {
            _impl.destroy(&*i++);
        }
        _impl.begin = new_start.idx;
        return last;
    } else {
        auto new_end = std::move(last, end(), first);
        auto i = new_end;
        auto e = end();
        while (i < e) {
            _impl.destroy(&*i++);
        }
        _impl.end = new_end.idx;
        return first;
    }
}

}
#include <functional>

namespace seastar {

// A stream/subscription pair is similar to a promise/future pair,
// but apply to a sequence of values instead of a single value.
//
// A stream<> is the producer side.  It may call produce() as long
// as the future<> returned from the previous invocation is ready.
// To signify no more data is available, call close().
//
// A subscription<> is the consumer side.  It is created by a call
// to stream::listen().  Calling subscription::start(),
// which registers the data processing callback, starts processing
// events.  It may register for end-of-stream notifications by
// chaining the when_done() future, which also delivers error
// events (as exceptions).
//
// The consumer can pause generation of new data by returning
// a non-ready future; when the future becomes ready, the producer
// will resume processing.

template <typename... T>
class stream;

template <typename... T>
class subscription;

template <typename... T>
class stream {
public:
    using next_fn = noncopyable_function<future<> (T...)>;

private:
    subscription<T...>* _sub = nullptr;
    promise<> _done;
    promise<> _ready;
    next_fn _next;

    /// \brief Start receiving events from the stream.
    ///
    /// \param next Callback to call for each event
    void start(next_fn next) {
        _next = std::move(next);
        _ready.set_value();
    }

public:
    stream() = default;
    stream(const stream&) = delete;
    stream(stream&&) = delete;
    ~stream() {
        if (_sub) {
            _sub->_stream = nullptr;
        }
    }
    void operator=(const stream&) = delete;
    void operator=(stream&&) = delete;

    // Returns a subscription that reads value from this
    // stream.
    subscription<T...> listen() {
        return subscription<T...>(this);
    }

    // Returns a subscription that reads value from this
    // stream, and also sets up the listen function.
    subscription<T...> listen(next_fn next) {
        start(std::move(next));
        return subscription<T...>(this);
    }

    // Becomes ready when the listener is ready to accept
    // values.  Call only once, when beginning to produce
    // values.
    future<> started() {
        return _ready.get_future();
    }

    // Produce a value.  Call only after started(), and after
    // a previous produce() is ready.
    future<> produce(T... data);

    // End the stream.   Call only after started(), and after
    // a previous produce() is ready.  No functions may be called
    // after this.
    void close() {
        _done.set_value();
    }

    // Signal an error.   Call only after started(), and after
    // a previous produce() is ready.  No functions may be called
    // after this.
    template <typename E>
    void set_exception(E ex) {
        _done.set_exception(ex);
    }

    friend class subscription<T...>;
};

template <typename... T>
class subscription {
    stream<T...>* _stream;
    future<> _done;
    explicit subscription(stream<T...>* s) : _stream(s), _done(s->_done.get_future()) {
        assert(!_stream->_sub);
        _stream->_sub = this;
    }

public:
    using next_fn = typename stream<T...>::next_fn;
    subscription(subscription&& x) : _stream(x._stream), _done(std::move(x._done)) {
        x._stream = nullptr;
        if (_stream) {
            _stream->_sub = this;
        }
    }
    ~subscription() {
        if (_stream) {
            _stream->_sub = nullptr;
        }
    }

    /// \brief Start receiving events from the stream.
    ///
    /// \param next Callback to call for each event
    void start(next_fn next) {
        return _stream->start(std::move(next));
    }

    // Becomes ready when the stream is empty, or when an error
    // happens (in that case, an exception is held).
    future<> done() {
        return std::move(_done);
    }

    friend class stream<T...>;
};

template <typename... T>
inline
future<>
stream<T...>::produce(T... data) {
    auto ret = futurize<void>::apply(_next, std::move(data)...);
    if (ret.available() && !ret.failed()) {
        // Native network stack depends on stream::produce() returning
        // a ready future to push packets along without dropping.  As
        // a temporary workaround, special case a ready, unfailed future
        // and return it immediately, so that then_wrapped(), below,
        // doesn't convert a ready future to an unready one.
        return ret;
    }
    return ret.then_wrapped([this] (auto&& f) {
        try {
            f.get();
        } catch (...) {
            _done.set_exception(std::current_exception());
            // FIXME: tell the producer to stop producing
            throw;
        }
    });
}
}

#include <cstdlib>

namespace seastar {

template <typename T>
inline constexpr
T align_up(T v, T align) {
    return (v + align - 1) & ~(align - 1);
}

template <typename T>
inline constexpr
T* align_up(T* v, size_t align) {
    static_assert(sizeof(T) == 1, "align byte pointers only");
    return reinterpret_cast<T*>(align_up(reinterpret_cast<uintptr_t>(v), align));
}

template <typename T>
inline constexpr
T align_down(T v, T align) {
    return v & ~(align - 1);
}

template <typename T>
inline constexpr
T* align_down(T* v, size_t align) {
    static_assert(sizeof(T) == 1, "align byte pointers only");
    return reinterpret_cast<T*>(align_down(reinterpret_cast<uintptr_t>(v), align));
}

}
#include <unordered_set>

namespace seastar {

/// \brief describes a request that passes through the fair queue
///
/// \related fair_queue
struct fair_queue_request_descriptor {
    unsigned weight = 1; ///< the weight of this request for capacity purposes (IOPS).
    unsigned size = 1;        ///< the effective size of this request
};

/// \addtogroup io-module
/// @{

/// \cond internal
class priority_class {
    struct request {
        noncopyable_function<void()> func;
        fair_queue_request_descriptor desc;
    };
    friend class fair_queue;
    uint32_t _shares = 0;
    float _accumulated = 0;
    circular_buffer<request> _queue;
    bool _queued = false;

    friend struct shared_ptr_no_esft<priority_class>;
    explicit priority_class(uint32_t shares) : _shares(std::max(shares, 1u)) {}

    void update_shares(uint32_t shares) {
        _shares = (std::max(shares, 1u));
    }
public:
    /// \brief return the current amount of shares for this priority class
    uint32_t shares() const {
        return _shares;
    }
};
/// \endcond

/// \brief Priority class, to be used with a given \ref fair_queue
///
/// An instance of this class is associated with a given \ref fair_queue. When registering
/// a class, the caller will receive a \ref lw_shared_ptr to an object of this class. All its methods
/// are private, so the only thing the caller is expected to do with it is to pass it later
/// to the \ref fair_queue to identify a given class.
///
/// \related fair_queue
using priority_class_ptr = lw_shared_ptr<priority_class>;

/// \brief Fair queuing class
///
/// This is a fair queue, allowing multiple request producers to queue requests
/// that will then be served proportionally to their classes' shares.
///
/// To each request, a weight can also be associated. A request of weight 1 will consume
/// 1 share. Higher weights for a request will consume a proportionally higher amount of
/// shares.
///
/// The user of this interface is expected to register multiple \ref priority_class
/// objects, which will each have a shares attribute.
///
/// Internally, each priority class may keep a separate queue of requests.
/// Requests pertaining to a class can go through even if they are over its
/// share limit, provided that the other classes have empty queues.
///
/// When the classes that lag behind start seeing requests, the fair queue will serve
/// them first, until balance is restored. This balancing is expected to happen within
/// a certain time window that obeys an exponential decay.
class fair_queue {
public:
    /// \brief Fair Queue configuration structure.
    ///
    /// \sets the operation parameters of a \ref fair_queue
    /// \related fair_queue
    struct config {
        unsigned capacity = std::numeric_limits<unsigned>::max();
        std::chrono::microseconds tau = std::chrono::milliseconds(100);
        unsigned max_req_count = std::numeric_limits<unsigned>::max();
        unsigned max_bytes_count = std::numeric_limits<unsigned>::max();
    };
private:
    friend priority_class;

    struct class_compare {
        bool operator() (const priority_class_ptr& lhs, const priority_class_ptr& rhs) const {
            return lhs->_accumulated > rhs->_accumulated;
        }
    };

    config _config;
    unsigned _requests_executing = 0;
    unsigned _req_count_executing = 0;
    unsigned _bytes_count_executing = 0;
    unsigned _requests_queued = 0;
    using clock_type = std::chrono::steady_clock::time_point;
    clock_type _base;
    using prioq = std::priority_queue<priority_class_ptr, std::vector<priority_class_ptr>, class_compare>;
    prioq _handles;
    std::unordered_set<priority_class_ptr> _all_classes;

    void push_priority_class(priority_class_ptr pc);

    priority_class_ptr pop_priority_class();

    float normalize_factor() const;

    void normalize_stats();

    bool can_dispatch() const;
public:
    /// Constructs a fair queue with configuration parameters \c cfg.
    ///
    /// \param cfg an instance of the class \ref config
    explicit fair_queue(config cfg)
        : _config(std::move(cfg))
        , _base(std::chrono::steady_clock::now())
    {}

    /// Constructs a fair queue with a given \c capacity.
    ///
    /// \param capacity how many concurrent requests are allowed in this queue.
    /// \param tau the queue exponential decay parameter, as in exp(-1/tau * t)
    explicit fair_queue(unsigned capacity, std::chrono::microseconds tau = std::chrono::milliseconds(100))
        : fair_queue(config{capacity, tau}) {}

    /// Registers a priority class against this fair queue.
    ///
    /// \param shares, how many shares to create this class with
    priority_class_ptr register_priority_class(uint32_t shares);

    /// Unregister a priority class.
    ///
    /// It is illegal to unregister a priority class that still have pending requests.
    void unregister_priority_class(priority_class_ptr pclass);

    /// \return how many waiters are currently queued for all classes.
    size_t waiters() const;

    /// \return the number of requests currently executing
    size_t requests_currently_executing() const;

    /// Queue the function \c func through this class' \ref fair_queue, with weight \c weight
    ///
    /// It is expected that \c func doesn't throw. If it does throw, it will be just removed from
    /// the queue and discarded.
    ///
    /// The user of this interface is supposed to call \ref notify_requests_finished when the
    /// request finishes executing - regardless of success or failure.
    void queue(priority_class_ptr pc, fair_queue_request_descriptor desc, noncopyable_function<void()> func);

    /// Notifies that ont request finished
    /// \param desc an instance of \c fair_queue_request_descriptor structure describing the request that just finished.
    void notify_requests_finished(fair_queue_request_descriptor& desc);

    /// Try to execute new requests if there is capacity left in the queue.
    void dispatch_requests();

    /// Updates the current shares of this priority class
    ///
    /// \param new_shares the new number of shares for this priority class
    static void update_shares(priority_class_ptr pc, uint32_t new_shares);
};
/// @}

}
#include <sys/statvfs.h>
#include <linux/fs.h>
#include <unistd.h>

namespace seastar {

/// \addtogroup fileio-module
/// @{

/// A directory entry being listed.
struct directory_entry {
    /// Name of the file in a directory entry.  Will never be "." or "..".  Only the last component is included.
    sstring name;
    /// Type of the directory entry, if known.
    compat::optional<directory_entry_type> type;
};

/// Filesystem object stat information
struct stat_data {
    uint64_t  device_id;      // ID of device containing file
    uint64_t  inode_number;   // Inode number
    uint64_t  mode;           // File type and mode
    directory_entry_type type;
    uint64_t  number_of_links;// Number of hard links
    uint64_t  uid;            // User ID of owner
    uint64_t  gid;            // Group ID of owner
    uint64_t  rdev;           // Device ID (if special file)
    uint64_t  size;           // Total size, in bytes
    uint64_t  block_size;     // Block size for filesystem I/O
    uint64_t  allocated_size; // Total size of allocated storage, in bytes

    std::chrono::system_clock::time_point time_accessed;  // Time of last content access
    std::chrono::system_clock::time_point time_modified;  // Time of last content modification
    std::chrono::system_clock::time_point time_changed;   // Time of last status change (either content or attributes)
};

/// File open options
///
/// Options used to configure an open file.
///
/// \ref file
struct file_open_options {
    uint64_t extent_allocation_size_hint = 1 << 20; ///< Allocate this much disk space when extending the file
    bool sloppy_size = false; ///< Allow the file size not to track the amount of data written until a flush
    uint64_t sloppy_size_hint = 1 << 20; ///< Hint as to what the eventual file size will be
    file_permissions create_permissions = file_permissions::default_file_permissions; ///< File permissions to use when creating a file
};

/// \cond internal
class io_queue;
using io_priority_class_id = unsigned;
class io_priority_class {
    io_priority_class_id _id;
    friend io_queue;

    explicit io_priority_class(io_priority_class_id id)
        : _id(id)
    { }

public:
    io_priority_class_id id() const {
        return _id;
    }
};

const io_priority_class& default_priority_class();

class file;
class file_impl;

class file_handle;

// A handle that can be transported across shards and used to
// create a dup(2)-like `file` object referring to the same underlying file
class file_handle_impl {
public:
    virtual ~file_handle_impl() = default;
    virtual std::unique_ptr<file_handle_impl> clone() const = 0;
    virtual shared_ptr<file_impl> to_file() && = 0;
};

class file_impl {
protected:
    static file_impl* get_file_impl(file& f);
public:
    unsigned _memory_dma_alignment = 4096;
    unsigned _disk_read_dma_alignment = 4096;
    unsigned _disk_write_dma_alignment = 4096;
public:
    virtual ~file_impl() {}

    virtual future<size_t> write_dma(uint64_t pos, const void* buffer, size_t len, const io_priority_class& pc) = 0;
    virtual future<size_t> write_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc) = 0;
    virtual future<size_t> read_dma(uint64_t pos, void* buffer, size_t len, const io_priority_class& pc) = 0;
    virtual future<size_t> read_dma(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc) = 0;
    virtual future<> flush(void) = 0;
    virtual future<struct stat> stat(void) = 0;
    virtual future<> truncate(uint64_t length) = 0;
    virtual future<> discard(uint64_t offset, uint64_t length) = 0;
    virtual future<> allocate(uint64_t position, uint64_t length) = 0;
    virtual future<uint64_t> size(void) = 0;
    virtual future<> close() = 0;
    virtual std::unique_ptr<file_handle_impl> dup();
    virtual subscription<directory_entry> list_directory(std::function<future<> (directory_entry de)> next) = 0;
    virtual future<temporary_buffer<uint8_t>> dma_read_bulk(uint64_t offset, size_t range_size, const io_priority_class& pc) = 0;

    friend class reactor;
};

/// \endcond

/// A data file on persistent storage.
///
/// File objects represent uncached, unbuffered files.  As such great care
/// must be taken to cache data at the application layer; neither seastar
/// nor the OS will cache these file.
///
/// Data is transferred using direct memory access (DMA).  This imposes
/// restrictions on file offsets and data pointers.  The former must be aligned
/// on a 4096 byte boundary, while a 512 byte boundary suffices for the latter.
class file {
    shared_ptr<file_impl> _file_impl;
private:
    explicit file(int fd, file_open_options options);
public:
    /// Default constructor constructs an uninitialized file object.
    ///
    /// A default constructor is useful for the common practice of declaring
    /// a variable, and only assigning to it later. The uninitialized file
    /// must not be used, or undefined behavior will result (currently, a null
    /// pointer dereference).
    ///
    /// One can check whether a file object is in uninitialized state with
    /// \ref operator bool(); One can reset a file back to uninitialized state
    /// by assigning file() to it.
    file() : _file_impl(nullptr) {}

    file(shared_ptr<file_impl> impl)
            : _file_impl(std::move(impl)) {}

    /// Constructs a file object from a \ref file_handle obtained from another shard
    explicit file(file_handle&& handle);

    /// Checks whether the file object was initialized.
    ///
    /// \return false if the file object is uninitialized (default
    /// constructed), true if the file object refers to an actual file.
    explicit operator bool() const noexcept { return bool(_file_impl); }

    /// Copies a file object.  The new and old objects refer to the
    /// same underlying file.
    ///
    /// \param x file object to be copied
    file(const file& x) = default;
    /// Moves a file object.
    file(file&& x) noexcept : _file_impl(std::move(x._file_impl)) {}
    /// Assigns a file object.  After assignent, the destination and source refer
    /// to the same underlying file.
    ///
    /// \param x file object to assign to `this`.
    file& operator=(const file& x) noexcept = default;
    /// Moves assigns a file object.
    file& operator=(file&& x) noexcept = default;

    // O_DIRECT reading requires that buffer, offset, and read length, are
    // all aligned. Alignment of 4096 was necessary in the past, but no longer
    // is - 512 is usually enough; But we'll need to use BLKSSZGET ioctl to
    // be sure it is really enough on this filesystem. 4096 is always safe.
    // In addition, if we start reading in things outside page boundaries,
    // we will end up with various pages around, some of them with
    // overlapping ranges. Those would be very challenging to cache.

    /// Alignment requirement for file offsets (for reads)
    uint64_t disk_read_dma_alignment() const {
        return _file_impl->_disk_read_dma_alignment;
    }

    /// Alignment requirement for file offsets (for writes)
    uint64_t disk_write_dma_alignment() const {
        return _file_impl->_disk_write_dma_alignment;
    }

    /// Alignment requirement for data buffers
    uint64_t memory_dma_alignment() const {
        return _file_impl->_memory_dma_alignment;
    }


    /**
     * Perform a single DMA read operation.
     *
     * @param aligned_pos offset to begin reading at (should be aligned)
     * @param aligned_buffer output buffer (should be aligned)
     * @param aligned_len number of bytes to read (should be aligned)
     * @param pc the IO priority class under which to queue this operation
     *
     * Alignment is HW dependent but use 4KB alignment to be on the safe side as
     * explained above.
     *
     * @return number of bytes actually read
     * @throw exception in case of I/O error
     */
    template <typename CharType>
    future<size_t>
    dma_read(uint64_t aligned_pos, CharType* aligned_buffer, size_t aligned_len, const io_priority_class& pc = default_priority_class()) {
        return _file_impl->read_dma(aligned_pos, aligned_buffer, aligned_len, pc);
    }

    /**
     * Read the requested amount of bytes starting from the given offset.
     *
     * @param pos offset to begin reading from
     * @param len number of bytes to read
     * @param pc the IO priority class under which to queue this operation
     *
     * @return temporary buffer containing the requested data.
     * @throw exception in case of I/O error
     *
     * This function doesn't require any alignment for both "pos" and "len"
     *
     * @note size of the returned buffer may be smaller than "len" if EOF is
     *       reached of in case of I/O error.
     */
    template <typename CharType>
    future<temporary_buffer<CharType>> dma_read(uint64_t pos, size_t len, const io_priority_class& pc = default_priority_class()) {
        return dma_read_bulk<CharType>(pos, len, pc).then(
                [len] (temporary_buffer<CharType> buf) {
            if (len < buf.size()) {
                buf.trim(len);
            }

            return SEASTAR_COPY_ELISION(buf);
        });
    }

    /// Error thrown when attempting to read past end-of-file
    /// with \ref dma_read_exactly().
    class eof_error : public std::exception {};

    /**
     * Read the exact amount of bytes.
     *
     * @param pos offset in a file to begin reading from
     * @param len number of bytes to read
     * @param pc the IO priority class under which to queue this operation
     *
     * @return temporary buffer containing the read data
     * @throw end_of_file_error if EOF is reached, file_io_error or
     *        std::system_error in case of I/O error.
     */
    template <typename CharType>
    future<temporary_buffer<CharType>>
    dma_read_exactly(uint64_t pos, size_t len, const io_priority_class& pc = default_priority_class()) {
        return dma_read<CharType>(pos, len, pc).then(
                [pos, len] (auto buf) {
            if (buf.size() < len) {
                throw eof_error();
            }

            return SEASTAR_COPY_ELISION(buf);
        });
    }

    /// Performs a DMA read into the specified iovec.
    ///
    /// \param pos offset to read from.  Must be aligned to \ref dma_alignment.
    /// \param iov vector of address/size pairs to read into.  Addresses must be
    ///            aligned.
    /// \param pc the IO priority class under which to queue this operation
    ///
    /// \return a future representing the number of bytes actually read.  A short
    ///         read may happen due to end-of-file or an I/O error.
    future<size_t> dma_read(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc = default_priority_class()) {
        return _file_impl->read_dma(pos, std::move(iov), pc);
    }

    /// Performs a DMA write from the specified buffer.
    ///
    /// \param pos offset to write into.  Must be aligned to \ref dma_alignment.
    /// \param buffer aligned address of buffer to read from.  Buffer must exists
    ///               until the future is made ready.
    /// \param len number of bytes to write.  Must be aligned.
    /// \param pc the IO priority class under which to queue this operation
    ///
    /// \return a future representing the number of bytes actually written.  A short
    ///         write may happen due to an I/O error.
    template <typename CharType>
    future<size_t> dma_write(uint64_t pos, const CharType* buffer, size_t len, const io_priority_class& pc = default_priority_class()) {
        return _file_impl->write_dma(pos, buffer, len, pc);
    }

    /// Performs a DMA write to the specified iovec.
    ///
    /// \param pos offset to write into.  Must be aligned to \ref dma_alignment.
    /// \param iov vector of address/size pairs to write from.  Addresses must be
    ///            aligned.
    /// \param pc the IO priority class under which to queue this operation
    ///
    /// \return a future representing the number of bytes actually written.  A short
    ///         write may happen due to an I/O error.
    future<size_t> dma_write(uint64_t pos, std::vector<iovec> iov, const io_priority_class& pc = default_priority_class()) {
        return _file_impl->write_dma(pos, std::move(iov), pc);
    }

    /// Causes any previously written data to be made stable on persistent storage.
    ///
    /// Prior to a flush, written data may or may not survive a power failure.  After
    /// a flush, data is guaranteed to be on disk.
    future<> flush() {
        return _file_impl->flush();
    }

    /// Returns \c stat information about the file.
    future<struct stat> stat() {
        return _file_impl->stat();
    }

    /// Truncates the file to a specified length.
    future<> truncate(uint64_t length) {
        return _file_impl->truncate(length);
    }

    /// Preallocate disk blocks for a specified byte range.
    ///
    /// Requests the file system to allocate disk blocks to
    /// back the specified range (\c length bytes starting at
    /// \c position).  The range may be outside the current file
    /// size; the blocks can then be used when appending to the
    /// file.
    ///
    /// \param position beginning of the range at which to allocate
    ///                 blocks.
    /// \parm length length of range to allocate.
    /// \return future that becomes ready when the operation completes.
    future<> allocate(uint64_t position, uint64_t length) {
        return _file_impl->allocate(position, length);
    }

    /// Discard unneeded data from the file.
    ///
    /// The discard operation tells the file system that a range of offsets
    /// (which be aligned) is no longer needed and can be reused.
    future<> discard(uint64_t offset, uint64_t length) {
        return _file_impl->discard(offset, length);
    }

    /// Gets the file size.
    future<uint64_t> size() const {
        return _file_impl->size();
    }

    /// Closes the file.
    ///
    /// Flushes any pending operations and release any resources associated with
    /// the file (except for stable storage).
    ///
    /// \note
    /// to ensure file data reaches stable storage, you must call \ref flush()
    /// before calling \c close().
    future<> close() {
        return _file_impl->close();
    }

    /// Returns a directory listing, given that this file object is a directory.
    subscription<directory_entry> list_directory(std::function<future<> (directory_entry de)> next) {
        return _file_impl->list_directory(std::move(next));
    }

    /**
     * Read a data bulk containing the provided addresses range that starts at
     * the given offset and ends at either the address aligned to
     * dma_alignment (4KB) or at the file end.
     *
     * @param offset starting address of the range the read bulk should contain
     * @param range_size size of the addresses range
     * @param pc the IO priority class under which to queue this operation
     *
     * @return temporary buffer containing the read data bulk.
     * @throw system_error exception in case of I/O error or eof_error when
     *        "offset" is beyond EOF.
     */
    template <typename CharType>
    future<temporary_buffer<CharType>>
    dma_read_bulk(uint64_t offset, size_t range_size, const io_priority_class& pc = default_priority_class()) {
        return _file_impl->dma_read_bulk(offset, range_size, pc).then([] (temporary_buffer<uint8_t> t) {
            return temporary_buffer<CharType>(reinterpret_cast<CharType*>(t.get_write()), t.size(), t.release());
        });
    }

    /// \brief Creates a handle that can be transported across shards.
    ///
    /// Creates a handle that can be transported across shards, and then
    /// used to create a new shard-local \ref file object that refers to
    /// the same on-disk file.
    ///
    /// \note Use on read-only files.
    ///
    file_handle dup();

    template <typename CharType>
    struct read_state;
private:
    friend class reactor;
    friend class file_impl;
};

/// \brief A shard-transportable handle to a file
///
/// If you need to access a file (for reads only) across multiple shards,
/// you can use the file::dup() method to create a `file_handle`, transport
/// this file handle to another shard, and use the handle to create \ref file
/// object on that shard.  This is more efficient than calling open_file_dma()
/// again.
class file_handle {
    std::unique_ptr<file_handle_impl> _impl;
private:
    explicit file_handle(std::unique_ptr<file_handle_impl> impl) : _impl(std::move(impl)) {}
public:
    /// Copies a file handle object
    file_handle(const file_handle&);
    /// Moves a file handle object
    file_handle(file_handle&&) noexcept;
    /// Assigns a file handle object
    file_handle& operator=(const file_handle&);
    /// Move-assigns a file handle object
    file_handle& operator=(file_handle&&) noexcept;
    /// Converts the file handle object to a \ref file.
    file to_file() const &;
    /// Converts the file handle object to a \ref file.
    file to_file() &&;

    friend class file;
};

/// \cond internal

template <typename CharType>
struct file::read_state {
    typedef temporary_buffer<CharType> tmp_buf_type;

    read_state(uint64_t offset, uint64_t front, size_t to_read,
            size_t memory_alignment, size_t disk_alignment)
    : buf(tmp_buf_type::aligned(memory_alignment,
                                align_up(to_read, disk_alignment)))
    , _offset(offset)
    , _to_read(to_read)
    , _front(front) {}

    bool done() const {
        return eof || pos >= _to_read;
    }

    /**
     * Trim the buffer to the actual number of read bytes and cut the
     * bytes from offset 0 till "_front".
     *
     * @note this function has to be called only if we read bytes beyond
     *       "_front".
     */
    void trim_buf_before_ret() {
        if (have_good_bytes()) {
            buf.trim(pos);
            buf.trim_front(_front);
        } else {
            buf.trim(0);
        }
    }

    uint64_t cur_offset() const {
        return _offset + pos;
    }

    size_t left_space() const {
        return buf.size() - pos;
    }

    size_t left_to_read() const {
        // positive as long as (done() == false)
        return _to_read - pos;
    }

    void append_new_data(tmp_buf_type& new_data) {
        auto to_copy = std::min(left_space(), new_data.size());

        std::memcpy(buf.get_write() + pos, new_data.get(), to_copy);
        pos += to_copy;
    }

    bool have_good_bytes() const {
        return pos > _front;
    }

public:
    bool         eof      = false;
    tmp_buf_type buf;
    size_t       pos      = 0;
private:
    uint64_t     _offset;
    size_t       _to_read;
    uint64_t     _front;
};

/// \endcond

/// @}

}


namespace seastar {

// An unbounded FIFO queue of objects of type T.
//
// It provides operations to push items in one end of the queue, and pop them
// from the other end of the queue - both operations are guaranteed O(1)
// (not just amortized O(1)). The size() operation is also O(1).
// chunked_fifo also guarantees that the largest contiguous memory allocation
// it does is O(1). The total memory used is, of course, O(N).
//
// How does chunked_fifo differ from std::list<>, circular_buffer<> and
// std::deque()?
//
// std::list<> can also make all the above guarantees, but is inefficient -
// both at run speed (every operation requires an allocation), and in memory
// use. Much more efficient than std::list<> is our circular_buffer<>, which
// allocates a contiguous array to hold the items and only reallocates it,
// exponentially, when the queue grows. On one test of several different
// push/pop scenarios, circular_buffer<> was between 5 and 20 times faster
// than std::list, and also used considerably less memory.
// The problem with circular_buffer<> is that gives up on the last guarantee
// we made above: circular_buffer<> allocates all the items in one large
// contiguous allocation - that might not be possible when the memory is
// highly fragmented.
// std::deque<> aims to solve the contiguous allocation problem by allocating
// smaller chunks of the queue, and keeping a list of them in an array. This
// array is necessary to allow for O(1) random access to any element, a
// feature which we do not need; But this array is itself contiguous so
// std::deque<> attempts larger contiguous allocations the larger the queue
// gets: std::deque<>'s contiguous allocation is still O(N) and in fact
// exactly 1/64 of the size of circular_buffer<>'s contiguous allocation.
// So it's an improvement over circular_buffer<>, but not a full solution.
//
// chunked_fifo<> is such a solution: it also allocates the queue in fixed-
// size chunks (just like std::deque) but holds them in a linked list, not
// a contiguous array, so there are no large contiguous allocations.
//
// Unlike std::deque<> or circular_buffer<>, chunked_fifo only provides the
// operations needed by std::queue, i.e.,: empty(), size(), front(), back(),
// push_back() and pop_front(). For simplicity, we do *not* implement other
// possible operations, like inserting or deleting elements from the "wrong"
// side of the queue or from the middle, nor random-access to items in the
// middle of the queue. However, chunked_fifo does allow iterating over all
// of the queue's elements without popping them, a feature which std::queue
// is missing.
//
// Another feature of chunked_fifo which std::deque is missing is the ability
// to control the chunk size, as a template parameter. In std::deque the
// chunk size is undocumented and fixed - in gcc, it is always 512 bytes.
// chunked_fifo, on the other hand, makes the chunk size (in number of items
// instead of bytes) a template parameter; In situations where the queue is
// expected to become very long, using a larger chunk size might make sense
// because it will result in fewer allocations.
//
// chunked_fifo uses uninitialized storage for unoccupied elements, and thus
// uses move/copy constructors instead of move/copy assignments, which are
// less efficient.

template <typename T, size_t items_per_chunk = 128>
class chunked_fifo {
    static_assert((items_per_chunk & (items_per_chunk - 1)) == 0,
            "chunked_fifo chunk size must be power of two");
    union maybe_item {
        maybe_item() noexcept {}
        ~maybe_item() {}
        T data;
    };
    struct chunk {
        maybe_item items[items_per_chunk];
        struct chunk* next;
        // begin and end interpreted mod items_per_chunk
        unsigned begin;
        unsigned end;
    };
    // We pop from the chunk at _front_chunk. This chunk is then linked to
    // the following chunks via the "next" link. _back_chunk points to the
    // last chunk in this list, and it is where we push.
    chunk* _front_chunk = nullptr; // where we pop
    chunk* _back_chunk = nullptr; // where we push
    // We want an O(1) size but don't want to maintain a size() counter
    // because this will slow down every push and pop operation just for
    // the rare size() call. Instead, we just keep a count of chunks (which
    // doesn't change on every push or pop), from which we can calculate
    // size() when needed, and still be O(1).
    // This assumes the invariant that all middle chunks (except the front
    // and back) are always full.
    size_t _nchunks = 0;
    // A list of freed chunks, to support reserve() and to improve
    // performance of repeated push and pop, especially on an empty queue.
    // It is a performance/memory tradeoff how many freed chunks to keep
    // here (see save_free_chunks constant below).
    chunk* _free_chunks = nullptr;
    size_t _nfree_chunks = 0;
public:
    using value_type = T;
    using size_type = size_t;
    using reference = T&;
    using pointer = T*;
    using const_reference = const T&;
    using const_pointer = const T*;

private:
    template <typename U>
    class basic_iterator {
        friend class chunked_fifo;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = U;
        using pointer = U*;
        using reference = U&;

    protected:
        chunk* _chunk = nullptr;
        size_t _item_index = 0;

    protected:
        inline explicit basic_iterator(chunk* c);
        inline basic_iterator(chunk* c, size_t item_index);

    public:
        inline bool operator==(const basic_iterator& o) const;
        inline bool operator!=(const basic_iterator& o) const;
        inline pointer operator->() const;
        inline reference operator*() const;
        inline basic_iterator operator++(int);
        basic_iterator& operator++();
    };

public:
    class iterator : public basic_iterator<T> {
        using basic_iterator<T>::basic_iterator;
    public:
        iterator() = default;
    };
    class const_iterator : public basic_iterator<const T> {
        using basic_iterator<T>::basic_iterator;
    public:
        const_iterator() = default;
        inline const_iterator(iterator o);
    };

public:
    chunked_fifo() = default;
    chunked_fifo(chunked_fifo&& x) noexcept;
    chunked_fifo(const chunked_fifo& X) = delete;
    ~chunked_fifo();
    chunked_fifo& operator=(const chunked_fifo&) = delete;
    chunked_fifo& operator=(chunked_fifo&&) noexcept;
    inline void push_back(const T& data);
    inline void push_back(T&& data);
    T& back();
    const T& back() const;
    template <typename... A>
    inline void emplace_back(A&&... args);
    inline T& front() const noexcept;
    inline void pop_front() noexcept;
    inline bool empty() const noexcept;
    inline size_t size() const noexcept;
    void clear() noexcept;
    // reserve(n) ensures that at least (n - size()) further push() calls can
    // be served without needing new memory allocation.
    // Calling pop()s between these push()es is also allowed and does not
    // alter this guarantee.
    // Note that reserve() does not reduce the amount of memory already
    // reserved - use shrink_to_fit() for that.
    void reserve(size_t n);
    // shrink_to_fit() frees memory held, but unused, by the queue. Such
    // unused memory might exist after pops, or because of reserve().
    void shrink_to_fit();
    inline iterator begin();
    inline iterator end();
    inline const_iterator begin() const;
    inline const_iterator end() const;
    inline const_iterator cbegin() const;
    inline const_iterator cend() const;
private:
    void back_chunk_new();
    void front_chunk_delete() noexcept;
    inline void ensure_room_back();
    void undo_room_back();
    static inline size_t mask(size_t idx) noexcept;

};

template <typename T, size_t items_per_chunk>
template <typename U>
inline
chunked_fifo<T, items_per_chunk>::basic_iterator<U>::basic_iterator(chunk* c) : _chunk(c), _item_index(_chunk ? _chunk->begin : 0) {
}

template <typename T, size_t items_per_chunk>
template <typename U>
inline
chunked_fifo<T, items_per_chunk>::basic_iterator<U>::basic_iterator(chunk* c, size_t item_index) : _chunk(c), _item_index(item_index) {
}

template <typename T, size_t items_per_chunk>
template <typename U>
inline bool
chunked_fifo<T, items_per_chunk>::basic_iterator<U>::operator==(const basic_iterator& o) const {
    return _chunk == o._chunk && _item_index == o._item_index;
}

template <typename T, size_t items_per_chunk>
template <typename U>
inline bool
chunked_fifo<T, items_per_chunk>::basic_iterator<U>::operator!=(const basic_iterator& o) const {
    return !(*this == o);
}

template <typename T, size_t items_per_chunk>
template <typename U>
inline typename chunked_fifo<T, items_per_chunk>::template basic_iterator<U>::pointer
chunked_fifo<T, items_per_chunk>::basic_iterator<U>::operator->() const {
    return &_chunk->items[chunked_fifo::mask(_item_index)].data;
}

template <typename T, size_t items_per_chunk>
template <typename U>
inline typename chunked_fifo<T, items_per_chunk>::template basic_iterator<U>::reference
chunked_fifo<T, items_per_chunk>::basic_iterator<U>::operator*() const {
    return _chunk->items[chunked_fifo::mask(_item_index)].data;
}

template <typename T, size_t items_per_chunk>
template <typename U>
inline typename chunked_fifo<T, items_per_chunk>::template basic_iterator<U>
chunked_fifo<T, items_per_chunk>::basic_iterator<U>::operator++(int) {
    auto it = *this;
    ++(*this);
    return it;
}

template <typename T, size_t items_per_chunk>
template <typename U>
typename chunked_fifo<T, items_per_chunk>::template basic_iterator<U>&
chunked_fifo<T, items_per_chunk>::basic_iterator<U>::operator++() {
    ++_item_index;
    if (_item_index == _chunk->end) {
        _chunk = _chunk->next;
        _item_index = _chunk ? _chunk->begin : 0;
    }
    return *this;
}

template <typename T, size_t items_per_chunk>
inline
chunked_fifo<T, items_per_chunk>::const_iterator::const_iterator(chunked_fifo<T, items_per_chunk>::iterator o)
    : basic_iterator<const T>(o._chunk, o._item_index) {
}

template <typename T, size_t items_per_chunk>
inline
chunked_fifo<T, items_per_chunk>::chunked_fifo(chunked_fifo&& x) noexcept
        : _front_chunk(x._front_chunk)
        , _back_chunk(x._back_chunk)
        , _nchunks(x._nchunks)
        , _free_chunks(x._free_chunks)
        , _nfree_chunks(x._nfree_chunks) {
    x._front_chunk = nullptr;
    x._back_chunk = nullptr;
    x._nchunks = 0;
    x._free_chunks = nullptr;
    x._nfree_chunks = 0;
}

template <typename T, size_t items_per_chunk>
inline
chunked_fifo<T, items_per_chunk>&
chunked_fifo<T, items_per_chunk>::operator=(chunked_fifo&& x) noexcept {
    if (&x != this) {
        this->~chunked_fifo();
        new (this) chunked_fifo(std::move(x));
    }
    return *this;
}

template <typename T, size_t items_per_chunk>
inline size_t
chunked_fifo<T, items_per_chunk>::mask(size_t idx) noexcept {
    return idx & (items_per_chunk - 1);
}

template <typename T, size_t items_per_chunk>
inline bool
chunked_fifo<T, items_per_chunk>::empty() const noexcept {
    return _front_chunk == nullptr;
}

template <typename T, size_t items_per_chunk>
inline size_t
chunked_fifo<T, items_per_chunk>::size() const noexcept{
    if (_front_chunk == nullptr) {
        return 0;
    } else if (_back_chunk == _front_chunk) {
        // Single chunk.
        return _front_chunk->end - _front_chunk->begin;
    } else {
        return _front_chunk->end - _front_chunk->begin
                +_back_chunk->end - _back_chunk->begin
                + (_nchunks - 2) * items_per_chunk;
    }
}

template <typename T, size_t items_per_chunk>
void chunked_fifo<T, items_per_chunk>::clear() noexcept {
#if 1
    while (!empty()) {
        pop_front();
    }
#else
    // This is specialized code to free the contents of all the chunks and the
    // chunks themselves. but since destroying a very full queue is not an
    // important use case to optimize, the simple loop above is preferable.
    if (!_front_chunk) {
        // Empty, nothing to do
        return;
    }
    // Delete front chunk (partially filled)
    for (auto i = _front_chunk->begin; i != _front_chunk->end; ++i) {
        _front_chunk->items[mask(i)].data.~T();
    }
    chunk *p = _front_chunk->next;
    delete _front_chunk;
    // Delete all the middle chunks (all completely filled)
    if (p) {
        while (p != _back_chunk) {
            // These are full chunks
            chunk *nextp = p->next;
            for (auto i = 0; i != items_per_chunk; ++i) {
                // Note we delete out of order (we don't start with p->begin).
                // That should be fine..
                p->items[i].data.~T();
        }
            delete p;
            p = nextp;
        }
        // Finally delete back chunk (partially filled)
        for (auto i = _back_chunk->begin; i != _back_chunk->end; ++i) {
            _back_chunk->items[mask(i)].data.~T();
        }
        delete _back_chunk;
    }
    _front_chunk = nullptr;
    _back_chunk = nullptr;
    _nchunks = 0;
#endif
}

template <typename T, size_t items_per_chunk> void
chunked_fifo<T, items_per_chunk>::shrink_to_fit() {
    while (_free_chunks) {
        auto next = _free_chunks->next;
        delete _free_chunks;
        _free_chunks = next;
    }
    _nfree_chunks = 0;
}

template <typename T, size_t items_per_chunk>
chunked_fifo<T, items_per_chunk>::~chunked_fifo() {
    clear();
    shrink_to_fit();
}

template <typename T, size_t items_per_chunk>
void
chunked_fifo<T, items_per_chunk>::back_chunk_new() {
    chunk *old = _back_chunk;
    if (_free_chunks) {
        _back_chunk = _free_chunks;
        _free_chunks = _free_chunks->next;
        --_nfree_chunks;
    } else {
        _back_chunk = new chunk;
    }
    _back_chunk->next = nullptr;
    _back_chunk->begin = 0;
    _back_chunk->end = 0;
    if (old) {
        old->next = _back_chunk;
    }
    if (_front_chunk == nullptr) {
        _front_chunk = _back_chunk;
    }
    _nchunks++;
}


template <typename T, size_t items_per_chunk>
inline void
chunked_fifo<T, items_per_chunk>::ensure_room_back() {
    // If we don't have a back chunk or it's full, we need to create a new one
    if (_back_chunk == nullptr ||
            (_back_chunk->end - _back_chunk->begin) == items_per_chunk) {
        back_chunk_new();
    }
}

template <typename T, size_t items_per_chunk>
void
chunked_fifo<T, items_per_chunk>::undo_room_back() {
    // If we failed creating a new item after ensure_room_back() created a
    // new empty chunk, we must remove it, or empty() will be incorrect
    // (either immediately, if the fifo was empty, or when all the items are
    // popped, if it already had items).
    if (_back_chunk->begin == _back_chunk->end) {
        delete _back_chunk;
        --_nchunks;
        if (_nchunks == 0) {
            _back_chunk = nullptr;
            _front_chunk = nullptr;
        } else {
            // Because we don't usually pop from the back, we don't have a "prev"
            // pointer so we need to find the previous chunk the hard and slow
            // way. B
            chunk *old = _back_chunk;
            _back_chunk = _front_chunk;
            while (_back_chunk->next != old) {
                _back_chunk = _back_chunk->next;
            }
            _back_chunk->next = nullptr;
        }
    }

}

template <typename T, size_t items_per_chunk>
template <typename... Args>
inline void
chunked_fifo<T, items_per_chunk>::emplace_back(Args&&... args) {
    ensure_room_back();
    auto p = &_back_chunk->items[mask(_back_chunk->end)].data;
    try {
        new(p) T(std::forward<Args>(args)...);
    } catch(...) {
        undo_room_back();
        throw;
    }
    ++_back_chunk->end;
}

template <typename T, size_t items_per_chunk>
inline void
chunked_fifo<T, items_per_chunk>::push_back(const T& data) {
    ensure_room_back();
    auto p = &_back_chunk->items[mask(_back_chunk->end)].data;
    try {
        new(p) T(data);
    } catch(...) {
        undo_room_back();
        throw;
    }
    ++_back_chunk->end;
}

template <typename T, size_t items_per_chunk>
inline void
chunked_fifo<T, items_per_chunk>::push_back(T&& data) {
    ensure_room_back();
    auto p = &_back_chunk->items[mask(_back_chunk->end)].data;
    try {
        new(p) T(std::move(data));
    } catch(...) {
        undo_room_back();
        throw;
    }
    ++_back_chunk->end;
}

template <typename T, size_t items_per_chunk>
inline
T&
chunked_fifo<T, items_per_chunk>::back() {
    return _back_chunk->items[mask(_back_chunk->end - 1)].data;
}

template <typename T, size_t items_per_chunk>
inline
const T&
chunked_fifo<T, items_per_chunk>::back() const {
    return _back_chunk->items[mask(_back_chunk->end - 1)].data;
}

template <typename T, size_t items_per_chunk>
inline T&
chunked_fifo<T, items_per_chunk>::front() const noexcept {
    return _front_chunk->items[mask(_front_chunk->begin)].data;
}

template <typename T, size_t items_per_chunk>
inline void
chunked_fifo<T, items_per_chunk>::front_chunk_delete() noexcept {
    chunk *next = _front_chunk->next;
    // Certain use cases may need to repeatedly allocate and free a chunk -
    // an obvious example is an empty queue to which we push, and then pop,
    // repeatedly. Another example is pushing and popping to a non-empty queue
    // we push and pop at different chunks so we need to free and allocate a
    // chunk every items_per_chunk operations.
    // The solution is to keep a list of freed chunks instead of freeing them
    // immediately. There is a performance/memory tradeoff of how many freed
    // chunks to save: If we save them all, the queue can never shrink from
    // its maximum memory use (this is how circular_buffer behaves).
    // The ad-hoc choice made here is to limit the number of saved chunks to 1,
    // but this could easily be made a configuration option.
    static constexpr int save_free_chunks = 1;
    if (_nfree_chunks < save_free_chunks) {
        _front_chunk->next = _free_chunks;
        _free_chunks = _front_chunk;
        ++_nfree_chunks;
    } else {
        delete _front_chunk;
    }
    // If we only had one chunk, _back_chunk is gone too.
    if (_back_chunk == _front_chunk) {
        _back_chunk = nullptr;
    }
    _front_chunk = next;
    --_nchunks;
}

template <typename T, size_t items_per_chunk>
inline void
chunked_fifo<T, items_per_chunk>::pop_front() noexcept {
    front().~T();
    // If the front chunk has become empty, we need to free remove it and use
    // the next one.
    if (++_front_chunk->begin == _front_chunk->end) {
        front_chunk_delete();
    }
}

template <typename T, size_t items_per_chunk>
void chunked_fifo<T, items_per_chunk>::reserve(size_t n) {
    // reserve() guarantees that (n - size()) additional push()es will
    // succeed without reallocation:
    size_t need = n - size();
    // If we already have a back chunk, it might have room for some pushes
    // before filling up, so decrease "need":
    if (_back_chunk) {
        need -= items_per_chunk - (_back_chunk->end - _back_chunk->begin);
    }
    size_t needed_chunks = (need + items_per_chunk - 1) / items_per_chunk;
    // If we already have some freed chunks saved, we need to allocate fewer
    // additional chunks, or none at all
    if (needed_chunks <= _nfree_chunks) {
        return;
    }
    needed_chunks -= _nfree_chunks;
    while (needed_chunks--) {
        chunk *c = new chunk;
        c->next = _free_chunks;
        _free_chunks = c;
        ++_nfree_chunks;
    }
}

template <typename T, size_t items_per_chunk>
inline typename chunked_fifo<T, items_per_chunk>::iterator
chunked_fifo<T, items_per_chunk>::begin() {
    return iterator(_front_chunk);
}

template <typename T, size_t items_per_chunk>
inline typename chunked_fifo<T, items_per_chunk>::iterator
chunked_fifo<T, items_per_chunk>::end() {
    return iterator(nullptr);
}

template <typename T, size_t items_per_chunk>
inline typename chunked_fifo<T, items_per_chunk>::const_iterator
chunked_fifo<T, items_per_chunk>::begin() const {
    return const_iterator(_front_chunk);
}

template <typename T, size_t items_per_chunk>
inline typename chunked_fifo<T, items_per_chunk>::const_iterator
chunked_fifo<T, items_per_chunk>::end() const {
    return const_iterator(nullptr);
}

template <typename T, size_t items_per_chunk>
inline typename chunked_fifo<T, items_per_chunk>::const_iterator
chunked_fifo<T, items_per_chunk>::cbegin() const {
    return const_iterator(_front_chunk);
}

template <typename T, size_t items_per_chunk>
inline typename chunked_fifo<T, items_per_chunk>::const_iterator
chunked_fifo<T, items_per_chunk>::cend() const {
    return const_iterator(nullptr);
}

}



namespace seastar {

//
// Forward declarations.
//

class lowres_clock;
class lowres_system_clock;

/// \cond internal

class lowres_clock_impl final {
public:
    using base_steady_clock = std::chrono::steady_clock;
    using base_system_clock = std::chrono::system_clock;

    // The clocks' resolutions are 10 ms. However, to make it is easier to do calculations with
    // `std::chrono::milliseconds`, we make the clock period 1 ms instead of 10 ms.
    using period = std::ratio<1, 1000>;

    using steady_rep = base_steady_clock::rep;
    using steady_duration = std::chrono::duration<steady_rep, period>;
    using steady_time_point = std::chrono::time_point<lowres_clock, steady_duration>;

    using system_rep = base_system_clock::rep;
    using system_duration = std::chrono::duration<system_rep, period>;
    using system_time_point = std::chrono::time_point<lowres_system_clock, system_duration>;

    static steady_time_point steady_now() {
        auto const nr = counters::_steady_now.load(std::memory_order_relaxed);
        return steady_time_point(steady_duration(nr));
    }

    static system_time_point system_now() {
        auto const nr = counters::_system_now.load(std::memory_order_relaxed);
        return system_time_point(system_duration(nr));
    }

    // For construction.
    friend class smp;
private:
    // Both counters are updated by cpu0 and read by other cpus. Place them on their own cache line to avoid false
    // sharing.
    struct alignas(seastar::cache_line_size) counters final {
        static std::atomic<steady_rep> _steady_now;
        static std::atomic<system_rep> _system_now;
    };

    // The timer expires every 10 ms.
    static constexpr std::chrono::milliseconds _granularity{10};

    // High-resolution timer to drive these low-resolution clocks.
    timer<> _timer{};

    static void update();

    // Private to ensure that static variables are only initialized once.
    lowres_clock_impl();
};

/// \endcond

//
/// \brief Low-resolution and efficient steady clock.
///
/// This is a monotonic clock with a granularity of 10 ms. Time points from this clock do not correspond to system
/// time.
///
/// The primary benefit of this clock is that invoking \c now() is inexpensive compared to
/// \c std::chrono::steady_clock::now().
///
/// \see \c lowres_system_clock for a low-resolution clock which produces time points corresponding to system time.
///
class lowres_clock final {
public:
    using rep = lowres_clock_impl::steady_rep;
    using period = lowres_clock_impl::period;
    using duration = lowres_clock_impl::steady_duration;
    using time_point = lowres_clock_impl::steady_time_point;

    static constexpr bool is_steady = true;

    ///
    /// \note Outside of a Seastar application, the result is undefined.
    ///
    static time_point now() {
        return lowres_clock_impl::steady_now();
    }
};

///
/// \brief Low-resolution and efficient system clock.
///
/// This clock has the same granularity as \c lowres_clock, but it is not required to be monotonic and its time points
/// correspond to system time.
///
/// The primary benefit of this clock is that invoking \c now() is inexpensive compared to
/// \c std::chrono::system_clock::now().
///
class lowres_system_clock final {
public:
    using rep = lowres_clock_impl::system_rep;
    using period = lowres_clock_impl::period;
    using duration = lowres_clock_impl::system_duration;
    using time_point = lowres_clock_impl::system_time_point;

    static constexpr bool is_steady = lowres_clock_impl::base_system_clock::is_steady;

    ///
    /// \note Outside of a Seastar application, the result is undefined.
    ///
    static time_point now() {
        return lowres_clock_impl::system_now();
    }

    static std::time_t to_time_t(time_point t) {
        return std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count();
    }

    static time_point from_time_t(std::time_t t) {
        return time_point(std::chrono::duration_cast<duration>(std::chrono::seconds(t)));
    }
};

extern template class timer<lowres_clock>;

}


namespace seastar {

template<typename T>
struct dummy_expiry {
    void operator()(T&) noexcept {};
};

template<typename... T>
struct promise_expiry {
    void operator()(promise<T...>& pr) noexcept {
        pr.set_exception(std::make_exception_ptr(timed_out_error()));
    };
};

/// Container for elements with support for expiration of entries.
///
/// OnExpiry is a functor which will be called with a reference to T right before it expires.
/// T is removed and destroyed from the container immediately after OnExpiry returns.
/// OnExpiry callback must not modify the container, it can only modify its argument.
///
/// The container can only be moved before any elements are pushed.
///
template <typename T, typename OnExpiry = dummy_expiry<T>, typename Clock = lowres_clock>
class expiring_fifo {
public:
    using clock = Clock;
    using time_point = typename Clock::time_point;
private:
    struct entry {
        compat::optional<T> payload; // disengaged means that it's expired
        timer<Clock> tr;
        entry(T&& payload_) : payload(std::move(payload_)) {}
        entry(const T& payload_) : payload(payload_) {}
        entry(T payload_, expiring_fifo& ef, time_point timeout)
                : payload(std::move(payload_))
                , tr([this, &ef] {
                    ef._on_expiry(*payload);
                    payload = compat::nullopt;
                    --ef._size;
                    ef.drop_expired_front();
                })
        {
            tr.arm(timeout);
        }
        entry(entry&& x) = delete;
        entry(const entry& x) = delete;
    };

    // If engaged, represents the first element.
    // This is to avoid large allocations done by chunked_fifo for single-element cases.
    // expiring_fifo is used to implement wait lists in synchronization primitives
    // and in some uses it's common to have at most one waiter.
    std::unique_ptr<entry> _front;

    // There is an invariant that the front element is never expired.
    chunked_fifo<entry> _list;
    OnExpiry _on_expiry;
    size_t _size = 0;

    // Ensures that front() is not expired by dropping expired elements from the front.
    void drop_expired_front() {
        while (!_list.empty() && !_list.front().payload) {
            _list.pop_front();
        }
        if (_front && !_front->payload) {
            _front.reset();
        }
    }
public:
    expiring_fifo() = default;
    expiring_fifo(OnExpiry on_expiry) : _on_expiry(std::move(on_expiry)) {}

    expiring_fifo(expiring_fifo&& o) noexcept
            : expiring_fifo() {
        // entry objects hold a reference to this so non-empty containers cannot be moved.
        assert(o._size == 0);
    }

    /// Checks if container contains any elements
    ///
    /// \note Inside OnExpiry callback, the expired element is still contained.
    ///
    /// \return true if and only if there are any elements contained.
    bool empty() const {
        return _size == 0;
    }

    /// Equivalent to !empty()
    explicit operator bool() const {
        return !empty();
    }

    /// Returns a reference to the element in the front.
    /// Valid only when !empty().
    T& front() {
        if (_front) {
            return *_front->payload;
        }
        return *_list.front().payload;
    }

    /// Returns a reference to the element in the front.
    /// Valid only when !empty().
    const T& front() const {
        if (_front) {
            return *_front->payload;
        }
        return *_list.front().payload;
    }

    /// Returns the number of elements contained.
    ///
    /// \note Expired elements are not contained. Expiring element is still contained when OnExpiry is called.
    size_t size() const {
        return _size;
    }

    /// Reserves storage in the container for at least 'size' elements.
    /// Note that expired elements may also take space when they are not in the front of the queue.
    ///
    /// Doesn't give any guarantees about exception safety of subsequent push_back().
    void reserve(size_t size) {
        return _list.reserve(size);
    }

    /// Adds element to the back of the queue.
    /// The element will never expire.
    void push_back(const T& payload) {
        if (_size == 0) {
            _front = std::make_unique<entry>(payload);
        } else {
            _list.emplace_back(payload);
        }
        ++_size;
    }

    /// Adds element to the back of the queue.
    /// The element will never expire.
    void push_back(T&& payload) {
        if (_size == 0) {
            _front = std::make_unique<entry>(std::move(payload));
        } else {
            _list.emplace_back(std::move(payload));
        }
        ++_size;
    }

    /// Adds element to the back of the queue.
    /// The element will expire when timeout is reached, unless it is time_point::max(), in which
    /// case it never expires.
    void push_back(T payload, time_point timeout) {
        if (timeout == time_point::max()) {
            push_back(std::move(payload));
            return;
        }
        if (_size == 0) {
            _front = std::make_unique<entry>(std::move(payload), *this, timeout);
        } else {
            _list.emplace_back(std::move(payload), *this, timeout);
        }
        ++_size;
    }

    /// Removes the element at the front.
    /// Can be called only if !empty().
    void pop_front() {
        if (_front) {
            _front.reset();
        } else {
            _list.pop_front();
        }
        --_size;
        drop_expired_front();
    }
};

}

namespace seastar {

/// \addtogroup fiber-module
/// @{

/// Exception thrown when a semaphore is broken by
/// \ref semaphore::broken().
class broken_semaphore : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Semaphore broken";
    }
};

/// Exception thrown when a semaphore wait operation
/// times out.
///
/// \see semaphore::wait(typename timer<>::duration timeout, size_t nr)
class semaphore_timed_out : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Semaphore timedout";
    }
};

/// Exception Factory for standard semaphore
///
/// constructs standard semaphore exceptions
/// \see semaphore_timed_out and broken_semaphore
struct semaphore_default_exception_factory {
    static semaphore_timed_out timeout() {
        return semaphore_timed_out();
    }
    static broken_semaphore broken() {
        return broken_semaphore();
    }
};

class named_semaphore_timed_out : public semaphore_timed_out {
    sstring _msg;
public:
    named_semaphore_timed_out(compat::string_view msg);
    virtual const char* what() const noexcept {
        return _msg.c_str();
    }
};

class broken_named_semaphore : public broken_semaphore {
    sstring _msg;
public:
    broken_named_semaphore(compat::string_view msg);
    virtual const char* what() const noexcept {
        return _msg.c_str();
    }
};

// A factory of semaphore exceptions that contain additional context: the semaphore name
// auto sem = named_semaphore(0, named_semaphore_exception_factory{"file_opening_limit_semaphore"});
struct named_semaphore_exception_factory {
    sstring name;
    named_semaphore_timed_out timeout() {
        return named_semaphore_timed_out(name);
    }
    broken_named_semaphore broken() {
        return broken_named_semaphore(name);
    }
};

/// \brief Counted resource guard.
///
/// This is a standard computer science semaphore, adapted
/// for futures.  You can deposit units into a counter,
/// or take them away.  Taking units from the counter may wait
/// if not enough units are available.
///
/// To support exceptional conditions, a \ref broken() method
/// is provided, which causes all current waiters to stop waiting,
/// with an exceptional future returned.  This allows causing all
/// fibers that are blocked on a semaphore to continue.  This is
/// similar to POSIX's `pthread_cancel()`, with \ref wait() acting
/// as a cancellation point.
///
/// \tparam ExceptionFactory template parameter allows modifying a semaphore to throw
/// customized exceptions on timeout/broken(). It has to provide two functions
/// ExceptionFactory::timeout() and ExceptionFactory::broken() which return corresponding
/// exception object.
template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
class basic_semaphore : private ExceptionFactory {
public:
    using duration = typename timer<Clock>::duration;
    using clock = typename timer<Clock>::clock;
    using time_point = typename timer<Clock>::time_point;
    using exception_factory = ExceptionFactory;
private:
    ssize_t _count;
    std::exception_ptr _ex;
    struct entry {
        promise<> pr;
        size_t nr;
        entry(promise<>&& pr_, size_t nr_) : pr(std::move(pr_)), nr(nr_) {}
    };
    struct expiry_handler : public exception_factory {
        expiry_handler() = default;
        expiry_handler(exception_factory&& f) : exception_factory(std::move(f)) { }
        void operator()(entry& e) noexcept {
            e.pr.set_exception(exception_factory::timeout());
        }
    };
    expiring_fifo<entry, expiry_handler, clock> _wait_list;
    bool has_available_units(size_t nr) const {
        return _count >= 0 && (static_cast<size_t>(_count) >= nr);
    }
    bool may_proceed(size_t nr) const {
        return has_available_units(nr) && _wait_list.empty();
    }
public:
    /// Returns the maximum number of units the semaphore counter can hold
    static constexpr size_t max_counter() {
        return std::numeric_limits<decltype(_count)>::max();
    }

    /// Constructs a semaphore object with a specific number of units
    /// in its internal counter. E.g., starting it at 1 is suitable for use as
    /// an unlocked mutex.
    ///
    /// \param count number of initial units present in the counter.
    basic_semaphore(size_t count) : _count(count) {}
    basic_semaphore(size_t count, exception_factory&& factory) : exception_factory(factory), _count(count), _wait_list(expiry_handler(std::move(factory))) {}
    /// Waits until at least a specific number of units are available in the
    /// counter, and reduces the counter by that amount of units.
    ///
    /// \note Waits are serviced in FIFO order, though if several are awakened
    ///       at once, they may be reordered by the scheduler.
    ///
    /// \param nr Amount of units to wait for (default 1).
    /// \return a future that becomes ready when sufficient units are available
    ///         to satisfy the request.  If the semaphore was \ref broken(), may
    ///         contain an exception.
    future<> wait(size_t nr = 1) {
        return wait(time_point::max(), nr);
    }
    /// Waits until at least a specific number of units are available in the
    /// counter, and reduces the counter by that amount of units.  If the request
    /// cannot be satisfied in time, the request is aborted.
    ///
    /// \note Waits are serviced in FIFO order, though if several are awakened
    ///       at once, they may be reordered by the scheduler.
    ///
    /// \param timeout expiration time.
    /// \param nr Amount of units to wait for (default 1).
    /// \return a future that becomes ready when sufficient units are available
    ///         to satisfy the request.  On timeout, the future contains a
    ///         \ref semaphore_timed_out exception.  If the semaphore was
    ///         \ref broken(), may contain an exception.
    future<> wait(time_point timeout, size_t nr = 1) {
        if (may_proceed(nr)) {
            _count -= nr;
            return make_ready_future<>();
        }
        if (_ex) {
            return make_exception_future(_ex);
        }
        promise<> pr;
        auto fut = pr.get_future();
        _wait_list.push_back(entry(std::move(pr), nr), timeout);
        return fut;
    }

    /// Waits until at least a specific number of units are available in the
    /// counter, and reduces the counter by that amount of units.  If the request
    /// cannot be satisfied in time, the request is aborted.
    ///
    /// \note Waits are serviced in FIFO order, though if several are awakened
    ///       at once, they may be reordered by the scheduler.
    ///
    /// \param timeout how long to wait.
    /// \param nr Amount of units to wait for (default 1).
    /// \return a future that becomes ready when sufficient units are available
    ///         to satisfy the request.  On timeout, the future contains a
    ///         \ref semaphore_timed_out exception.  If the semaphore was
    ///         \ref broken(), may contain an exception.
    future<> wait(duration timeout, size_t nr = 1) {
        return wait(clock::now() + timeout, nr);
    }
    /// Deposits a specified number of units into the counter.
    ///
    /// The counter is incremented by the specified number of units.
    /// If the new counter value is sufficient to satisfy the request
    /// of one or more waiters, their futures (in FIFO order) become
    /// ready, and the value of the counter is reduced according to
    /// the amount requested.
    ///
    /// \param nr Number of units to deposit (default 1).
    void signal(size_t nr = 1) {
        if (_ex) {
            return;
        }
        _count += nr;
        while (!_wait_list.empty() && has_available_units(_wait_list.front().nr)) {
            auto& x = _wait_list.front();
            _count -= x.nr;
            x.pr.set_value();
            _wait_list.pop_front();
        }
    }

    /// Consume the specific number of units without blocking
    //
    /// Consume the specific number of units now, regardless of how many units are available
    /// in the counter, and reduces the counter by that amount of units. This operation may
    /// cause the counter to go negative.
    ///
    /// \param nr Amount of units to consume (default 1).
    void consume(size_t nr = 1) {
        if (_ex) {
            return;
        }
        _count -= nr;
    }

    /// Attempts to reduce the counter value by a specified number of units.
    ///
    /// If sufficient units are available in the counter, and if no
    /// other fiber is waiting, then the counter is reduced.  Otherwise,
    /// nothing happens.  This is useful for "opportunistic" waits where
    /// useful work can happen if the counter happens to be ready, but
    /// when it is not worthwhile to wait.
    ///
    /// \param nr number of units to reduce the counter by (default 1).
    /// \return `true` if the counter had sufficient units, and was decremented.
    bool try_wait(size_t nr = 1) {
        if (may_proceed(nr)) {
            _count -= nr;
            return true;
        } else {
            return false;
        }
    }
    /// Returns the number of units available in the counter.
    ///
    /// Does not take into account any waiters.
    size_t current() const { return std::max(_count, ssize_t(0)); }

    /// Returns the number of available units.
    ///
    /// Takes into account units consumed using \ref consume() and therefore
    /// may return a negative value.
    ssize_t available_units() const { return _count; }

    /// Returns the current number of waiters
    size_t waiters() const { return _wait_list.size(); }

    /// Signal to waiters that an error occurred.  \ref wait() will see
    /// an exceptional future<> containing a \ref broken_semaphore exception.
    /// The future is made available immediately.
    void broken() { broken(std::make_exception_ptr(exception_factory::broken())); }

    /// Signal to waiters that an error occurred.  \ref wait() will see
    /// an exceptional future<> containing the provided exception parameter.
    /// The future is made available immediately.
    template <typename Exception>
    void broken(const Exception& ex) {
        broken(std::make_exception_ptr(ex));
    }

    /// Signal to waiters that an error occurred.  \ref wait() will see
    /// an exceptional future<> containing the provided exception parameter.
    /// The future is made available immediately.
    void broken(std::exception_ptr ex);

    /// Reserve memory for waiters so that wait() will not throw.
    void ensure_space_for_waiters(size_t n) {
        _wait_list.reserve(n);
    }
};

template<typename ExceptionFactory, typename Clock>
inline
void
basic_semaphore<ExceptionFactory, Clock>::broken(std::exception_ptr xp) {
    _ex = xp;
    _count = 0;
    while (!_wait_list.empty()) {
        auto& x = _wait_list.front();
        x.pr.set_exception(xp);
        _wait_list.pop_front();
    }
}

template<typename ExceptionFactory = semaphore_default_exception_factory, typename Clock = typename timer<>::clock>
class semaphore_units {
    basic_semaphore<ExceptionFactory, Clock>* _sem;
    size_t _n;

    semaphore_units(basic_semaphore<ExceptionFactory, Clock>* sem, size_t n) noexcept : _sem(sem), _n(n) {}
public:
    semaphore_units() noexcept : semaphore_units(nullptr, 0) {}
    semaphore_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t n) noexcept : semaphore_units(&sem, n) {}
    semaphore_units(semaphore_units&& o) noexcept : _sem(o._sem), _n(std::exchange(o._n, 0)) {
    }
    semaphore_units& operator=(semaphore_units&& o) noexcept {
        _sem = o._sem;
        _n = std::exchange(o._n, 0);
        return *this;
    }
    semaphore_units(const semaphore_units&) = delete;
    ~semaphore_units() noexcept {
        if (_n) {
            _sem->signal(_n);
        }
    }
    /// Releases ownership of the units. The semaphore will not be signalled.
    ///
    /// \return the number of units held
    size_t release() {
        return std::exchange(_n, 0);
    }
    /// Splits this instance into a \ref semaphore_units object holding the specified amount of units.
    /// This object will continue holding the remaining units.
    ///
    /// noexcept if \ref units <= \ref _n
    ///
    /// \return semaphore_units holding the specified number of units
    semaphore_units split(size_t units) {
        if (units > _n) {
            throw std::invalid_argument("Cannot take more units than those protected by the semaphore");
        }
        _n -= units;
        return semaphore_units(_sem, units);
    }
    /// The inverse of split(), in which the units held by the specified \ref semaphore_units
    /// object are merged into the current one. The function assumes (and asserts) that both
    /// are associated with the same \ref semaphore.
    ///
    /// \return the updated semaphore_units object
    void adopt(semaphore_units&& other) noexcept {
        assert(other._sem == _sem);
        _n += other.release();
    }
};

/// \brief Take units from semaphore temporarily
///
/// Takes units from the semaphore and returns them when the \ref semaphore_units object goes out of scope.
/// This provides a safe way to temporarily take units from a semaphore and ensure
/// that they are eventually returned under all circumstances (exceptions, premature scope exits, etc).
///
/// Unlike with_semaphore(), the scope of unit holding is not limited to the scope of a single async lambda.
///
/// \param sem The semaphore to take units from
/// \param units  Number of units to take
/// \return a \ref future<> holding \ref semaphore_units object. When the object goes out of scope
///         the units are returned to the semaphore.
///
/// \note The caller must guarantee that \c sem is valid as long as
///      \ref seaphore_units object is alive.
///
/// \related semaphore
template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
future<semaphore_units<ExceptionFactory, Clock>>
get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units) {
    return sem.wait(units).then([&sem, units] {
        return semaphore_units<ExceptionFactory, Clock>{ sem, units };
    });
}

/// \brief Take units from semaphore temporarily with time bound on wait
///
/// Like \ref get_units(basic_semaphore<ExceptionFactory>&, size_t) but when
/// timeout is reached before units are granted throws semaphore_timed_out exception.
///
/// \param sem The semaphore to take units from
/// \param units  Number of units to take
/// \return a \ref future<> holding \ref semaphore_units object. When the object goes out of scope
///         the units are returned to the semaphore.
///
/// \note The caller must guarantee that \c sem is valid as long as
///      \ref seaphore_units object is alive.
///
/// \related semaphore
template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
future<semaphore_units<ExceptionFactory, Clock>>
get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, typename basic_semaphore<ExceptionFactory, Clock>::time_point timeout) {
    return sem.wait(timeout, units).then([&sem, units] {
        return semaphore_units<ExceptionFactory, Clock>{ sem, units };
    });
}

/// \brief Take units from semaphore temporarily with time bound on wait
///
/// Like \ref get_units(basic_semaphore<ExceptionFactory>&, size_t, basic_semaphore<ExceptionFactory>::time_point) but
/// allow the timeout to be specified as a duration.
///
/// \param sem The semaphore to take units from
/// \param units  Number of units to take
/// \param timeout a duration specifying when to timeout the current request
/// \return a \ref future<> holding \ref semaphore_units object. When the object goes out of scope
///         the units are returned to the semaphore.
///
/// \note The caller must guarantee that \c sem is valid as long as
///      \ref seaphore_units object is alive.
///
/// \related semaphore
template<typename ExceptionFactory, typename Clock>
future<semaphore_units<ExceptionFactory, Clock>>
get_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, typename basic_semaphore<ExceptionFactory, Clock>::duration timeout) {
    return sem.wait(timeout, units).then([&sem, units] {
        return semaphore_units<ExceptionFactory, Clock>{ sem, units };
    });
}


/// \brief Consume units from semaphore temporarily
///
/// Consume units from the semaphore and returns them when the \ref semaphore_units object goes out of scope.
/// This provides a safe way to temporarily take units from a semaphore and ensure
/// that they are eventually returned under all circumstances (exceptions, premature scope exits, etc).
///
/// Unlike get_units(), this calls the non-blocking consume() API.
///
/// Unlike with_semaphore(), the scope of unit holding is not limited to the scope of a single async lambda.
///
/// \param sem The semaphore to take units from
/// \param units  Number of units to consume
template<typename ExceptionFactory, typename Clock = typename timer<>::clock>
semaphore_units<ExceptionFactory, Clock>
consume_units(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units) {
    sem.consume(units);
    return semaphore_units<ExceptionFactory, Clock>{ sem, units };
}

/// \brief Runs a function protected by a semaphore
///
/// Acquires a \ref semaphore, runs a function, and releases
/// the semaphore, returning the the return value of the function,
/// as a \ref future.
///
/// \param sem The semaphore to be held while the \c func is
///            running.
/// \param units  Number of units to acquire from \c sem (as
///               with semaphore::wait())
/// \param func   The function to run; signature \c void() or
///               \c future<>().
/// \return a \ref future<> holding the function's return value
///         or exception thrown; or a \ref future<> containing
///         an exception from one of the semaphore::broken()
///         variants.
///
/// \note The caller must guarantee that \c sem is valid until
///       the future returned by with_semaphore() resolves.
///
/// \related semaphore
template <typename ExceptionFactory, typename Func, typename Clock = typename timer<>::clock>
inline
futurize_t<std::result_of_t<Func()>>
with_semaphore(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, Func&& func) {
    return get_units(sem, units).then([func = std::forward<Func>(func)] (auto units) mutable {
        return futurize_apply(std::forward<Func>(func)).finally([units = std::move(units)] {});
    });
}

/// \brief Runs a function protected by a semaphore with time bound on wait
///
/// If possible, acquires a \ref semaphore, runs a function, and releases
/// the semaphore, returning the the return value of the function,
/// as a \ref future.
///
/// If the semaphore can't be acquired within the specified timeout, returns
/// a semaphore_timed_out exception
///
/// \param sem The semaphore to be held while the \c func is
///            running.
/// \param units  Number of units to acquire from \c sem (as
///               with semaphore::wait())
/// \param timeout a duration specifying when to timeout the current request
/// \param func   The function to run; signature \c void() or
///               \c future<>().
/// \return a \ref future<> holding the function's return value
///         or exception thrown; or a \ref future<> containing
///         an exception from one of the semaphore::broken()
///         variants.
///
/// \note The caller must guarantee that \c sem is valid until
///       the future returned by with_semaphore() resolves.
///
/// \related semaphore
template <typename ExceptionFactory, typename Clock, typename Func>
inline
futurize_t<std::result_of_t<Func()>>
with_semaphore(basic_semaphore<ExceptionFactory, Clock>& sem, size_t units, typename basic_semaphore<ExceptionFactory, Clock>::duration timeout, Func&& func) {
    return get_units(sem, units, timeout).then([func = std::forward<Func>(func)] (auto units) mutable {
        return futurize_apply(std::forward<Func>(func)).finally([units = std::move(units)] {});
    });
}

/// default basic_semaphore specialization that throws semaphore specific exceptions
/// on error conditions.
using semaphore = basic_semaphore<semaphore_default_exception_factory>;
using named_semaphore = basic_semaphore<named_semaphore_exception_factory>;

/// @}

}

namespace seastar {

template <typename T>
class enum_hash {
    static_assert(std::is_enum<T>::value, "must be an enum");
public:
    std::size_t operator()(const T& e) const {
        using utype = typename std::underlying_type<T>::type;
        return std::hash<utype>()(static_cast<utype>(e));
    }
};

}


#include <vector>
#include <sched.h>
#include <boost/any.hpp>
#include <unordered_map>

namespace seastar {

cpu_set_t cpuid_to_cpuset(unsigned cpuid);

namespace resource {

using compat::optional;

using cpuset = std::set<unsigned>;

struct configuration {
    optional<size_t> total_memory;
    optional<size_t> reserve_memory;  // if total_memory not specified
    optional<size_t> cpus;
    optional<cpuset> cpu_set;
    std::unordered_map<dev_t, unsigned> num_io_queues;
};

struct memory {
    size_t bytes;
    unsigned nodeid;

};

// Since this is static information, we will keep a copy at each CPU.
// This will allow us to easily find who is the IO coordinator for a given
// node without a trip to a remote CPU.
struct io_queue_topology {
    std::vector<unsigned> shard_to_coordinator;
    std::vector<unsigned> coordinators;
    std::vector<unsigned> coordinator_to_idx;
    std::vector<bool> coordinator_to_idx_valid; // for validity asserts
};

struct cpu {
    unsigned cpu_id;
    std::vector<memory> mem;
};

struct resources {
    std::vector<cpu> cpus;
    std::unordered_map<dev_t, io_queue_topology> ioq_topology;
};

resources allocate(configuration c);
unsigned nr_processing_units();
}

// We need a wrapper class, because boost::program_options wants validate()
// (below) to be in the same namespace as the type it is validating.
struct cpuset_bpo_wrapper {
    resource::cpuset value;
};

// Overload for boost program options parsing/validation
extern
void validate(boost::any& v,
              const std::vector<std::string>& values,
              cpuset_bpo_wrapper* target_type, int);

}
#include <new>
#include <vector>

namespace seastar {

/// \defgroup memory-module Memory management
///
/// Functions and classes for managing memory.
///
/// Memory management in seastar consists of the following:
///
///   - Low-level memory management in the \ref memory namespace.
///   - Various smart pointers: \ref shared_ptr, \ref lw_shared_ptr,
///     and \ref foreign_ptr.
///   - zero-copy support: \ref temporary_buffer and \ref deleter.

/// Low-level memory management support
///
/// The \c memory namespace provides functions and classes for interfacing
/// with the seastar memory allocator.
///
/// The seastar memory allocator splits system memory into a pool per
/// logical core (lcore).  Memory allocated one an lcore should be freed
/// on the same lcore; failing to do so carries a severe performance
/// penalty.  It is possible to share memory with another core, but this
/// should be limited to avoid cache coherency traffic.
namespace memory {

/// \cond internal

#ifdef SEASTAR_OVERRIDE_ALLOCATOR_PAGE_SIZE
#define SEASTAR_INTERNAL_ALLOCATOR_PAGE_SIZE (SEASTAR_OVERRIDE_ALLOCATOR_PAGE_SIZE)
#else
#define SEASTAR_INTERNAL_ALLOCATOR_PAGE_SIZE 4096
#endif

static constexpr size_t page_size = SEASTAR_INTERNAL_ALLOCATOR_PAGE_SIZE;
static constexpr size_t page_bits = log2ceil(page_size);
static constexpr size_t huge_page_size =
#if defined(__x86_64__) || defined(__i386__) || defined(__s390x__) || defined(__zarch__)
    1 << 21; // 2M
#elif defined(__aarch64__)
    1 << 21; // 2M
#elif defined(__PPC__)
    1 << 24; // 16M
#else
#error "Huge page size is not defined for this architecture"
#endif

void configure(std::vector<resource::memory> m, bool mbind,
        compat::optional<std::string> hugetlbfs_path = {});

void enable_abort_on_allocation_failure();

class disable_abort_on_alloc_failure_temporarily {
public:
    disable_abort_on_alloc_failure_temporarily();
    ~disable_abort_on_alloc_failure_temporarily() noexcept;
};

enum class reclaiming_result {
    reclaimed_nothing,
    reclaimed_something
};

// Determines when reclaimer can be invoked
enum class reclaimer_scope {
    //
    // Reclaimer is only invoked in its own fiber. That fiber will be
    // given higher priority than regular application fibers.
    //
    async,

    //
    // Reclaimer may be invoked synchronously with allocation.
    // It may also be invoked in async scope.
    //
    // Reclaimer may invoke allocation, though it is discouraged because
    // the system may be low on memory and such allocations may fail.
    // Reclaimers which allocate should be prepared for re-entry.
    //
    sync
};

class reclaimer {
public:
    struct request {
        // The number of bytes which is needed to be released.
        // The reclaimer can release a different amount.
        // If less is released then the reclaimer may be invoked again.
        size_t bytes_to_reclaim;
    };
    using reclaim_fn = std::function<reclaiming_result ()>;
private:
    std::function<reclaiming_result (request)> _reclaim;
    reclaimer_scope _scope;
public:
    // Installs new reclaimer which will be invoked when system is falling
    // low on memory. 'scope' determines when reclaimer can be executed.
    reclaimer(std::function<reclaiming_result ()> reclaim, reclaimer_scope scope = reclaimer_scope::async);
    reclaimer(std::function<reclaiming_result (request)> reclaim, reclaimer_scope scope = reclaimer_scope::async);
    ~reclaimer();
    reclaiming_result do_reclaim(size_t bytes_to_reclaim) { return _reclaim(request{bytes_to_reclaim}); }
    reclaimer_scope scope() const { return _scope; }
};

extern compat::polymorphic_allocator<char>* malloc_allocator;

// Call periodically to recycle objects that were freed
// on cpu other than the one they were allocated on.
//
// Returns @true if any work was actually performed.
bool drain_cross_cpu_freelist();


// We don't want the memory code calling back into the rest of
// the system, so allow the rest of the system to tell the memory
// code how to initiate reclaim.
//
// When memory is low, calling \c hook(fn) will result in fn being called
// in a safe place wrt. allocations.
void set_reclaim_hook(
        std::function<void (std::function<void ()>)> hook);

/// \endcond

class statistics;

/// Capture a snapshot of memory allocation statistics for this lcore.
statistics stats();

/// Memory allocation statistics.
class statistics {
    uint64_t _mallocs;
    uint64_t _frees;
    uint64_t _cross_cpu_frees;
    size_t _total_memory;
    size_t _free_memory;
    uint64_t _reclaims;
    uint64_t _large_allocs;
private:
    statistics(uint64_t mallocs, uint64_t frees, uint64_t cross_cpu_frees,
            uint64_t total_memory, uint64_t free_memory, uint64_t reclaims, uint64_t large_allocs)
        : _mallocs(mallocs), _frees(frees), _cross_cpu_frees(cross_cpu_frees)
        , _total_memory(total_memory), _free_memory(free_memory), _reclaims(reclaims), _large_allocs(large_allocs) {}
public:
    /// Total number of memory allocations calls since the system was started.
    uint64_t mallocs() const { return _mallocs; }
    /// Total number of memory deallocations calls since the system was started.
    uint64_t frees() const { return _frees; }
    /// Total number of memory deallocations that occured on a different lcore
    /// than the one on which they were allocated.
    uint64_t cross_cpu_frees() const { return _cross_cpu_frees; }
    /// Total number of objects which were allocated but not freed.
    size_t live_objects() const { return mallocs() - frees(); }
    /// Total free memory (in bytes)
    size_t free_memory() const { return _free_memory; }
    /// Total allocated memory (in bytes)
    size_t allocated_memory() const { return _total_memory - _free_memory; }
    /// Total memory (in bytes)
    size_t total_memory() const { return _total_memory; }
    /// Number of reclaims performed due to low memory
    uint64_t reclaims() const { return _reclaims; }
    /// Number of allocations which violated the large allocation threshold
    uint64_t large_allocations() const { return _large_allocs; }
    friend statistics stats();
};

struct memory_layout {
    uintptr_t start;
    uintptr_t end;
};

// Discover virtual address range used by the allocator on current shard.
// Supported only when seastar allocator is enabled.
memory::memory_layout get_memory_layout();

/// Returns the value of free memory low water mark in bytes.
/// When free memory is below this value, reclaimers are invoked until it goes above again.
size_t min_free_memory();

/// Sets the value of free memory low water mark in memory::page_size units.
void set_min_free_pages(size_t pages);

/// Enable the large allocation warning threshold.
///
/// Warn when allocation above a given threshold are performed.
///
/// \param threshold size (in bytes) above which an allocation will be logged
void set_large_allocation_warning_threshold(size_t threshold);

/// Gets the current large allocation warning threshold.
size_t get_large_allocation_warning_threshold();

/// Disable large allocation warnings.
void disable_large_allocation_warning();

/// Set a different large allocation warning threshold for a scope.
class scoped_large_allocation_warning_threshold {
    size_t _old_threshold;
public:
    explicit scoped_large_allocation_warning_threshold(size_t threshold)
            : _old_threshold(get_large_allocation_warning_threshold()) {
        set_large_allocation_warning_threshold(threshold);
    }
    scoped_large_allocation_warning_threshold(const scoped_large_allocation_warning_threshold&) = delete;
    scoped_large_allocation_warning_threshold(scoped_large_allocation_warning_threshold&& x) = delete;
    ~scoped_large_allocation_warning_threshold() {
        if (_old_threshold) {
            set_large_allocation_warning_threshold(_old_threshold);
        }
    }
    void operator=(const scoped_large_allocation_warning_threshold&) const = delete;
    void operator=(scoped_large_allocation_warning_threshold&&) = delete;
};

/// Disable large allocation warnings for a scope.
class scoped_large_allocation_warning_disable {
    size_t _old_threshold;
public:
    scoped_large_allocation_warning_disable()
            : _old_threshold(get_large_allocation_warning_threshold()) {
        disable_large_allocation_warning();
    }
    scoped_large_allocation_warning_disable(const scoped_large_allocation_warning_disable&) = delete;
    scoped_large_allocation_warning_disable(scoped_large_allocation_warning_disable&& x) = delete;
    ~scoped_large_allocation_warning_disable() {
        if (_old_threshold) {
            set_large_allocation_warning_threshold(_old_threshold);
        }
    }
    void operator=(const scoped_large_allocation_warning_disable&) const = delete;
    void operator=(scoped_large_allocation_warning_disable&&) = delete;
};

/// Enable/disable heap profiling.
///
/// In order to use heap profiling you have to define
/// `SEASTAR_HEAPPROF`.
/// Heap profiling data is not currently exposed via an API for
/// inspection, instead it was designed to be inspected from a
/// debugger.
/// For an example script that makes use of the heap profiling data
/// see [scylla-gdb.py] (https://github.com/scylladb/scylla/blob/e1b22b6a4c56b4f1d0adf65d1a11db4bcb51fe7d/scylla-gdb.py#L1439)
/// This script can generate either textual representation of the data,
/// or a zoomable flame graph ([flame graph generation instructions](https://github.com/scylladb/scylla/wiki/Seastar-heap-profiler),
/// [example flame graph](https://user-images.githubusercontent.com/1389273/72920437-f0cf8a80-3d51-11ea-92f0-f3dbeb698871.png)).
void set_heap_profiling_enabled(bool);

/// Enable heap profiling for the duration of the scope.
///
/// For more information about heap profiling see
/// \ref set_heap_profiling_enabled().
class scoped_heap_profiling {
public:
    scoped_heap_profiling() noexcept;
    ~scoped_heap_profiling();
};

}
}
#include <time.h>

namespace seastar {

class thread_cputime_clock {
public:
    using rep = int64_t;
    using period = std::chrono::nanoseconds::period;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<thread_cputime_clock, duration>;
public:
    static time_point now() {
        using namespace std::chrono_literals;

        struct timespec tp;
        [[gnu::unused]] auto ret = clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tp);
        assert(ret == 0);
        return time_point(tp.tv_nsec * 1ns + tp.tv_sec * 1s);
    }
};

}

#include <boost/range/irange.hpp>


namespace seastar {

/// \addtogroup fiber-module
/// @{

/// Exception thrown when a condition variable is broken by
/// \ref condition_variable::broken().
class broken_condition_variable : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Condition variable is broken";
    }
};

/// Exception thrown when wait() operation times out
/// \ref condition_variable::wait(time_point timeout).
class condition_variable_timed_out : public std::exception {
public:
    /// Reports the exception reason.
    virtual const char* what() const noexcept {
        return "Condition variable timed out";
    }
};

/// \brief Conditional variable.
///
/// This is a standard computer science condition variable sans locking,
/// since in seastar access to variables is atomic anyway, adapted
/// for futures.  You can wait for variable to be notified.
///
/// To support exceptional conditions, a \ref broken() method
/// is provided, which causes all current waiters to stop waiting,
/// with an exceptional future returned.  This allows causing all
/// fibers that are blocked on a condition variable to continue.
/// This issimilar to POSIX's `pthread_cancel()`, with \ref wait()
/// acting as a cancellation point.

class condition_variable {
    using duration = semaphore::duration;
    using clock = semaphore::clock;
    using time_point = semaphore::time_point;
    struct condition_variable_exception_factory {
        static condition_variable_timed_out timeout() {
            return condition_variable_timed_out();
        }
        static broken_condition_variable broken() {
            return broken_condition_variable();
        }
    };
    basic_semaphore<condition_variable_exception_factory> _sem;
public:
    /// Constructs a condition_variable object.
    /// Initialzie the semaphore with a default value of 0 to enusre
    /// the first call to wait() before signal() won't be waken up immediately.
    condition_variable() : _sem(0) {}

    /// Waits until condition variable is signaled, may wake up without condition been met
    ///
    /// \return a future that becomes ready when \ref signal() is called
    ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
    ///         exception.
    future<> wait() {
        return _sem.wait();
    }

    /// Waits until condition variable is signaled or timeout is reached
    ///
    /// \param timeout time point at which wait will exit with a timeout
    ///
    /// \return a future that becomes ready when \ref signal() is called
    ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
    ///         exception. If timepoint is reached will return \ref condition_variable_timed_out exception.
    future<> wait(time_point timeout) {
        return _sem.wait(timeout);
    }

    /// Waits until condition variable is signaled or timeout is reached
    ///
    /// \param timeout duration after which wait will exit with a timeout
    ///
    /// \return a future that becomes ready when \ref signal() is called
    ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
    ///         exception. If timepoint is passed will return \ref condition_variable_timed_out exception.
    future<> wait(duration timeout) {
        return _sem.wait(timeout);
    }

    /// Waits until condition variable is notified and pred() == true, otherwise
    /// wait again.
    ///
    /// \param pred predicate that checks that awaited condition is true
    ///
    /// \return a future that becomes ready when \ref signal() is called
    ///         If the condition variable was \ref broken(), may contain an exception.
    template<typename Pred>
    future<> wait(Pred&& pred) {
        return do_until(std::forward<Pred>(pred), [this] {
            return wait();
        });
    }

    /// Waits until condition variable is notified and pred() == true or timeout is reached, otherwise
    /// wait again.
    ///
    /// \param timeout time point at which wait will exit with a timeout
    /// \param pred predicate that checks that awaited condition is true
    ///
    /// \return a future that becomes ready when \ref signal() is called
    ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
    ///         exception. If timepoint is reached will return \ref condition_variable_timed_out exception.
    /// \param
    template<typename Pred>
    future<> wait(time_point timeout, Pred&& pred) {
        return do_until(std::forward<Pred>(pred), [this, timeout] () mutable {
            return wait(timeout);
        });
    }

    /// Waits until condition variable is notified and pred() == true or timeout is reached, otherwise
    /// wait again.
    ///
    /// \param timeout duration after which wait will exit with a timeout
    /// \param pred predicate that checks that awaited condition is true
    ///
    /// \return a future that becomes ready when \ref signal() is called
    ///         If the condition variable was \ref broken() will return \ref broken_condition_variable
    ///         exception. If timepoint is passed will return \ref condition_variable_timed_out exception.
    template<typename Pred>
    future<> wait(duration timeout, Pred&& pred) {
        return wait(clock::now() + timeout, std::forward<Pred>(pred));
    }
    /// Notify variable and wake up a waiter if there is one
    void signal() {
        if (_sem.waiters()) {
            _sem.signal();
        }
    }
    /// Notify variable and wake up all waiter
    void broadcast() {
        _sem.signal(_sem.waiters());
    }

    /// Signal to waiters that an error occurred.  \ref wait() will see
    /// an exceptional future<> containing the provided exception parameter.
    /// The future is made available immediately.
    void broken() {
        _sem.broken();
    }
};

/// @}

}

#include <unordered_map>
#include <mutex>
#include <boost/lexical_cast.hpp>


/// \addtogroup logging
/// @{

namespace seastar {

/// \brief log level used with \see {logger}
/// used with the logger.do_log method.
/// Levels are in increasing order. That is if you want to see debug(3) logs you
/// will also see error(0), warn(1), info(2).
///
enum class log_level {
    error,
    warn,
    info,
    debug,
    trace,
};

std::ostream& operator<<(std::ostream& out, log_level level);
std::istream& operator>>(std::istream& in, log_level& level);
}

// Boost doesn't auto-deduce the existence of the streaming operators for some reason

namespace boost {
template<>
seastar::log_level lexical_cast(const std::string& source);

}

namespace seastar {

class logger;
class logger_registry;

/// \brief Logger class for ostream or syslog.
///
/// Java style api for logging.
/// \code {.cpp}
/// static seastar::logger logger("lsa-api");
/// logger.info("Triggering compaction");
/// \endcode
/// The output format is: (depending on level)
/// DEBUG  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
///
class logger {
    sstring _name;
    std::atomic<log_level> _level = { log_level::info };
    static std::ostream* _out;
    static std::atomic<bool> _ostream;
    static std::atomic<bool> _syslog;
private:
    struct stringer {
        void (*append)(std::ostream& os, const void* object);
        const void* object;
    };
    template <typename Arg>
    stringer stringer_for(const Arg& arg) {
        return stringer{
            [] (std::ostream& os, const void* object) {
                os << *static_cast<const std::remove_reference_t<Arg>*>(object);
            },
            &arg
        };
    };
    template <typename... Args>
    void do_log(log_level level, const char* fmt, Args&&... args);
    void really_do_log(log_level level, const char* fmt, const stringer* stringers, size_t n);
    void failed_to_log(std::exception_ptr ex);
public:
    explicit logger(sstring name);
    logger(logger&& x);
    ~logger();

    bool is_shard_zero();

    /// Test if desired log level is enabled
    ///
    /// \param level - enum level value (info|error...)
    /// \return true if the log level has been enabled.
    bool is_enabled(log_level level) const {
        return __builtin_expect(level <= _level.load(std::memory_order_relaxed), false);
    }

    /// logs to desired level if enabled, otherwise we ignore the log line
    ///
    /// \param fmt - printf style format
    /// \param args - args to print string
    ///
    template <typename... Args>
    void log(log_level level, const char* fmt, const Args&... args) {
        if (is_enabled(level)) {
            try {
                do_log(level, fmt, args...);
            } catch (...) {
                failed_to_log(std::current_exception());
            }
        }
    }

    /// Log with error tag:
    /// ERROR  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
    ///
    /// \param fmt - printf style format
    /// \param args - args to print string
    ///
    template <typename... Args>
    void error(const char* fmt, Args&&... args) {
        log(log_level::error, fmt, std::forward<Args>(args)...);
    }
    /// Log with warning tag:
    /// WARN  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
    ///
    /// \param fmt - printf style format
    /// \param args - args to print string
    ///
    template <typename... Args>
    void warn(const char* fmt, Args&&... args) {
        log(log_level::warn, fmt, std::forward<Args>(args)...);
    }
    /// Log with info tag:
    /// INFO  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
    ///
    /// \param fmt - printf style format
    /// \param args - args to print string
    ///
    template <typename... Args>
    void info(const char* fmt, Args&&... args) {
        log(log_level::info, fmt, std::forward<Args>(args)...);
    }
    /// Log with info tag on shard zero only:
    /// INFO  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
    ///
    /// \param fmt - printf style format
    /// \param args - args to print string
    ///
    template <typename... Args>
    void info0(const char* fmt, Args&&... args) {
        if (is_shard_zero()) {
            log(log_level::info, fmt, std::forward<Args>(args)...);
        }
    }
    /// Log with info tag:
    /// DEBUG  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
    ///
    /// \param fmt - printf style format
    /// \param args - args to print string
    ///
    template <typename... Args>
    void debug(const char* fmt, Args&&... args) {
        log(log_level::debug, fmt, std::forward<Args>(args)...);
    }
    /// Log with trace tag:
    /// TRACE  %Y-%m-%d %T,%03d [shard 0] - "your msg" \n
    ///
    /// \param fmt - printf style format
    /// \param args - args to print string
    ///
    template <typename... Args>
    void trace(const char* fmt, Args&&... args) {
        log(log_level::trace, fmt, std::forward<Args>(args)...);
    }

    /// \return name of the logger. Usually one logger per module
    ///
    const sstring& name() const {
        return _name;
    }

    /// \return current log level for this logger
    ///
    log_level level() const {
        return _level.load(std::memory_order_relaxed);
    }

    /// \param level - set the log level
    ///
    void set_level(log_level level) {
        _level.store(level, std::memory_order_relaxed);
    }

    /// Set output stream, default is std::cerr
    static void set_ostream(std::ostream& out);

    /// Also output to ostream. default is true
    static void set_ostream_enabled(bool enabled);

    /// Also output to stdout. default is true
    [[deprecated("Use set_ostream_enabled instead")]]
    static void set_stdout_enabled(bool enabled);

    /// Also output to syslog. default is false
    ///
    /// NOTE: syslog() can block, which will stall the reactor thread.
    ///       this should be rare (will have to fill the pipe buffer
    ///       before syslogd can clear it) but can happen.
    static void set_syslog_enabled(bool enabled);
};

/// \brief used to keep a static registry of loggers
/// since the typical use case is to do:
/// \code {.cpp}
/// static seastar::logger("my_module");
/// \endcode
/// this class is used to wrap around the static map
/// that holds pointers to all logs
///
class logger_registry {
    mutable std::mutex _mutex;
    std::unordered_map<sstring, logger*> _loggers;
public:
    /// loops through all registered loggers and sets the log level
    /// Note: this method locks
    ///
    /// \param level - desired level: error,info,...
    void set_all_loggers_level(log_level level);

    /// Given a name for a logger returns the log_level enum
    /// Note: this method locks
    ///
    /// \return log_level for the given logger name
    log_level get_logger_level(sstring name) const;

    /// Sets the log level for a given logger
    /// Note: this method locks
    ///
    /// \param name - name of logger
    /// \param level - desired level of logging
    void set_logger_level(sstring name, log_level level);

    /// Returns a list of registered loggers
    /// Note: this method locks
    ///
    /// \return all registered loggers
    std::vector<sstring> get_all_logger_names();

    /// Registers a logger with the static map
    /// Note: this method locks
    ///
    void register_logger(logger* l);
    /// Unregisters a logger with the static map
    /// Note: this method locks
    ///
    void unregister_logger(logger* l);
    /// Swaps the logger given the from->name() in the static map
    /// Note: this method locks
    ///
    void moved(logger* from, logger* to);
};

logger_registry& global_logger_registry();

enum class logger_timestamp_style {
    none,
    boot,
    real,
};

enum class logger_ostream_type {
    none,
    stdout,
    stderr,
};

struct logging_settings final {
    std::unordered_map<sstring, log_level> logger_levels;
    log_level default_level;
    bool stdout_enabled;
    bool syslog_enabled;
    logger_timestamp_style stdout_timestamp_style = logger_timestamp_style::real;
    logger_ostream_type logger_ostream = logger_ostream_type::stderr;
};

/// Shortcut for configuring the logging system all at once.
///
void apply_logging_settings(const logging_settings&);

/// \cond internal

extern thread_local uint64_t logging_failures;

sstring pretty_type_name(const std::type_info&);

sstring level_name(log_level level);

template <typename T>
class logger_for : public logger {
public:
    logger_for() : logger(pretty_type_name(typeid(T))) {}
};

template <typename... Args>
void
logger::do_log(log_level level, const char* fmt, Args&&... args) {
    [&](auto... stringers) {
        stringer s[sizeof...(stringers)] = {stringers...};
        this->really_do_log(level, fmt, s, sizeof...(stringers));
    } (stringer_for<Args>(std::forward<Args>(args))...);
}

/// \endcond
} // end seastar namespace

// Pretty-printer for exceptions to be logged, e.g., std::current_exception().
namespace std {
std::ostream& operator<<(std::ostream&, const std::exception_ptr&);
std::ostream& operator<<(std::ostream&, const std::exception&);
std::ostream& operator<<(std::ostream&, const std::system_error&);
}

/// @}

namespace seastar {

class manual_clock {
public:
    using rep = int64_t;
    using period = std::chrono::nanoseconds::period;
    using duration = std::chrono::duration<rep, period>;
    using time_point = std::chrono::time_point<manual_clock, duration>;
private:
    static std::atomic<rep> _now;
    static void expire_timers();
public:
    manual_clock();
    static time_point now() {
        return time_point(duration(_now.load(std::memory_order_relaxed)));
    }
    static void advance(duration d);
};

extern template class timer<manual_clock>;

}
#include <vector>


/*!
 * \file metrics_registration.hh
 * \brief holds the metric_groups definition needed by class that reports metrics
 *
 * If class A needs to report metrics,
 * typically you include metrics_registration.hh, in A header file and add to A:
 * * metric_groups _metrics as a member
 * * set_metrics() method that would be called in the constructor.
 * \code
 * class A {
 *   metric_groups _metrics
 *
 *   void setup_metrics();
 *
 * };
 * \endcode
 * To define the metrics, include in your source file metircs.hh
 * @see metrics.hh for the definition for adding a metric.
 */

namespace seastar {

namespace metrics {

namespace impl {
class metric_groups_def;
struct metric_definition_impl;
class metric_groups_impl;
}

using group_name_type = sstring; /*!< A group of logically related metrics */
class metric_groups;

class metric_definition {
    std::unique_ptr<impl::metric_definition_impl> _impl;
public:
    metric_definition(const impl::metric_definition_impl& impl) noexcept;
    metric_definition(metric_definition&& m) noexcept;
    ~metric_definition();
    friend metric_groups;
    friend impl::metric_groups_impl;
};

class metric_group_definition {
public:
    group_name_type name;
    std::initializer_list<metric_definition> metrics;
    metric_group_definition(const group_name_type& name, std::initializer_list<metric_definition> l);
    metric_group_definition(const metric_group_definition&) = delete;
    ~metric_group_definition();
};

/*!
 * metric_groups
 * \brief holds the metric definition.
 *
 * Add multiple metric groups definitions.
 * Initialization can be done in the constructor or with a call to add_group
 * @see metrics.hh for example and supported metrics
 */
class metric_groups {
    std::unique_ptr<impl::metric_groups_def> _impl;
public:
    metric_groups() noexcept;
    metric_groups(metric_groups&&) = default;
    virtual ~metric_groups();
    metric_groups& operator=(metric_groups&&) = default;
    /*!
     * \brief add metrics belong to the same group in the constructor.
     *
     * combine the constructor with the add_group functionality.
     */
    metric_groups(std::initializer_list<metric_group_definition> mg);

    /*!
     * \brief Add metrics belonging to the same group.
     *
     * Use the metrics creation functions to add metrics.
     *
     * For example:
     *  _metrics.add_group("my_group", {
     *      make_counter("my_counter_name1", counter, description("my counter description")),
     *      make_counter("my_counter_name2", counter, description("my second counter description")),
     *      make_gauge("my_gauge_name1", gauge, description("my gauge description")),
     *  });
     *
     * Metric name should be unique inside the group.
     * You can chain add_group calls like:
     * _metrics.add_group("my group1", {...}).add_group("my group2", {...});
     *
     * This overload (with initializer_list) is needed because metric_definition
     * has no copy constructor, so the other overload (with vector) cannot be
     * invoked on a braced-init-list.
     */
    metric_groups& add_group(const group_name_type& name, const std::initializer_list<metric_definition>& l);

    /*!
     * \brief Add metrics belonging to the same group.
     *
     * Use the metrics creation functions to add metrics.
     *
     * For example:
     *  vector<metric_definition> v;
     *  v.push_back(make_counter("my_counter_name1", counter, description("my counter description")));
     *  v.push_back(make_counter("my_counter_name2", counter, description("my second counter description")));
     *  v.push_back(make_gauge("my_gauge_name1", gauge, description("my gauge description")));
     *  _metrics.add_group("my_group", v);
     *
     * Metric name should be unique inside the group.
     * You can chain add_group calls like:
     * _metrics.add_group("my group1", vec1).add_group("my group2", vec2);
     */
    metric_groups& add_group(const group_name_type& name, const std::vector<metric_definition>& l);

    /*!
     * \brief clear all metrics groups registrations.
     */
    void clear();
};


/*!
 * \brief hold a single metric group
 * Initialization is done in the constructor or
 * with a call to add_group
 */
class metric_group : public metric_groups {
public:
    metric_group() noexcept;
    metric_group(const metric_group&) = delete;
    metric_group(metric_group&&) = default;
    virtual ~metric_group();
    metric_group& operator=(metric_group&&) = default;

    /*!
     * \brief add metrics belong to the same group in the constructor.
     *
     *
     */
    metric_group(const group_name_type& name, std::initializer_list<metric_definition> l);
};


}
}


#include <map>
#include <vector>

namespace seastar {
namespace metrics {


/*!
 * \brief Histogram bucket type
 *
 * A histogram bucket contains an upper bound and the number
 * of events in the buckets.
 */
struct histogram_bucket {
    uint64_t count = 0; // number of events.
    double upper_bound = 0;      // Inclusive.
};


/*!
 * \brief Histogram data type
 *
 * The histogram struct is a container for histogram values.
 * It is not a histogram implementation but it will be used by histogram
 * implementation to return its internal values.
 */
struct histogram {
    uint64_t sample_count = 0;
    double sample_sum = 0;
    std::vector<histogram_bucket> buckets; // Ordered in increasing order of upper_bound, +Inf bucket is optional.

    /*!
     * \brief Addition assigning a historgram
     *
     * The histogram must match the buckets upper bounds
     * or an exception will be thrown
     */
    histogram& operator+=(const histogram& h);

    /*!
     * \brief Addition historgrams
     *
     * Add two histograms and return the result as a new histogram
     * The histogram must match the buckets upper bounds
     * or an exception will be thrown
     */
    histogram operator+(const histogram& h) const;

    /*!
     * \brief Addition historgrams
     *
     * Add two histograms and return the result as a new histogram
     * The histogram must match the buckets upper bounds
     * or an exception will be thrown
     */
    histogram operator+(histogram&& h) const;

};

}

}

/*! \file metrics.hh
 *  \brief header for metrics creation.
 *
 *  This header file contains the metrics creation method with their helper function.
 *  Include this file when need to create metrics.
 *  Typically this will be in your source file.
 *
 *  Code that is under the impl namespace should not be used directly.
 *
 */

namespace seastar {

/*!
 * \namespace seastar::metrics
 * \brief metrics creation and registration
 *
 * the metrics namespace holds the relevant method and classes to generate metrics.
 *
 * The metrics layer support registering metrics, that later will be
 * exported via different API protocols.
 *
 * To be able to support multiple protocols the following simplifications where made:
 * 1. The id of the metrics is based on the collectd id
 * 2. A metric could be a single value either a reference or a function
 *
 * To add metrics definition to class A do the following:
 * * Add a metrics_group memeber to A
 * * Add a a set_metrics() method that would be called in the constructor.
 *
 *
 * In A header file
 * \code
 * #include "core/metrics_registration.hh"
 * class A {
 *   metric_groups _metrics
 *
 *   void setup_metrics();
 *
 * };
 * \endcode
 *
 * In A source file:
 *
 * \code
 * include "core/metrics.hh"
 *
 * void A::setup_metrics() {
 *   namespace sm = seastar::metrics;
 *   _metrics = sm::create_metric_group();
 *   _metrics->add_group("cache", {sm::make_gauge("bytes", "used", [this] { return _region.occupancy().used_space(); })});
 * }
 * \endcode
 */

namespace metrics {

class double_registration : public std::runtime_error {
public:
    double_registration(std::string what);
};

/*!
 * \defgroup metrics_types metrics type definitions
 * The following are for the metric layer use, do not use them directly
 * Instead use the make_counter, make_gauge, make_absolute and make_derived
 *
 */
using metric_type_def = sstring; /*!< Used to hold an inherit type (like bytes)*/
using metric_name_type = sstring; /*!<  The metric name'*/
using instance_id_type = sstring; /*!<  typically used for the shard id*/

/*!
 * \brief Human-readable description of a metric/group.
 *
 *
 * Uses a separate class to deal with type resolution
 *
 * Add this to metric creation:
 *
 * \code
 * _metrics->add_group("groupname", {
 *   sm::make_gauge("metric_name", value, description("A documentation about the return value"))
 * });
 * \endcode
 *
 */
class description {
public:
    description(sstring s = sstring()) : _s(std::move(s))
    {}
    const sstring& str() const {
        return _s;
    }
private:
    sstring _s;
};

/*!
 * \brief Label a metrics
 *
 * Label are useful for adding information about a metric that
 * later you would need to aggregate by.
 * For example, if you have multiple queues on a shard.
 * Adding the queue id as a Label will allow you to use the same name
 * of the metrics with multiple id instances.
 *
 * label_instance holds an instance of label consist of a key and value.
 *
 * Typically you will not generate a label_instance yourself, but use a label
 * object for that.
 * @see label for more information
 *
 *
 */
class label_instance {
    sstring _key;
    sstring _value;
public:
    /*!
     * \brief create a label_instance
     * label instance consists of key and value.
     * The key is an sstring.
     * T - the value type can be any type that can be lexical_cast to string
     * (ie. if it support the redirection operator for stringstream).
     *
     * All primitive types are supported so all the following examples are valid:
     * label_instance a("smp_queue", 1)
     * label_instance a("my_key", "my_value")
     * label_instance a("internal_id", -1)
     */
    template<typename T>
    label_instance(const sstring& key, T v) : _key(key), _value(boost::lexical_cast<std::string>(v)){}

    /*!
     * \brief returns the label key
     */
    const sstring key() const {
        return _key;
    }

    /*!
     * \brief returns the label value
     */
    const sstring value() const {
        return _value;
    }
    bool operator<(const label_instance&) const;
    bool operator==(const label_instance&) const;
    bool operator!=(const label_instance&) const;
};


/*!
 * \brief Class that creates label instances
 *
 * A factory class to create label instance
 * Typically, the same Label name is used in multiple places.
 * label is a label factory, you create it once, and use it to create the label_instance.
 *
 * In the example we would like to label the smp_queue with with the queue owner
 *
 * seastar::metrics::label smp_owner("smp_owner");
 *
 * now, when creating a new smp metric we can add a label to it:
 *
 * sm::make_queue_length("send_batch_queue_length", _last_snt_batch, {smp_owner(cpuid)})
 *
 * where cpuid in this case is unsiged.
 */
class label {
    sstring key;
public:
    using instance = label_instance;
    /*!
     * \brief creating a label
     * key is the label name, it will be the key for all label_instance
     * that will be created from this label.
     */
    explicit label(const sstring& key) : key(key) {
    }

    /*!
     * \brief creating a label instance
     *
     * Use the function operator to create a new label instance.
     * T - the value type can be any type that can be lexical_cast to string
     * (ie. if it support the redirection operator for stringstream).
     *
     * All primitive types are supported so if lab is a label, all the following examples are valid:
     * lab(1)
     * lab("my_value")
     * lab(-1)
     */
    template<typename T>
    instance operator()(T value) const {
        return label_instance(key, std::forward<T>(value));
    }

    /*!
     * \brief returns the label name
     */
    const sstring& name() const {
        return key;
    }
};

/*!
 * \namesapce impl
 * \brief holds the implementation parts of the metrics layer, do not use directly.
 *
 * The metrics layer define a thin API for adding metrics.
 * Some of the implementation details need to be in the header file, they should not be use directly.
 */
namespace impl {

// The value binding data types
enum class data_type : uint8_t {
    COUNTER, // unsigned int 64
    GAUGE, // double
    DERIVE, // signed int 64
    ABSOLUTE, // unsigned int 64
    HISTOGRAM,
};

/*!
 * \breif A helper class that used to return metrics value.
 *
 * Do not use directly @see metrics_creation
 */
struct metric_value {
    compat::variant<double, histogram> u;
    data_type _type;
    data_type type() const {
        return _type;
    }

    double d() const {
        return compat::get<double>(u);
    }

    uint64_t ui() const {
        return compat::get<double>(u);
    }

    int64_t i() const {
        return compat::get<double>(u);
    }

    metric_value()
            : _type(data_type::GAUGE) {
    }

    metric_value(histogram&& h, data_type t = data_type::HISTOGRAM) :
        u(std::move(h)), _type(t) {
    }
    metric_value(const histogram& h, data_type t = data_type::HISTOGRAM) :
        u(h), _type(t) {
    }

    metric_value(double d, data_type t)
            : u(d), _type(t) {
    }

    metric_value& operator=(const metric_value& c) = default;

    metric_value& operator+=(const metric_value& c) {
        *this = *this + c;
        return *this;
    }

    metric_value operator+(const metric_value& c);
    const histogram& get_histogram() const {
        return compat::get<histogram>(u);
    }
};

using metric_function = std::function<metric_value()>;

struct metric_type {
    data_type base_type;
    metric_type_def type_name;
};

struct metric_definition_impl {
    metric_name_type name;
    metric_type type;
    metric_function f;
    description d;
    bool enabled = true;
    std::map<sstring, sstring> labels;
    metric_definition_impl& operator ()(bool enabled);
    metric_definition_impl& operator ()(const label_instance& label);
    metric_definition_impl(
        metric_name_type name,
        metric_type type,
        metric_function f,
        description d,
        std::vector<label_instance> labels);
};

class metric_groups_def {
public:
    metric_groups_def() = default;
    virtual ~metric_groups_def() = default;
    metric_groups_def(const metric_groups_def&) = delete;
    metric_groups_def(metric_groups_def&&) = default;
    virtual metric_groups_def& add_metric(group_name_type name, const metric_definition& md) = 0;
    virtual metric_groups_def& add_group(group_name_type name, const std::initializer_list<metric_definition>& l) = 0;
    virtual metric_groups_def& add_group(group_name_type name, const std::vector<metric_definition>& l) = 0;
};

instance_id_type shard();

template<typename T, typename En = std::true_type>
struct is_callable;

template<typename T>
struct is_callable<T, typename std::integral_constant<bool, !std::is_void<typename std::result_of<T()>::type>::value>::type> : public std::true_type {
};

template<typename T>
struct is_callable<T, typename std::enable_if<std::is_fundamental<T>::value, std::true_type>::type> : public std::false_type {
};

template<typename T, typename = std::enable_if_t<is_callable<T>::value>>
metric_function make_function(T val, data_type dt) {
    return [dt, val] {
        return metric_value(val(), dt);
    };
}

template<typename T, typename = std::enable_if_t<!is_callable<T>::value>>
metric_function make_function(T& val, data_type dt) {
    return [dt, &val] {
        return metric_value(val, dt);
    };
}
}

extern const bool metric_disabled;

extern label shard_label;
extern label type_label;

/*
 * The metrics definition are defined to be compatible with collectd metrics defintion.
 * Typically you should used gauge or derived.
 */


/*!
 * \brief Gauge are a general purpose metric.
 *
 * They can support floating point and can increase or decrease
 */
template<typename T>
impl::metric_definition_impl make_gauge(metric_name_type name,
        T&& val, description d=description(), std::vector<label_instance> labels = {}) {
    return {name, {impl::data_type::GAUGE, "gauge"}, make_function(std::forward<T>(val), impl::data_type::GAUGE), d, labels};
}

/*!
 * \brief Gauge are a general purpose metric.
 *
 * They can support floating point and can increase or decrease
 */
template<typename T>
impl::metric_definition_impl make_gauge(metric_name_type name,
        description d, T&& val) {
    return {name, {impl::data_type::GAUGE, "gauge"}, make_function(std::forward<T>(val), impl::data_type::GAUGE), d, {}};
}

/*!
 * \brief Gauge are a general purpose metric.
 *
 * They can support floating point and can increase or decrease
 */
template<typename T>
impl::metric_definition_impl make_gauge(metric_name_type name,
        description d, std::vector<label_instance> labels, T&& val) {
    return {name, {impl::data_type::GAUGE, "gauge"}, make_function(std::forward<T>(val), impl::data_type::GAUGE), d, labels};
}


/*!
 * \brief Derive are used when a rate is more interesting than the value.
 *
 * Derive is an integer value that can increase or decrease, typically it is used when looking at the
 * derivation of the value.
 *
 * It is OK to use it when counting things and if no wrap-around is expected (it shouldn't) it's prefer over counter metric.
 */
template<typename T>
impl::metric_definition_impl make_derive(metric_name_type name,
        T&& val, description d=description(), std::vector<label_instance> labels = {}) {
    return {name, {impl::data_type::DERIVE, "derive"}, make_function(std::forward<T>(val), impl::data_type::DERIVE), d, labels};
}


/*!
 * \brief Derive are used when a rate is more interesting than the value.
 *
 * Derive is an integer value that can increase or decrease, typically it is used when looking at the
 * derivation of the value.
 *
 * It is OK to use it when counting things and if no wrap-around is expected (it shouldn't) it's prefer over counter metric.
 */
template<typename T>
impl::metric_definition_impl make_derive(metric_name_type name, description d,
        T&& val) {
    return {name, {impl::data_type::DERIVE, "derive"}, make_function(std::forward<T>(val), impl::data_type::DERIVE), d, {}};
}


/*!
 * \brief Derive are used when a rate is more interesting than the value.
 *
 * Derive is an integer value that can increase or decrease, typically it is used when looking at the
 * derivation of the value.
 *
 * It is OK to use it when counting things and if no wrap-around is expected (it shouldn't) it's prefer over counter metric.
 */
template<typename T>
impl::metric_definition_impl make_derive(metric_name_type name, description d, std::vector<label_instance> labels,
        T&& val) {
    return {name, {impl::data_type::DERIVE, "derive"}, make_function(std::forward<T>(val), impl::data_type::DERIVE), d, labels};
}


/*!
 * \brief create a counter metric
 *
 * Counters are similar to derived, but they assume monotony, so if a counter value decrease in a series it is count as a wrap-around.
 * It is better to use large enough data value than to use counter.
 *
 */
template<typename T>
impl::metric_definition_impl make_counter(metric_name_type name,
        T&& val, description d=description(), std::vector<label_instance> labels = {}) {
    return {name, {impl::data_type::COUNTER, "counter"}, make_function(std::forward<T>(val), impl::data_type::COUNTER), d, labels};
}

/*!
 * \brief create an absolute metric.
 *
 * Absolute are used for metric that are being erased after each time they are read.
 * They are here for compatibility reasons and should general be avoided in most applications.
 */
template<typename T>
impl::metric_definition_impl make_absolute(metric_name_type name,
        T&& val, description d=description(), std::vector<label_instance> labels = {}) {
    return {name, {impl::data_type::ABSOLUTE, "absolute"}, make_function(std::forward<T>(val), impl::data_type::ABSOLUTE), d, labels};
}

/*!
 * \brief create a histogram metric.
 *
 * Histograms are a list o buckets with upper values and counter for the number
 * of entries in each bucket.
 */
template<typename T>
impl::metric_definition_impl make_histogram(metric_name_type name,
        T&& val, description d=description(), std::vector<label_instance> labels = {}) {
    return  {name, {impl::data_type::HISTOGRAM, "histogram"}, make_function(std::forward<T>(val), impl::data_type::HISTOGRAM), d, labels};
}

/*!
 * \brief create a histogram metric.
 *
 * Histograms are a list o buckets with upper values and counter for the number
 * of entries in each bucket.
 */
template<typename T>
impl::metric_definition_impl make_histogram(metric_name_type name,
        description d, std::vector<label_instance> labels, T&& val) {
    return  {name, {impl::data_type::HISTOGRAM, "histogram"}, make_function(std::forward<T>(val), impl::data_type::HISTOGRAM), d, labels};
}


/*!
 * \brief create a histogram metric.
 *
 * Histograms are a list o buckets with upper values and counter for the number
 * of entries in each bucket.
 */
template<typename T>
impl::metric_definition_impl make_histogram(metric_name_type name,
        description d, T&& val) {
    return  {name, {impl::data_type::HISTOGRAM, "histogram"}, make_function(std::forward<T>(val), impl::data_type::HISTOGRAM), d, {}};
}


/*!
 * \brief create a total_bytes metric.
 *
 * total_bytes are used for an ever growing counters, like the total bytes
 * passed on a network.
 */

template<typename T>
impl::metric_definition_impl make_total_bytes(metric_name_type name,
        T&& val, description d=description(), std::vector<label_instance> labels = {},
        instance_id_type instance = impl::shard()) {
    return make_derive(name, std::forward<T>(val), d, labels)(type_label("total_bytes"));
}

/*!
 * \brief create a current_bytes metric.
 *
 * current_bytes are used to report on current status in bytes.
 * For example the current free memory.
 */

template<typename T>
impl::metric_definition_impl make_current_bytes(metric_name_type name,
        T&& val, description d=description(), std::vector<label_instance> labels = {},
        instance_id_type instance = impl::shard()) {
    return make_derive(name, std::forward<T>(val), d, labels)(type_label("bytes"));
}


/*!
 * \brief create a queue_length metric.
 *
 * queue_length are used to report on queue length
 */

template<typename T>
impl::metric_definition_impl make_queue_length(metric_name_type name,
        T&& val, description d=description(), std::vector<label_instance> labels = {},
        instance_id_type instance = impl::shard()) {
    return make_gauge(name, std::forward<T>(val), d, labels)(type_label("queue_length"));
}


/*!
 * \brief create a total operation metric.
 *
 * total_operations are used for ever growing operation counter.
 */

template<typename T>
impl::metric_definition_impl make_total_operations(metric_name_type name,
        T&& val, description d=description(), std::vector<label_instance> labels = {},
        instance_id_type instance = impl::shard()) {
    return make_derive(name, std::forward<T>(val), d, labels)(type_label("total_operations"));
}

/*! @} */
}
}
#include <deque>

/// \file

namespace seastar {

using shard_id = unsigned;

class smp_service_group;
class reactor_backend_selector;

namespace internal {

unsigned smp_service_group_id(smp_service_group ssg);

}

/// Returns shard_id of the of the current shard.
shard_id this_shard_id();

/// Configuration for smp_service_group objects.
///
/// \see create_smp_service_group()
struct smp_service_group_config {
    /// The maximum number of non-local requests that execute on a shard concurrently
    ///
    /// Will be adjusted upwards to allow at least one request per non-local shard.
    unsigned max_nonlocal_requests = 0;
};

/// A resource controller for cross-shard calls.
///
/// An smp_service_group allows you to limit the concurrency of
/// smp::submit_to() and similar calls. While it's easy to limit
/// the caller's concurrency (for example, by using a semaphore),
/// the concurrency at the remote end can be multiplied by a factor
/// of smp::count-1, which can be large.
///
/// The class is called a service _group_ because it can be used
/// to group similar calls that share resource usage characteristics,
/// need not be isolated from each other, but do need to be isolated
/// from other groups. Calls in a group should not nest; doing so
/// can result in ABA deadlocks.
///
/// Nested submit_to() calls must form a directed acyclic graph
/// when considering their smp_service_groups as nodes. For example,
/// if a call using ssg1 then invokes another call using ssg2, the
/// internal call may not call again via either ssg1 or ssg2, or it
/// may form a cycle (and risking an ABBA deadlock). Create a
/// new smp_service_group_instead.
class smp_service_group {
    unsigned _id;
private:
    explicit smp_service_group(unsigned id) : _id(id) {}

    friend unsigned internal::smp_service_group_id(smp_service_group ssg);
    friend smp_service_group default_smp_service_group();
    friend future<smp_service_group> create_smp_service_group(smp_service_group_config ssgc);
};

inline
unsigned
internal::smp_service_group_id(smp_service_group ssg) {
    return ssg._id;
}

/// Returns the default smp_service_group. This smp_service_group
/// does not impose any limits on concurrency in the target shard.
/// This makes is deadlock-safe, but can consume unbounded resources,
/// and should therefore only be used when initiator concurrency is
/// very low (e.g. administrative tasks).
smp_service_group default_smp_service_group();

/// Creates an smp_service_group with the specified configuration.
///
/// The smp_service_group is global, and after this call completes,
/// the returned value can be used on any shard.
future<smp_service_group> create_smp_service_group(smp_service_group_config ssgc);

/// Destroy an smp_service_group.
///
/// Frees all resources used by an smp_service_group. It must not
/// be used again once this function is called.
future<> destroy_smp_service_group(smp_service_group ssg);

inline
smp_service_group default_smp_service_group() {
    return smp_service_group(0);
}

using smp_timeout_clock = lowres_clock;
using smp_service_group_semaphore = basic_semaphore<named_semaphore_exception_factory, smp_timeout_clock>;
using smp_service_group_semaphore_units = semaphore_units<named_semaphore_exception_factory, smp_timeout_clock>;

static constexpr smp_timeout_clock::time_point smp_no_timeout = smp_timeout_clock::time_point::max();

/// Options controlling the behaviour of \ref smp::submit_to().
struct smp_submit_to_options {
    /// Controls resource allocation.
    smp_service_group service_group = default_smp_service_group();
    /// The timeout is relevant only to the time the call spends waiting to be
    /// processed by the remote shard, and *not* to the time it takes to be
    /// executed there.
    smp_timeout_clock::time_point timeout = smp_no_timeout;

    smp_submit_to_options(smp_service_group service_group = default_smp_service_group(), smp_timeout_clock::time_point timeout = smp_no_timeout)
        : service_group(service_group)
        , timeout(timeout) {
    }
};

void init_default_smp_service_group(shard_id cpu);

smp_service_group_semaphore& get_smp_service_groups_semaphore(unsigned ssg_id, shard_id t);

class smp_message_queue {
    static constexpr size_t queue_length = 128;
    static constexpr size_t batch_size = 16;
    static constexpr size_t prefetch_cnt = 2;
    struct work_item;
    struct lf_queue_remote {
        reactor* remote;
    };
    using lf_queue_base = boost::lockfree::spsc_queue<work_item*,
                            boost::lockfree::capacity<queue_length>>;
    // use inheritence to control placement order
    struct lf_queue : lf_queue_remote, lf_queue_base {
        lf_queue(reactor* remote) : lf_queue_remote{remote} {}
        void maybe_wakeup();
        ~lf_queue();
    };
    lf_queue _pending;
    lf_queue _completed;
    struct alignas(seastar::cache_line_size) {
        size_t _sent = 0;
        size_t _compl = 0;
        size_t _last_snt_batch = 0;
        size_t _last_cmpl_batch = 0;
        size_t _current_queue_length = 0;
    };
    // keep this between two structures with statistics
    // this makes sure that they have at least one cache line
    // between them, so hw prefetcher will not accidentally prefetch
    // cache line used by another cpu.
    metrics::metric_groups _metrics;
    struct alignas(seastar::cache_line_size) {
        size_t _received = 0;
        size_t _last_rcv_batch = 0;
    };
    struct work_item : public task {
        explicit work_item(smp_service_group ssg) : task(current_scheduling_group()), ssg(ssg) {}
        smp_service_group ssg;
        virtual ~work_item() {}
        virtual void fail_with(std::exception_ptr) = 0;
        void process();
        virtual void complete() = 0;
    };
    template <typename Func>
    struct async_work_item : work_item {
        smp_message_queue& _queue;
        Func _func;
        using futurator = futurize<std::result_of_t<Func()>>;
        using future_type = typename futurator::type;
        using value_type = typename future_type::value_type;
        compat::optional<value_type> _result;
        std::exception_ptr _ex; // if !_result
        typename futurator::promise_type _promise; // used on local side
        async_work_item(smp_message_queue& queue, smp_service_group ssg, Func&& func) : work_item(ssg), _queue(queue), _func(std::move(func)) {}
        virtual void fail_with(std::exception_ptr ex) override {
            _promise.set_exception(std::move(ex));
        }
        virtual void run_and_dispose() noexcept override {
            // _queue.respond() below forwards the continuation chain back to the
            // calling shard.
            (void)futurator::apply(this->_func).then_wrapped([this] (auto f) {
                if (f.failed()) {
                    _ex = f.get_exception();
                } else {
                    _result = f.get();
                }
                _queue.respond(this);
            });
            // We don't delete the task here as the creator of the work item will
            // delete it on the origin shard.
        }
        virtual void complete() override {
            if (_result) {
                _promise.set_value(std::move(*_result));
            } else {
                // FIXME: _ex was allocated on another cpu
                _promise.set_exception(std::move(_ex));
            }
        }
        future_type get_future() { return _promise.get_future(); }
    };
    union tx_side {
        tx_side() {}
        ~tx_side() {}
        void init() { new (&a) aa; }
        struct aa {
            std::deque<work_item*> pending_fifo;
        } a;
    } _tx;
    std::vector<work_item*> _completed_fifo;
public:
    smp_message_queue(reactor* from, reactor* to);
    ~smp_message_queue();
    template <typename Func>
    futurize_t<std::result_of_t<Func()>> submit(shard_id t, smp_submit_to_options options, Func&& func) {
        auto wi = std::make_unique<async_work_item<Func>>(*this, options.service_group, std::forward<Func>(func));
        auto fut = wi->get_future();
        submit_item(t, options.timeout, std::move(wi));
        return fut;
    }
    void start(unsigned cpuid);
    template<size_t PrefetchCnt, typename Func>
    size_t process_queue(lf_queue& q, Func process);
    size_t process_incoming();
    size_t process_completions(shard_id t);
    void stop();
private:
    void work();
    void submit_item(shard_id t, smp_timeout_clock::time_point timeout, std::unique_ptr<work_item> wi);
    void respond(work_item* wi);
    void move_pending();
    void flush_request_batch();
    void flush_response_batch();
    bool has_unflushed_responses() const;
    bool pure_poll_rx() const;
    bool pure_poll_tx() const;

    friend class smp;
};

class smp {
    static std::vector<posix_thread> _threads;
    static std::vector<std::function<void ()>> _thread_loops; // for dpdk
    static compat::optional<boost::barrier> _all_event_loops_done;
    static std::vector<reactor*> _reactors;
    struct qs_deleter {
      void operator()(smp_message_queue** qs) const;
    };
    static std::unique_ptr<smp_message_queue*[], qs_deleter> _qs;
    static std::thread::id _tmain;
    static bool _using_dpdk;

    template <typename Func>
    using returns_future = is_future<std::result_of_t<Func()>>;
    template <typename Func>
    using returns_void = std::is_same<std::result_of_t<Func()>, void>;
public:
    static boost::program_options::options_description get_options_description();
    static void register_network_stacks();
    static void configure(boost::program_options::variables_map vm, reactor_config cfg = {});
    static void cleanup();
    static void cleanup_cpu();
    static void arrive_at_event_loop_end();
    static void join_all();
    static bool main_thread() { return std::this_thread::get_id() == _tmain; }

    /// Runs a function on a remote core.
    ///
    /// \param t designates the core to run the function on (may be a remote
    ///          core or the local core).
    /// \param options an \ref smp_submit_to_options that contains options for this call.
    /// \param func a callable to run on core \c t.
    ///          If \c func is a temporary object, its lifetime will be
    ///          extended by moving. This movement and the eventual
    ///          destruction of func are both done in the _calling_ core.
    ///          If \c func is a reference, the caller must guarantee that
    ///          it will survive the call.
    /// \return whatever \c func returns, as a future<> (if \c func does not return a future,
    ///         submit_to() will wrap it in a future<>).
    template <typename Func>
    static futurize_t<std::result_of_t<Func()>> submit_to(unsigned t, smp_submit_to_options options, Func&& func) {
        using ret_type = std::result_of_t<Func()>;
        if (t == this_shard_id()) {
            try {
                if (!is_future<ret_type>::value) {
                    // Non-deferring function, so don't worry about func lifetime
                    return futurize<ret_type>::apply(std::forward<Func>(func));
                } else if (std::is_lvalue_reference<Func>::value) {
                    // func is an lvalue, so caller worries about its lifetime
                    return futurize<ret_type>::apply(func);
                } else {
                    // Deferring call on rvalue function, make sure to preserve it across call
                    auto w = std::make_unique<std::decay_t<Func>>(std::move(func));
                    auto ret = futurize<ret_type>::apply(*w);
                    return ret.finally([w = std::move(w)] {});
                }
            } catch (...) {
                // Consistently return a failed future rather than throwing, to simplify callers
                return futurize<std::result_of_t<Func()>>::make_exception_future(std::current_exception());
            }
        } else {
            return _qs[t][this_shard_id()].submit(t, options, std::forward<Func>(func));
        }
    }
    /// Runs a function on a remote core.
    ///
    /// Uses default_smp_service_group() to control resource allocation.
    ///
    /// \param t designates the core to run the function on (may be a remote
    ///          core or the local core).
    /// \param func a callable to run on core \c t.
    ///          If \c func is a temporary object, its lifetime will be
    ///          extended by moving. This movement and the eventual
    ///          destruction of func are both done in the _calling_ core.
    ///          If \c func is a reference, the caller must guarantee that
    ///          it will survive the call.
    /// \return whatever \c func returns, as a future<> (if \c func does not return a future,
    ///         submit_to() will wrap it in a future<>).
    template <typename Func>
    static futurize_t<std::result_of_t<Func()>> submit_to(unsigned t, Func&& func) {
        return submit_to(t, default_smp_service_group(), std::forward<Func>(func));
    }
    static bool poll_queues();
    static bool pure_poll_queues();
    static boost::integer_range<unsigned> all_cpus() {
        return boost::irange(0u, count);
    }
    /// Invokes func on all shards.
    ///
    /// \param options the options to forward to the \ref smp::submit_to()
    ///         called behind the scenes.
    /// \param func the function to be invoked on each shard. May return void or
    ///         future<>. Each async invocation will work with a separate copy
    ///         of \c func.
    /// \returns a future that resolves when all async invocations finish.
    template<typename Func>
    static future<> invoke_on_all(smp_submit_to_options options, Func&& func) {
        static_assert(std::is_same<future<>, typename futurize<std::result_of_t<Func()>>::type>::value, "bad Func signature");
        return parallel_for_each(all_cpus(), [options, &func] (unsigned id) {
            return smp::submit_to(id, options, Func(func));
        });
    }
    /// Invokes func on all shards.
    ///
    /// \param func the function to be invoked on each shard. May return void or
    ///         future<>. Each async invocation will work with a separate copy
    ///         of \c func.
    /// \returns a future that resolves when all async invocations finish.
    ///
    /// Passes the default \ref smp_submit_to_options to the
    /// \ref smp::submit_to() called behind the scenes.
    template<typename Func>
    static future<> invoke_on_all(Func&& func) {
        return invoke_on_all(smp_submit_to_options{}, std::forward<Func>(func));
    }
    /// Invokes func on all other shards.
    ///
    /// \param cpu_id the cpu on which **not** to run the function.
    /// \param options the options to forward to the \ref smp::submit_to()
    ///         called behind the scenes.
    /// \param func the function to be invoked on each shard. May return void or
    ///         future<>. Each async invocation will work with a separate copy
    ///         of \c func.
    /// \returns a future that resolves when all async invocations finish.
    template<typename Func>
    static future<> invoke_on_others(unsigned cpu_id, smp_submit_to_options options, Func func) {
        static_assert(std::is_same<future<>, typename futurize<std::result_of_t<Func()>>::type>::value, "bad Func signature");
        return parallel_for_each(all_cpus(), [cpu_id, options, func = std::move(func)] (unsigned id) {
            return id != cpu_id ? smp::submit_to(id, options, func) : make_ready_future<>();
        });
    }
    /// Invokes func on all other shards.
    ///
    /// \param cpu_id the cpu on which **not** to run the function.
    /// \param func the function to be invoked on each shard. May return void or
    ///         future<>. Each async invocation will work with a separate copy
    ///         of \c func.
    /// \returns a future that resolves when all async invocations finish.
    ///
    /// Passes the default \ref smp_submit_to_options to the
    /// \ref smp::submit_to() called behind the scenes.
    template<typename Func>
    static future<> invoke_on_others(unsigned cpu_id, Func func) {
        return invoke_on_others(cpu_id, smp_submit_to_options{}, std::move(func));
    }
private:
    static void start_all_queues();
    static void pin(unsigned cpu_id);
    static void allocate_reactor(unsigned id, reactor_backend_selector rbs, reactor_config cfg);
    static void create_thread(std::function<void ()> thread_loop);
public:
    static unsigned count;
};

}


namespace seastar {

class kernel_completion {
protected:
    ~kernel_completion() = default;
public:
    virtual void complete_with(ssize_t res) = 0;
};
}

namespace seastar {
namespace internal {

class io_request {
public:
    enum class operation { read, readv, write, writev, fdatasync, recv, recvmsg, send, sendmsg, accept, connect, poll_add, poll_remove, cancel };
private:
    operation _op;
    int _fd;
    union {
        uint64_t pos;
        int flags;
        int events;
    } _attr;
    // the upper layers give us void pointers, but storing void pointers here is just
    // dangerous. The constructors seem to be happy to convert other pointers to void*,
    // even if they are marked as explicit, and then you end up losing approximately 3 hours
    // and 15 minutes (hypothetically, of course), trying to chase the weirdest bug.
    // Let's store a char* for safety, and cast it back to void* in the accessor.
    union {
        char* addr;
        ::iovec* iovec;
        ::msghdr* msghdr;
        ::sockaddr* sockaddr;
    } _ptr;

    // accept wants a socklen_t*, connect wants a socklen_t
    union {
        size_t len;
        socklen_t* socklen_ptr;
        socklen_t socklen;
    } _size;
    kernel_completion* _kernel_completion;

    explicit io_request(operation op, int fd, int flags, ::msghdr* msg)
        : _op(op)
        , _fd(fd)
    {
        _attr.flags = flags;
        _ptr.msghdr = msg;
    }

    explicit io_request(operation op, int fd, sockaddr* sa, socklen_t sl)
        : _op(op)
        , _fd(fd)
    {
        _ptr.sockaddr = sa;
        _size.socklen = sl;
    }

    explicit io_request(operation op, int fd, int flags, sockaddr* sa, socklen_t* sl)
        : _op(op)
        , _fd(fd)
    {
        _attr.flags = flags;
        _ptr.sockaddr = sa;
        _size.socklen_ptr = sl;
    }
    explicit io_request(operation op, int fd, uint64_t pos, char* ptr, size_t size)
        : _op(op)
        , _fd(fd)
    {
        _attr.pos = pos;
        _ptr.addr = ptr;
        _size.len = size;
    }

    explicit io_request(operation op, int fd, uint64_t pos, iovec* ptr, size_t size)
        : _op(op)
        , _fd(fd)
    {
        _attr.pos = pos;
        _ptr.iovec = ptr;
        _size.len = size;
    }

    explicit io_request(operation op, int fd)
        : _op(op)
        , _fd(fd)
    {}
    explicit io_request(operation op, int fd, int events)
        : _op(op)
        , _fd(fd)
    {
        _attr.events = events;
    }

    explicit io_request(operation op, int fd, char *ptr)
        : _op(op)
        , _fd(fd)
    {
        _ptr.addr = ptr;
    }
public:
    bool is_read() const {
        switch (_op) {
        case operation::read:
        case operation::readv:
        case operation::recvmsg:
        case operation::recv:
            return true;
        default:
            return false;
        }
    }

    bool is_write() const {
        switch (_op) {
        case operation::write:
        case operation::writev:
        case operation::send:
        case operation::sendmsg:
            return true;
        default:
            return false;
        }
    }

    sstring opname() const;

    operation opcode() const {
        return _op;
    }

    int fd() const {
        return _fd;
    }

    uint64_t pos() const {
        return _attr.pos;
    }

    int flags() const {
        return _attr.flags;
    }

    int events() const {
        return _attr.events;
    }

    void* address() const {
        return reinterpret_cast<void*>(_ptr.addr);
    }

    iovec* iov() const {
        return _ptr.iovec;
    }

    ::sockaddr* posix_sockaddr() const {
        return _ptr.sockaddr;
    }

    ::msghdr* msghdr() const {
        return _ptr.msghdr;
    }

    size_t size() const {
        return _size.len;
    }

    size_t iov_len() const {
        return _size.len;
    }

    socklen_t socklen() const {
        return _size.socklen;
    }

    socklen_t* socklen_ptr() const {
        return _size.socklen_ptr;
    }

    void attach_kernel_completion(kernel_completion* kc) {
        _kernel_completion = kc;
    }

    kernel_completion* get_kernel_completion() const {
        return _kernel_completion;
    }

    static io_request make_read(int fd, uint64_t pos, void* address, size_t size) {
        return io_request(operation::read, fd, pos, reinterpret_cast<char*>(address), size);
    }

    static io_request make_readv(int fd, uint64_t pos, std::vector<iovec>& iov) {
        return io_request(operation::readv, fd, pos, iov.data(), iov.size());
    }

    static io_request make_recv(int fd, void* address, size_t size, int flags) {
        return io_request(operation::recv, fd, flags, reinterpret_cast<char*>(address), size);
    }

    static io_request make_recvmsg(int fd, ::msghdr* msg, int flags) {
        return io_request(operation::recvmsg, fd, flags, msg);
    }

    static io_request make_send(int fd, const void* address, size_t size, int flags) {
        return io_request(operation::send, fd, flags, const_cast<char*>(reinterpret_cast<const char*>(address)), size);
    }

    static io_request make_sendmsg(int fd, ::msghdr* msg, int flags) {
        return io_request(operation::sendmsg, fd, flags, msg);
    }

    static io_request make_write(int fd, uint64_t pos, const void* address, size_t size) {
        return io_request(operation::write, fd, pos, const_cast<char*>(reinterpret_cast<const char*>(address)), size);
    }

    static io_request make_writev(int fd, uint64_t pos, std::vector<iovec>& iov) {
        return io_request(operation::writev, fd, pos, iov.data(), iov.size());
    }

    static io_request make_fdatasync(int fd) {
        return io_request(operation::fdatasync, fd);
    }

    static io_request make_accept(int fd, struct sockaddr* addr, socklen_t* addrlen, int flags) {
        return io_request(operation::accept, fd, flags, addr, addrlen);
    }

    static io_request make_connect(int fd, struct sockaddr* addr, socklen_t addrlen) {
        return io_request(operation::connect, fd, addr, addrlen);
    }

    static io_request make_poll_add(int fd, int events) {
        return io_request(operation::poll_add, fd, events);
    }

    static io_request make_poll_remove(int fd, void *addr) {
        return io_request(operation::poll_remove, fd, reinterpret_cast<char*>(addr));
    }
    static io_request make_cancel(int fd, void *addr) {
        return io_request(operation::cancel, fd, reinterpret_cast<char*>(addr));
    }
};
}
}
#include <vector>
#include <tuple>
#include <boost/intrusive_ptr.hpp>

namespace seastar {

class reactor;
class pollable_fd;
class pollable_fd_state;
class socket_address;

namespace internal {

class buffer_allocator;

}

namespace net {

class packet;

}

class pollable_fd_state;

using pollable_fd_state_ptr = boost::intrusive_ptr<pollable_fd_state>;

class pollable_fd_state {
    unsigned _refs = 0;
public:
    virtual ~pollable_fd_state() {}
    struct speculation {
        int events = 0;
        explicit speculation(int epoll_events_guessed = 0) : events(epoll_events_guessed) {}
    };
    pollable_fd_state(const pollable_fd_state&) = delete;
    void operator=(const pollable_fd_state&) = delete;
    void speculate_epoll(int events) { events_known |= events; }
    file_desc fd;
    bool events_rw = false;   // single consumer for both read and write (accept())
    bool no_more_recv = false; // For udp, there is no shutdown indication from the kernel
    bool no_more_send = false; // For udp, there is no shutdown indication from the kernel
    int events_requested = 0; // wanted by pollin/pollout promises
    int events_epoll = 0;     // installed in epoll
    int events_known = 0;     // returned from epoll

    friend class reactor;
    friend class pollable_fd;

    future<size_t> read_some(char* buffer, size_t size);
    future<size_t> read_some(uint8_t* buffer, size_t size);
    future<size_t> read_some(const std::vector<iovec>& iov);
    future<temporary_buffer<char>> read_some(internal::buffer_allocator* ba);
    future<> write_all(const char* buffer, size_t size);
    future<> write_all(const uint8_t* buffer, size_t size);
    future<size_t> write_some(net::packet& p);
    future<> write_all(net::packet& p);
    future<> readable();
    future<> writeable();
    future<> readable_or_writeable();
    void abort_reader();
    void abort_writer();
    future<std::tuple<pollable_fd, socket_address>> accept();
    future<> connect(socket_address& sa);
    future<size_t> sendmsg(struct msghdr *msg);
    future<size_t> recvmsg(struct msghdr *msg);
    future<size_t> sendto(socket_address addr, const void* buf, size_t len);

protected:
    explicit pollable_fd_state(file_desc fd, speculation speculate = speculation())
        : fd(std::move(fd)), events_known(speculate.events) {}
private:
    void maybe_no_more_recv();
    void maybe_no_more_send();
    void forget(); // called on end-of-life

    friend void intrusive_ptr_add_ref(pollable_fd_state* fd) {
        ++fd->_refs;
    }
    friend void intrusive_ptr_release(pollable_fd_state* fd);
};

class pollable_fd {
public:
    using speculation = pollable_fd_state::speculation;
    pollable_fd(file_desc fd, speculation speculate = speculation());
public:
    future<size_t> read_some(char* buffer, size_t size) {
        return _s->read_some(buffer, size);
    }
    future<size_t> read_some(uint8_t* buffer, size_t size) {
        return _s->read_some(buffer, size);
    }
    future<size_t> read_some(const std::vector<iovec>& iov) {
        return _s->read_some(iov);
    }
    future<temporary_buffer<char>> read_some(internal::buffer_allocator* ba) {
        return _s->read_some(ba);
    }
    future<> write_all(const char* buffer, size_t size) {
        return _s->write_all(buffer, size);
    }
    future<> write_all(const uint8_t* buffer, size_t size) {
        return _s->write_all(buffer, size);
    }
    future<size_t> write_some(net::packet& p) {
        return _s->write_some(p);
    }
    future<> write_all(net::packet& p) {
        return _s->write_all(p);
    }
    future<> readable() {
        return _s->readable();
    }
    future<> writeable() {
        return _s->writeable();
    }
    future<> readable_or_writeable() {
        return _s->readable_or_writeable();
    }
    void abort_reader() {
        return _s->abort_reader();
    }
    void abort_writer() {
        return _s->abort_writer();
    }
    future<std::tuple<pollable_fd, socket_address>> accept() {
        return _s->accept();
    }
    future<> connect(socket_address& sa) {
        return _s->connect(sa);
    }
    future<size_t> sendmsg(struct msghdr *msg) {
        return _s->sendmsg(msg);
    }
    future<size_t> recvmsg(struct msghdr *msg) {
        return _s->recvmsg(msg);
    }
    future<size_t> sendto(socket_address addr, const void* buf, size_t len) {
        return _s->sendto(addr, buf, len);
    }
    file_desc& get_file_desc() const { return _s->fd; }
    void shutdown(int how);
    void close() { _s.reset(); }
protected:
    int get_fd() const { return _s->fd.get(); }
    void maybe_no_more_recv() { return _s->maybe_no_more_recv(); }
    void maybe_no_more_send() { return _s->maybe_no_more_send(); }
    friend class reactor;
    friend class readable_eventfd;
    friend class writeable_eventfd;
    friend class aio_storage_context;
private:
    pollable_fd_state_ptr _s;
};

class writeable_eventfd;

class readable_eventfd {
    pollable_fd _fd;
public:
    explicit readable_eventfd(size_t initial = 0) : _fd(try_create_eventfd(initial)) {}
    readable_eventfd(readable_eventfd&&) = default;
    writeable_eventfd write_side();
    future<size_t> wait();
    int get_write_fd() { return _fd.get_fd(); }
private:
    explicit readable_eventfd(file_desc&& fd) : _fd(std::move(fd)) {}
    static file_desc try_create_eventfd(size_t initial);

    friend class writeable_eventfd;
};

class writeable_eventfd {
    file_desc _fd;
public:
    explicit writeable_eventfd(size_t initial = 0) : _fd(try_create_eventfd(initial)) {}
    writeable_eventfd(writeable_eventfd&&) = default;
    readable_eventfd read_side();
    void signal(size_t nr);
    int get_read_fd() { return _fd.get(); }
private:
    explicit writeable_eventfd(file_desc&& fd) : _fd(std::move(fd)) {}
    static file_desc try_create_eventfd(size_t initial);

    friend class readable_eventfd;
};

}
namespace seastar {

struct pollfn {
    virtual ~pollfn() {}
    // Returns true if work was done (false = idle)
    virtual bool poll() = 0;
    // Checks if work needs to be done, but without actually doing any
    // returns true if works needs to be done (false = idle)
    virtual bool pure_poll() = 0;
    // Tries to enter interrupt mode.
    //
    // If it returns true, then events from this poller will wake
    // a sleeping idle loop, and exit_interrupt_mode() must be called
    // to return to normal polling.
    //
    // If it returns false, the sleeping idle loop may not be entered.
    virtual bool try_enter_interrupt_mode() { return false; }
    virtual void exit_interrupt_mode() {}
};

}

#ifdef HAVE_OSV
#include <osv/sched.hh>
#include <osv/mutex.h>
#include <osv/condvar.h>
#include <osv/newpoll.hh>
#endif

struct _Unwind_Exception;
extern "C" int _Unwind_RaiseException(struct _Unwind_Exception *h);

namespace seastar {

using shard_id = unsigned;

namespace alien {
class message_queue;
}
class reactor;
inline
size_t iovec_len(const std::vector<iovec>& iov)
{
    size_t ret = 0;
    for (auto&& e : iov) {
        ret += e.iov_len;
    }
    return ret;
}

}

namespace std {

template <>
struct hash<::sockaddr_in> {
    size_t operator()(::sockaddr_in a) const {
        return a.sin_port ^ a.sin_addr.s_addr;
    }
};

}

bool operator==(const ::sockaddr_in a, const ::sockaddr_in b);

namespace seastar {

void register_network_stack(sstring name, boost::program_options::options_description opts,
    noncopyable_function<future<std::unique_ptr<network_stack>>(boost::program_options::variables_map opts)> create,
    bool make_default = false);

class thread_pool;
class smp;

class reactor_backend_selector;

class reactor_backend;

namespace internal {

class reactor_stall_sampler;
class cpu_stall_detector;
class buffer_allocator;

}

class kernel_completion;
class io_queue;
class disk_config_params;

class reactor {
    using sched_clock = std::chrono::steady_clock;
private:
    struct task_queue;
    using task_queue_list = circular_buffer_fixed_capacity<task_queue*, max_scheduling_groups()>;
    using pollfn = seastar::pollfn;

    class signal_pollfn;
    class batch_flush_pollfn;
    class smp_pollfn;
    class drain_cross_cpu_freelist_pollfn;
    class lowres_timer_pollfn;
    class manual_timer_pollfn;
    class epoll_pollfn;
    class reap_kernel_completions_pollfn;
    class kernel_submit_work_pollfn;
    class io_queue_submission_pollfn;
    class syscall_pollfn;
    class execution_stage_pollfn;
    friend signal_pollfn;
    friend batch_flush_pollfn;
    friend smp_pollfn;
    friend drain_cross_cpu_freelist_pollfn;
    friend lowres_timer_pollfn;
    friend class manual_clock;
    friend class epoll_pollfn;
    friend class reap_kernel_completions_pollfn;
    friend class kernel_submit_work_pollfn;
    friend class io_queue_submission_pollfn;
    friend class syscall_pollfn;
    friend class execution_stage_pollfn;
    friend class file_data_source_impl; // for fstream statistics
    friend class internal::reactor_stall_sampler;
    friend class preempt_io_context;
    friend struct hrtimer_aio_completion;
    friend struct task_quota_aio_completion;
    friend class reactor_backend_epoll;
    friend class reactor_backend_aio;
    friend class reactor_backend_selector;
    friend class aio_storage_context;
public:
    class poller {
        std::unique_ptr<pollfn> _pollfn;
        class registration_task;
        class deregistration_task;
        registration_task* _registration_task = nullptr;
    public:
        template <typename Func> // signature: bool ()
        static poller simple(Func&& poll) {
            return poller(make_pollfn(std::forward<Func>(poll)));
        }
        poller(std::unique_ptr<pollfn> fn)
                : _pollfn(std::move(fn)) {
            do_register();
        }
        ~poller();
        poller(poller&& x);
        poller& operator=(poller&& x);
        void do_register() noexcept;
        friend class reactor;
    };
    enum class idle_cpu_handler_result {
        no_more_work,
        interrupted_by_higher_priority_task
    };
    using work_waiting_on_reactor = const noncopyable_function<bool()>&;
    using idle_cpu_handler = noncopyable_function<idle_cpu_handler_result(work_waiting_on_reactor)>;

    struct io_stats {
        uint64_t aio_reads = 0;
        uint64_t aio_read_bytes = 0;
        uint64_t aio_writes = 0;
        uint64_t aio_write_bytes = 0;
        uint64_t aio_errors = 0;
        uint64_t fstream_reads = 0;
        uint64_t fstream_read_bytes = 0;
        uint64_t fstream_reads_blocked = 0;
        uint64_t fstream_read_bytes_blocked = 0;
        uint64_t fstream_read_aheads_discarded = 0;
        uint64_t fstream_read_ahead_discarded_bytes = 0;
    };
private:
    reactor_config _cfg;
    file_desc _notify_eventfd;
    file_desc _task_quota_timer;
#ifdef HAVE_OSV
    reactor_backend_osv _backend;
    sched::thread _timer_thread;
    sched::thread *_engine_thread;
    mutable mutex _timer_mutex;
    condvar _timer_cond;
    s64 _timer_due = 0;
#else
    std::unique_ptr<reactor_backend> _backend;
#endif
    sigset_t _active_sigmask; // holds sigmask while sleeping with sig disabled
    std::vector<pollfn*> _pollers;

    static constexpr unsigned max_aio_per_queue = 128;
    static constexpr unsigned max_queues = 8;
    static constexpr unsigned max_aio = max_aio_per_queue * max_queues;
    friend disk_config_params;

    // Not all reactors have IO queues. If the number of IO queues is less than the number of shards,
    // some reactors will talk to foreign io_queues. If this reactor holds a valid IO queue, it will
    // be stored here.
    std::vector<std::unique_ptr<io_queue>> my_io_queues;
    std::unordered_map<dev_t, io_queue*> _io_queues;
    friend io_queue;

    std::vector<noncopyable_function<future<> ()>> _exit_funcs;
    unsigned _id = 0;
    bool _stopping = false;
    bool _stopped = false;
    bool _finished_running_tasks = false;
    condition_variable _stop_requested;
    bool _handle_sigint = true;
    compat::optional<future<std::unique_ptr<network_stack>>> _network_stack_ready;
    int _return = 0;
    promise<> _start_promise;
    semaphore _cpu_started;
    internal::preemption_monitor _preemption_monitor{};
    uint64_t _global_tasks_processed = 0;
    uint64_t _polls = 0;
    std::unique_ptr<internal::cpu_stall_detector> _cpu_stall_detector;

    unsigned _max_task_backlog = 1000;
    timer_set<timer<>, &timer<>::_link> _timers;
    timer_set<timer<>, &timer<>::_link>::timer_list_t _expired_timers;
    timer_set<timer<lowres_clock>, &timer<lowres_clock>::_link> _lowres_timers;
    timer_set<timer<lowres_clock>, &timer<lowres_clock>::_link>::timer_list_t _expired_lowres_timers;
    timer_set<timer<manual_clock>, &timer<manual_clock>::_link> _manual_timers;
    timer_set<timer<manual_clock>, &timer<manual_clock>::_link>::timer_list_t _expired_manual_timers;
    io_stats _io_stats;
    uint64_t _fsyncs = 0;
    uint64_t _cxx_exceptions = 0;
    uint64_t _abandoned_failed_futures = 0;
    struct task_queue {
        explicit task_queue(unsigned id, sstring name, float shares);
        int64_t _vruntime = 0;
        float _shares;
        int64_t _reciprocal_shares_times_2_power_32;
        bool _current = false;
        bool _active = false;
        uint8_t _id;
        sched_clock::duration _runtime = {};
        uint64_t _tasks_processed = 0;
        circular_buffer<task*> _q;
        sstring _name;
        /**
         * This array holds pointers to the scheduling group specific
         * data. The pointer is not use as is but is cast to a reference
         * to the appropriate type that is actually pointed to.
         */
        std::vector<void*> _scheduling_group_specific_vals;
        int64_t to_vruntime(sched_clock::duration runtime) const;
        void set_shares(float shares);
        struct indirect_compare;
        sched_clock::duration _time_spent_on_task_quota_violations = {};
        seastar::metrics::metric_groups _metrics;
        void rename(sstring new_name);
    private:
        void register_stats();
    };

    circular_buffer<internal::io_request> _pending_io;
    boost::container::static_vector<std::unique_ptr<task_queue>, max_scheduling_groups()> _task_queues;
    std::vector<scheduling_group_key_config> _scheduling_group_key_configs;
    int64_t _last_vruntime = 0;
    task_queue_list _active_task_queues;
    task_queue_list _activating_task_queues;
    task_queue* _at_destroy_tasks;
    sched_clock::duration _task_quota;
    /// Handler that will be called when there is no task to execute on cpu.
    /// It represents a low priority work.
    ///
    /// Handler's return value determines whether handler did any actual work. If no work was done then reactor will go
    /// into sleep.
    ///
    /// Handler's argument is a function that returns true if a task which should be executed on cpu appears or false
    /// otherwise. This function should be used by a handler to return early if a task appears.
    idle_cpu_handler _idle_cpu_handler{ [] (work_waiting_on_reactor) {return idle_cpu_handler_result::no_more_work;} };
    std::unique_ptr<network_stack> _network_stack;
    // _lowres_clock_impl will only be created on cpu 0
    std::unique_ptr<lowres_clock_impl> _lowres_clock_impl;
    lowres_clock::time_point _lowres_next_timeout;
    compat::optional<poller> _epoll_poller;
    compat::optional<pollable_fd> _aio_eventfd;
    const bool _reuseport;
    circular_buffer<double> _loads;
    double _load = 0;
    sched_clock::duration _total_idle;
    sched_clock::duration _total_sleep;
    sched_clock::time_point _start_time = sched_clock::now();
    std::chrono::nanoseconds _max_poll_time = calculate_poll_time();
    circular_buffer<output_stream<char>* > _flush_batching;
    std::atomic<bool> _sleeping alignas(seastar::cache_line_size);
    pthread_t _thread_id alignas(seastar::cache_line_size) = pthread_self();
    bool _strict_o_direct = true;
    bool _force_io_getevents_syscall = false;
    bool _bypass_fsync = false;
    bool _have_aio_fsync = false;
    std::atomic<bool> _dying{false};
private:
    static std::chrono::nanoseconds calculate_poll_time();
    static void block_notifier(int);
    void wakeup();
    size_t handle_aio_error(internal::linux_abi::iocb* iocb, int ec);
    bool flush_pending_aio();
    bool flush_tcp_batches();
    bool do_expire_lowres_timers();
    bool do_check_lowres_timers() const;
    void expire_manual_timers();
    void abort_on_error(int ret);
    void start_aio_eventfd_loop();
    void stop_aio_eventfd_loop();
    template <typename T, typename E, typename EnableFunc>
    void complete_timers(T&, E&, EnableFunc&& enable_fn);

    /**
     * Returns TRUE if all pollers allow blocking.
     *
     * @return FALSE if at least one of the blockers requires a non-blocking
     *         execution.
     */
    bool poll_once();
    bool pure_poll_once();
    template <typename Func> // signature: bool ()
    static std::unique_ptr<pollfn> make_pollfn(Func&& func);

public:
    /// Register a user-defined signal handler
    void handle_signal(int signo, noncopyable_function<void ()>&& handler);

private:
    class signals {
    public:
        signals();
        ~signals();

        bool poll_signal();
        bool pure_poll_signal() const;
        void handle_signal(int signo, noncopyable_function<void ()>&& handler);
        void handle_signal_once(int signo, noncopyable_function<void ()>&& handler);
        static void action(int signo, siginfo_t* siginfo, void* ignore);
        static void failed_to_handle(int signo);
    private:
        struct signal_handler {
            signal_handler(int signo, noncopyable_function<void ()>&& handler);
            noncopyable_function<void ()> _handler;
        };
        std::atomic<uint64_t> _pending_signals;
        std::unordered_map<int, signal_handler> _signal_handlers;

        friend void reactor::handle_signal(int, noncopyable_function<void ()>&&);
    };

    signals _signals;
    std::unique_ptr<thread_pool> _thread_pool;
    friend class thread_pool;
    friend class internal::cpu_stall_detector;

    uint64_t pending_task_count() const;
    void run_tasks(task_queue& tq);
    bool have_more_tasks() const;
    bool posix_reuseport_detect();
    void task_quota_timer_thread_fn();
    void run_some_tasks();
    void activate(task_queue& tq);
    void insert_active_task_queue(task_queue* tq);
    void insert_activating_task_queues();
    void account_runtime(task_queue& tq, sched_clock::duration runtime);
    void account_idle(sched_clock::duration idletime);
    void allocate_scheduling_group_specific_data(scheduling_group sg, scheduling_group_key key);
    future<> init_scheduling_group(scheduling_group sg, sstring name, float shares);
    future<> init_new_scheduling_group_key(scheduling_group_key key, scheduling_group_key_config cfg);
    future<> destroy_scheduling_group(scheduling_group sg);
    [[noreturn]] void no_such_scheduling_group(scheduling_group sg);
    void* get_scheduling_group_specific_value(scheduling_group sg, scheduling_group_key key) {
        if (!_task_queues[sg._id]) {
            no_such_scheduling_group(sg);
        }
        return _task_queues[sg._id]->_scheduling_group_specific_vals[key.id()];
    }
    void* get_scheduling_group_specific_value(scheduling_group_key key) {
        return get_scheduling_group_specific_value(*internal::current_scheduling_group_ptr(), key);
    }
    uint64_t tasks_processed() const;
    uint64_t min_vruntime() const;
    void request_preemption();
    void start_handling_signal();
    void reset_preemption_monitor();
    void service_highres_timer();

    future<std::tuple<pollable_fd, socket_address>>
    do_accept(pollable_fd_state& listen_fd);
    future<> do_connect(pollable_fd_state& pfd, socket_address& sa);

    future<size_t>
    do_read_some(pollable_fd_state& fd, void* buffer, size_t size);
    future<size_t>
    do_read_some(pollable_fd_state& fd, const std::vector<iovec>& iov);
    future<temporary_buffer<char>>
    do_read_some(pollable_fd_state& fd, internal::buffer_allocator* ba);

    future<size_t>
    do_write_some(pollable_fd_state& fd, const void* buffer, size_t size);
    future<size_t>
    do_write_some(pollable_fd_state& fd, net::packet& p);
public:
    static boost::program_options::options_description get_options_description(reactor_config cfg);
    explicit reactor(unsigned id, reactor_backend_selector rbs, reactor_config cfg);
    reactor(const reactor&) = delete;
    ~reactor();
    void operator=(const reactor&) = delete;

    sched_clock::duration uptime() {
        return sched_clock::now() - _start_time;
    }

    io_queue& get_io_queue(dev_t devid = 0) {
        auto queue = _io_queues.find(devid);
        if (queue == _io_queues.end()) {
            return *_io_queues[0];
        } else {
            return *(queue->second);
        }
    }

    io_priority_class register_one_priority_class(sstring name, uint32_t shares);

    /// \brief Updates the current amount of shares for a given priority class
    ///
    /// This can involve a cross-shard call if the I/O Queue that is responsible for
    /// this class lives in a foreign shard.
    ///
    /// \param pc the priority class handle
    /// \param shares the new shares value
    /// \return a future that is ready when the share update is applied
    future<> update_shares_for_class(io_priority_class pc, uint32_t shares);
    static future<> rename_priority_class(io_priority_class pc, sstring new_name);

    void configure(boost::program_options::variables_map config);

    server_socket listen(socket_address sa, listen_options opts = {});

    future<connected_socket> connect(socket_address sa);
    future<connected_socket> connect(socket_address, socket_address, transport proto = transport::TCP);

    pollable_fd posix_listen(socket_address sa, listen_options opts = {});

    bool posix_reuseport_available() const { return _reuseport; }

    lw_shared_ptr<pollable_fd> make_pollable_fd(socket_address sa, int proto);

    future<> posix_connect(lw_shared_ptr<pollable_fd> pfd, socket_address sa, socket_address local);

    future<> write_all(pollable_fd_state& fd, const void* buffer, size_t size);

    future<file> open_file_dma(sstring name, open_flags flags, file_open_options options = {});
    future<file> open_directory(sstring name);
    future<> make_directory(sstring name, file_permissions permissions = file_permissions::default_dir_permissions);
    future<> touch_directory(sstring name, file_permissions permissions = file_permissions::default_dir_permissions);
    future<compat::optional<directory_entry_type>>  file_type(sstring name, follow_symlink = follow_symlink::yes);
    future<stat_data> file_stat(sstring pathname, follow_symlink);
    future<uint64_t> file_size(sstring pathname);
    future<bool> file_accessible(sstring pathname, access_flags flags);
    future<bool> file_exists(sstring pathname) {
        return file_accessible(pathname, access_flags::exists);
    }
    future<fs_type> file_system_at(sstring pathname);
    future<struct statvfs> statvfs(sstring pathname);
    future<> remove_file(sstring pathname);
    future<> rename_file(sstring old_pathname, sstring new_pathname);
    future<> link_file(sstring oldpath, sstring newpath);
    future<> chmod(sstring name, file_permissions permissions);

    // In the following three methods, prepare_io is not guaranteed to execute in the same processor
    // in which it was generated. Therefore, care must be taken to avoid the use of objects that could
    // be destroyed within or at exit of prepare_io.
    void submit_io(kernel_completion* desc, internal::io_request req);
    future<size_t> submit_io_read(io_queue* ioq,
            const io_priority_class& priority_class,
            size_t len,
            internal::io_request req);
    future<size_t> submit_io_write(io_queue* ioq,
            const io_priority_class& priority_class,
            size_t len,
            internal::io_request req);

    inline void handle_io_result(ssize_t res) {
        if (res < 0) {
            ++_io_stats.aio_errors;
            throw_kernel_error(res);
        }
    }

    int run();
    void exit(int ret);
    future<> when_started() { return _start_promise.get_future(); }
    // The function waits for timeout period for reactor stop notification
    // which happens on termination signals or call for exit().
    template <typename Rep, typename Period>
    future<> wait_for_stop(std::chrono::duration<Rep, Period> timeout) {
        return _stop_requested.wait(timeout, [this] { return _stopping; });
    }

    void at_exit(noncopyable_function<future<> ()> func);

    template <typename Func>
    void at_destroy(Func&& func) {
        _at_destroy_tasks->_q.push_back(make_task(default_scheduling_group(), std::forward<Func>(func)));
    }

#ifdef SEASTAR_SHUFFLE_TASK_QUEUE
    void shuffle(task*&, task_queue&);
#endif

    void add_task(task* t) noexcept {
        auto sg = t->group();
        auto* q = _task_queues[sg._id].get();
        bool was_empty = q->_q.empty();
        q->_q.push_back(std::move(t));
#ifdef SEASTAR_SHUFFLE_TASK_QUEUE
        shuffle(q->_q.back(), *q);
#endif
        if (was_empty) {
            activate(*q);
        }
    }
    void add_urgent_task(task* t) noexcept {
        auto sg = t->group();
        auto* q = _task_queues[sg._id].get();
        bool was_empty = q->_q.empty();
        q->_q.push_front(std::move(t));
#ifdef SEASTAR_SHUFFLE_TASK_QUEUE
        shuffle(q->_q.front(), *q);
#endif
        if (was_empty) {
            activate(*q);
        }
    }

    /// Set a handler that will be called when there is no task to execute on cpu.
    /// Handler should do a low priority work.
    ///
    /// Handler's return value determines whether handler did any actual work. If no work was done then reactor will go
    /// into sleep.
    ///
    /// Handler's argument is a function that returns true if a task which should be executed on cpu appears or false
    /// otherwise. This function should be used by a handler to return early if a task appears.
    void set_idle_cpu_handler(idle_cpu_handler&& handler) {
        _idle_cpu_handler = std::move(handler);
    }
    void force_poll();

    void add_high_priority_task(task*) noexcept;

    network_stack& net() { return *_network_stack; }
    shard_id cpu_id() const { return _id; }

    void sleep();

    steady_clock_type::duration total_idle_time();
    steady_clock_type::duration total_busy_time();
    std::chrono::nanoseconds total_steal_time();

    const io_stats& get_io_stats() const { return _io_stats; }
    uint64_t abandoned_failed_futures() const { return _abandoned_failed_futures; }
#ifdef HAVE_OSV
    void timer_thread_func();
    void set_timer(sched::timer &tmr, s64 t);
#endif
private:
    /**
     * Add a new "poller" - a non-blocking function returning a boolean, that
     * will be called every iteration of a main loop.
     * If it returns FALSE then reactor's main loop is forbidden to block in the
     * current iteration.
     *
     * @param fn a new "poller" function to register
     */
    void register_poller(pollfn* p);
    void unregister_poller(pollfn* p);
    void replace_poller(pollfn* old, pollfn* neww);
    void register_metrics();
    future<> write_all_part(pollable_fd_state& fd, const void* buffer, size_t size, size_t completed);

    future<> fdatasync(int fd);

    void add_timer(timer<steady_clock_type>*);
    bool queue_timer(timer<steady_clock_type>*);
    void del_timer(timer<steady_clock_type>*);
    void add_timer(timer<lowres_clock>*);
    bool queue_timer(timer<lowres_clock>*);
    void del_timer(timer<lowres_clock>*);
    void add_timer(timer<manual_clock>*);
    bool queue_timer(timer<manual_clock>*);
    void del_timer(timer<manual_clock>*);

    future<> run_exit_tasks();
    void stop();
    friend class alien::message_queue;
    friend class pollable_fd;
    friend class pollable_fd_state;
    friend struct pollable_fd_state_deleter;
    friend class posix_file_impl;
    friend class blockdev_file_impl;
    friend class readable_eventfd;
    friend class timer<>;
    friend class timer<lowres_clock>;
    friend class timer<manual_clock>;
    friend class smp;
    friend class smp_message_queue;
    friend class poller;
    friend class scheduling_group;
    friend void add_to_flush_poller(output_stream<char>* os);
    friend int ::_Unwind_RaiseException(struct _Unwind_Exception *h);
    friend void report_failed_future(const std::exception_ptr& eptr) noexcept;
    metrics::metric_groups _metric_groups;
    friend future<scheduling_group> create_scheduling_group(sstring name, float shares);
    friend future<> seastar::destroy_scheduling_group(scheduling_group);
    friend future<> seastar::rename_scheduling_group(scheduling_group sg, sstring new_name);
    friend future<scheduling_group_key> scheduling_group_key_create(scheduling_group_key_config cfg);

    template<typename T>
    friend T& scheduling_group_get_specific(scheduling_group sg, scheduling_group_key key);
    template<typename T>
    friend T& scheduling_group_get_specific(scheduling_group_key key);
    template<typename SpecificValType, typename Mapper, typename Reducer, typename Initial>
        GCC6_CONCEPT( requires requires(SpecificValType specific_val, Mapper mapper, Reducer reducer, Initial initial) {
            {reducer(initial, mapper(specific_val))} -> Initial;
        })
    friend future<typename function_traits<Reducer>::return_type>
    map_reduce_scheduling_group_specific(Mapper mapper, Reducer reducer, Initial initial_val, scheduling_group_key key);
    template<typename SpecificValType, typename Reducer, typename Initial>
    GCC6_CONCEPT( requires requires(SpecificValType specific_val, Reducer reducer, Initial initial) {
        {reducer(initial, specific_val)} -> Initial;
    })
    friend future<typename function_traits<Reducer>::return_type>
        reduce_scheduling_group_specific(Reducer reducer, Initial initial_val, scheduling_group_key key);
public:
    future<> readable(pollable_fd_state& fd);
    future<> writeable(pollable_fd_state& fd);
    future<> readable_or_writeable(pollable_fd_state& fd);
    void abort_reader(pollable_fd_state& fd);
    void abort_writer(pollable_fd_state& fd);
    void enable_timer(steady_clock_type::time_point when);
    /// Sets the "Strict DMA" flag.
    ///
    /// When true (default), file I/O operations must use DMA.  This is
    /// the most performant option, but does not work on some file systems
    /// such as tmpfs or aufs (used in some Docker setups).
    ///
    /// When false, file I/O operations can fall back to buffered I/O if
    /// DMA is not available.  This can result in dramatic reducation in
    /// performance and an increase in memory consumption.
    void set_strict_dma(bool value);
    void set_bypass_fsync(bool value);
    void update_blocked_reactor_notify_ms(std::chrono::milliseconds ms);
    std::chrono::milliseconds get_blocked_reactor_notify_ms() const;
    // For testing:
    void set_stall_detector_report_function(std::function<void ()> report);
    std::function<void ()> get_stall_detector_report_function() const;
};

template <typename Func> // signature: bool ()
inline
std::unique_ptr<reactor::pollfn>
reactor::make_pollfn(Func&& func) {
    struct the_pollfn : pollfn {
        the_pollfn(Func&& func) : func(std::forward<Func>(func)) {}
        Func func;
        virtual bool poll() override final {
            return func();
        }
        virtual bool pure_poll() override final {
            return poll(); // dubious, but compatible
        }
    };
    return std::make_unique<the_pollfn>(std::forward<Func>(func));
}

extern __thread reactor* local_engine;
extern __thread size_t task_quota;

inline reactor& engine() {
    return *local_engine;
}

inline bool engine_is_ready() {
    return local_engine != nullptr;
}

inline
size_t iovec_len(const iovec* begin, size_t len)
{
    size_t ret = 0;
    auto end = begin + len;
    while (begin != end) {
        ret += begin++->iov_len;
    }
    return ret;
}

inline int hrtimer_signal() {
    // We don't want to use SIGALRM, because the boost unit test library
    // also plays with it.
    return SIGRTMIN;
}


extern logger seastar_logger;

}


namespace seastar {

/// Asynchronous single-producer single-consumer queue with limited capacity.
/// There can be at most one producer-side and at most one consumer-side operation active at any time.
/// Operations returning a future are considered to be active until the future resolves.
template <typename T>
class queue {
    std::queue<T, circular_buffer<T>> _q;
    size_t _max;
    compat::optional<promise<>> _not_empty;
    compat::optional<promise<>> _not_full;
    std::exception_ptr _ex = nullptr;
private:
    void notify_not_empty();
    void notify_not_full();
public:
    explicit queue(size_t size);

    /// \brief Push an item.
    ///
    /// Returns false if the queue was full and the item was not pushed.
    bool push(T&& a);

    /// \brief Pop an item.
    ///
    /// Popping from an empty queue will result in undefined behavior.
    T pop();

    /// Consumes items from the queue, passing them to @func, until @func
    /// returns false or the queue it empty
    ///
    /// Returns false if func returned false.
    template <typename Func>
    bool consume(Func&& func);

    /// Returns true when the queue is empty.
    bool empty() const;

    /// Returns true when the queue is full.
    bool full() const;

    /// Returns a future<> that becomes available when pop() or consume()
    /// can be called.
    /// A consumer-side operation. Cannot be called concurrently with other consumer-side operations.
    future<> not_empty();

    /// Returns a future<> that becomes available when push() can be called.
    /// A producer-side operation. Cannot be called concurrently with other producer-side operations.
    future<> not_full();

    /// Pops element now or when there is some. Returns a future that becomes
    /// available when some element is available.
    /// If the queue is, or already was, abort()ed, the future resolves with
    /// the exception provided to abort().
    /// A consumer-side operation. Cannot be called concurrently with other consumer-side operations.
    future<T> pop_eventually();

    /// Pushes the element now or when there is room. Returns a future<> which
    /// resolves when data was pushed.
    /// If the queue is, or already was, abort()ed, the future resolves with
    /// the exception provided to abort().
    /// A producer-side operation. Cannot be called concurrently with other producer-side operations.
    future<> push_eventually(T&& data);

    /// Returns the number of items currently in the queue.
    size_t size() const { return _q.size(); }

    /// Returns the size limit imposed on the queue during its construction
    /// or by a call to set_max_size(). If the queue contains max_size()
    /// items (or more), further items cannot be pushed until some are popped.
    size_t max_size() const { return _max; }

    /// Set the maximum size to a new value. If the queue's max size is reduced,
    /// items already in the queue will not be expunged and the queue will be temporarily
    /// bigger than its max_size.
    void set_max_size(size_t max) {
        _max = max;
        if (!full()) {
            notify_not_full();
        }
    }

    /// Destroy any items in the queue, and pass the provided exception to any
    /// waiting readers or writers - or to any later read or write attempts.
    void abort(std::exception_ptr ex) {
        while (!_q.empty()) {
            _q.pop();
        }
        _ex = ex;
        if (_not_full) {
            _not_full->set_exception(ex);
            _not_full= compat::nullopt;
        }
        if (_not_empty) {
            _not_empty->set_exception(std::move(ex));
            _not_empty = compat::nullopt;
        }
    }

    /// \brief Check if there is an active consumer
    ///
    /// Returns true if another fiber waits for an item to be pushed into the queue
    bool has_blocked_consumer() const {
        return bool(_not_empty);
    }
};

template <typename T>
inline
queue<T>::queue(size_t size)
    : _max(size) {
}

template <typename T>
inline
void queue<T>::notify_not_empty() {
    if (_not_empty) {
        _not_empty->set_value();
        _not_empty = compat::optional<promise<>>();
    }
}

template <typename T>
inline
void queue<T>::notify_not_full() {
    if (_not_full) {
        _not_full->set_value();
        _not_full = compat::optional<promise<>>();
    }
}

template <typename T>
inline
bool queue<T>::push(T&& data) {
    if (_q.size() < _max) {
        _q.push(std::move(data));
        notify_not_empty();
        return true;
    } else {
        return false;
    }
}

template <typename T>
inline
T queue<T>::pop() {
    if (_q.size() == _max) {
        notify_not_full();
    }
    T data = std::move(_q.front());
    _q.pop();
    return data;
}

template <typename T>
inline
future<T> queue<T>::pop_eventually() {
    if (_ex) {
        return make_exception_future<T>(_ex);
    }
    if (empty()) {
        return not_empty().then([this] {
            if (_ex) {
                return make_exception_future<T>(_ex);
            } else {
                return make_ready_future<T>(pop());
            }
        });
    } else {
        return make_ready_future<T>(pop());
    }
}

template <typename T>
inline
future<> queue<T>::push_eventually(T&& data) {
    if (_ex) {
        return make_exception_future<>(_ex);
    }
    if (full()) {
        return not_full().then([this, data = std::move(data)] () mutable {
            _q.push(std::move(data));
            notify_not_empty();
        });
    } else {
        _q.push(std::move(data));
        notify_not_empty();
        return make_ready_future<>();
    }
}

template <typename T>
template <typename Func>
inline
bool queue<T>::consume(Func&& func) {
    if (_ex) {
        std::rethrow_exception(_ex);
    }
    bool running = true;
    while (!_q.empty() && running) {
        running = func(std::move(_q.front()));
        _q.pop();
    }
    if (!full()) {
        notify_not_full();
    }
    return running;
}

template <typename T>
inline
bool queue<T>::empty() const {
    return _q.empty();
}

template <typename T>
inline
bool queue<T>::full() const {
    return _q.size() >= _max;
}

template <typename T>
inline
future<> queue<T>::not_empty() {
    if (_ex) {
        return make_exception_future<>(_ex);
    }
    if (!empty()) {
        return make_ready_future<>();
    } else {
        _not_empty = promise<>();
        return _not_empty->get_future();
    }
}

template <typename T>
inline
future<> queue<T>::not_full() {
    if (_ex) {
        return make_exception_future<>(_ex);
    }
    if (!full()) {
        return make_ready_future<>();
    } else {
        _not_full = promise<>();
        return _not_full->get_future();
    }
}

}

#include <vector>

namespace seastar {

using rss_key_type = compat::basic_string_view<uint8_t>;

// Mellanox Linux's driver key
static constexpr uint8_t default_rsskey_40bytes_v[] = {
    0xd1, 0x81, 0xc6, 0x2c, 0xf7, 0xf4, 0xdb, 0x5b,
    0x19, 0x83, 0xa2, 0xfc, 0x94, 0x3e, 0x1a, 0xdb,
    0xd9, 0x38, 0x9e, 0x6b, 0xd1, 0x03, 0x9c, 0x2c,
    0xa7, 0x44, 0x99, 0xad, 0x59, 0x3d, 0x56, 0xd9,
    0xf3, 0x25, 0x3c, 0x06, 0x2a, 0xdc, 0x1f, 0xfc
};

static constexpr rss_key_type default_rsskey_40bytes{default_rsskey_40bytes_v, sizeof(default_rsskey_40bytes_v)};

// Intel's i40e PMD default RSS key
static constexpr uint8_t default_rsskey_52bytes_v[] = {
    0x44, 0x39, 0x79, 0x6b, 0xb5, 0x4c, 0x50, 0x23,
    0xb6, 0x75, 0xea, 0x5b, 0x12, 0x4f, 0x9f, 0x30,
    0xb8, 0xa2, 0xc0, 0x3d, 0xdf, 0xdc, 0x4d, 0x02,
    0xa0, 0x8c, 0x9b, 0x33, 0x4a, 0xf6, 0x4a, 0x4c,
    0x05, 0xc6, 0xfa, 0x34, 0x39, 0x58, 0xd8, 0x55,
    0x7d, 0x99, 0x58, 0x3a, 0xe1, 0x38, 0xc9, 0x2e,
    0x81, 0x15, 0x03, 0x66
};

static constexpr rss_key_type default_rsskey_52bytes{default_rsskey_52bytes_v, sizeof(default_rsskey_52bytes_v)};

template<typename T>
static inline uint32_t
toeplitz_hash(rss_key_type key, const T& data)
{
        uint32_t hash = 0, v;
        u_int i, b;

        /* XXXRW: Perhaps an assertion about key length vs. data length? */

        v = (key[0]<<24) + (key[1]<<16) + (key[2] <<8) + key[3];
        for (i = 0; i < data.size(); i++) {
                for (b = 0; b < 8; b++) {
                        if (data[i] & (1<<(7-b)))
                                hash ^= v;
                        v <<= 1;
                        if ((i + 4) < key.size() &&
                            (key[i+4] & (1<<(7-b))))
                                v |= 1;
                }
        }
        return (hash);
}

}

namespace seastar {

namespace net {

struct ethernet_address {
    ethernet_address()
        : mac{} {}

    ethernet_address(const uint8_t *eaddr) {
        std::copy(eaddr, eaddr + 6, mac.begin());
    }

    ethernet_address(std::initializer_list<uint8_t> eaddr) {
        assert(eaddr.size() == mac.size());
        std::copy(eaddr.begin(), eaddr.end(), mac.begin());
    }

    std::array<uint8_t, 6> mac;

    template <typename Adjuster>
    void adjust_endianness(Adjuster a) {}

    static ethernet_address read(const char* p) {
        ethernet_address ea;
        std::copy_n(p, size(), reinterpret_cast<char*>(ea.mac.data()));\
        return ea;
    }
    static ethernet_address consume(const char*& p) {
        auto ea = read(p);
        p += size();
        return ea;
    }
    void write(char* p) const {
        std::copy_n(reinterpret_cast<const char*>(mac.data()), size(), p);
    }
    void produce(char*& p) const {
        write(p);
        p += size();
    }
    static constexpr size_t size() {
        return 6;
    }
} __attribute__((packed));

std::ostream& operator<<(std::ostream& os, ethernet_address ea);

struct ethernet {
    using address = ethernet_address;
    static address broadcast_address() {
        return  {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    }
    static constexpr uint16_t arp_hardware_type() { return 1; }
};

struct eth_hdr {
    ethernet_address dst_mac;
    ethernet_address src_mac;
    packed<uint16_t> eth_proto;
    template <typename Adjuster>
    auto adjust_endianness(Adjuster a) {
        return a(eth_proto);
    }
} __attribute__((packed));

ethernet_address parse_ethernet_address(std::string addr);
}

}
#include <unordered_map>

namespace seastar {

namespace net {

class packet;
class interface;
class device;
class qp;
class l3_protocol;

class forward_hash {
    uint8_t data[64];
    size_t end_idx = 0;
public:
    size_t size() const {
        return end_idx;
    }
    void push_back(uint8_t b) {
        assert(end_idx < sizeof(data));
        data[end_idx++] = b;
    }
    void push_back(uint16_t b) {
        push_back(uint8_t(b));
        push_back(uint8_t(b >> 8));
    }
    void push_back(uint32_t b) {
        push_back(uint16_t(b));
        push_back(uint16_t(b >> 16));
    }
    const uint8_t& operator[](size_t idx) const {
        return data[idx];
    }
};

struct hw_features {
    // Enable tx ip header checksum offload
    bool tx_csum_ip_offload = false;
    // Enable tx l4 (TCP or UDP) checksum offload
    bool tx_csum_l4_offload = false;
    // Enable rx checksum offload
    bool rx_csum_offload = false;
    // LRO is enabled
    bool rx_lro = false;
    // Enable tx TCP segment offload
    bool tx_tso = false;
    // Enable tx UDP fragmentation offload
    bool tx_ufo = false;
    // Maximum Transmission Unit
    uint16_t mtu = 1500;
    // Maximun packet len when TCP/UDP offload is enabled
    uint16_t max_packet_len = ip_packet_len_max - eth_hdr_len;
};

class l3_protocol {
public:
    struct l3packet {
        eth_protocol_num proto_num;
        ethernet_address to;
        packet p;
    };
    using packet_provider_type = std::function<compat::optional<l3packet> ()>;
private:
    interface* _netif;
    eth_protocol_num _proto_num;
public:
    explicit l3_protocol(interface* netif, eth_protocol_num proto_num, packet_provider_type func);
    future<> receive(
            std::function<future<> (packet, ethernet_address)> rx_fn,
            std::function<bool (forward_hash&, packet&, size_t)> forward);
private:
    friend class interface;
};

class interface {
    struct l3_rx_stream {
        stream<packet, ethernet_address> packet_stream;
        future<> ready;
        std::function<bool (forward_hash&, packet&, size_t)> forward;
        l3_rx_stream(std::function<bool (forward_hash&, packet&, size_t)>&& fw) : ready(packet_stream.started()), forward(fw) {}
    };
    std::unordered_map<uint16_t, l3_rx_stream> _proto_map;
    std::shared_ptr<device> _dev;
    ethernet_address _hw_address;
    net::hw_features _hw_features;
    std::vector<l3_protocol::packet_provider_type> _pkt_providers;
private:
    future<> dispatch_packet(packet p);
public:
    explicit interface(std::shared_ptr<device> dev);
    ethernet_address hw_address() { return _hw_address; }
    const net::hw_features& hw_features() const { return _hw_features; }
    future<> register_l3(eth_protocol_num proto_num,
            std::function<future<> (packet p, ethernet_address from)> next,
            std::function<bool (forward_hash&, packet&, size_t)> forward);
    void forward(unsigned cpuid, packet p);
    unsigned hash2cpu(uint32_t hash);
    void register_packet_provider(l3_protocol::packet_provider_type func) {
        _pkt_providers.push_back(std::move(func));
    }
    uint16_t hw_queues_count();
    rss_key_type rss_key() const;
    friend class l3_protocol;
};

struct qp_stats_good {
    /**
     * Update the packets bunch related statistics.
     *
     * Update the last packets bunch size and the total packets counter.
     *
     * @param count Number of packets in the last packets bunch.
     */
    void update_pkts_bunch(uint64_t count) {
        last_bunch = count;
        packets   += count;
    }

    /**
     * Increment the appropriate counters when a few fragments have been
     * processed in a copy-way.
     *
     * @param nr_frags Number of copied fragments
     * @param bytes    Number of copied bytes
     */
    void update_copy_stats(uint64_t nr_frags, uint64_t bytes) {
        copy_frags += nr_frags;
        copy_bytes += bytes;
    }

    /**
     * Increment total fragments and bytes statistics
     *
     * @param nfrags Number of processed fragments
     * @param nbytes Number of bytes in the processed fragments
     */
    void update_frags_stats(uint64_t nfrags, uint64_t nbytes) {
        nr_frags += nfrags;
        bytes    += nbytes;
    }

    uint64_t bytes;      // total number of bytes
    uint64_t nr_frags;   // total number of fragments
    uint64_t copy_frags; // fragments that were copied on L2 level
    uint64_t copy_bytes; // bytes that were copied on L2 level
    uint64_t packets;    // total number of packets
    uint64_t last_bunch; // number of packets in the last sent/received bunch
};

struct qp_stats {
    qp_stats() : rx{}, tx{} {}

    struct {
        struct qp_stats_good good;

        struct {
            void inc_csum_err() {
                ++csum;
                ++total;
            }

            void inc_no_mem() {
                ++no_mem;
                ++total;
            }

            uint64_t no_mem;       // Packets dropped due to allocation failure
            uint64_t total;        // total number of erroneous packets
            uint64_t csum;         // packets with bad checksum
        } bad;
    } rx;

    struct {
        struct qp_stats_good good;
        uint64_t linearized;       // number of packets that were linearized
    } tx;
};

class qp {
    using packet_provider_type = std::function<compat::optional<packet> ()>;
    std::vector<packet_provider_type> _pkt_providers;
    compat::optional<std::array<uint8_t, 128>> _sw_reta;
    circular_buffer<packet> _proxy_packetq;
    stream<packet> _rx_stream;
    reactor::poller _tx_poller;
    circular_buffer<packet> _tx_packetq;

protected:
    const std::string _stats_plugin_name;
    const std::string _queue_name;
    metrics::metric_groups _metrics;
    qp_stats _stats;

public:
    qp(bool register_copy_stats = false,
       const std::string stats_plugin_name = std::string("network"),
       uint8_t qid = 0);
    virtual ~qp();
    virtual future<> send(packet p) = 0;
    virtual uint32_t send(circular_buffer<packet>& p) {
        uint32_t sent = 0;
        while (!p.empty()) {
            // FIXME: future is discarded
            (void)send(std::move(p.front()));
            p.pop_front();
            sent++;
        }
        return sent;
    }
    virtual void rx_start() {};
    void configure_proxies(const std::map<unsigned, float>& cpu_weights);
    // build REdirection TAble for cpu_weights map: target cpu -> weight
    void build_sw_reta(const std::map<unsigned, float>& cpu_weights);
    void proxy_send(packet p) {
        _proxy_packetq.push_back(std::move(p));
    }
    void register_packet_provider(packet_provider_type func) {
        _pkt_providers.push_back(std::move(func));
    }
    bool poll_tx();
    friend class device;
};

class device {
protected:
    std::unique_ptr<qp*[]> _queues;
    size_t _rss_table_bits = 0;
public:
    device() {
        _queues = std::make_unique<qp*[]>(smp::count);
    }
    virtual ~device() {};
    qp& queue_for_cpu(unsigned cpu) { return *_queues[cpu]; }
    qp& local_queue() { return queue_for_cpu(engine().cpu_id()); }
    void l2receive(packet p) {
        // FIXME: future is discarded
        (void)_queues[engine().cpu_id()]->_rx_stream.produce(std::move(p));
    }
    future<> receive(std::function<future<> (packet)> next_packet);
    virtual ethernet_address hw_address() = 0;
    virtual net::hw_features hw_features() = 0;
    virtual rss_key_type rss_key() const { return default_rsskey_40bytes; }
    virtual uint16_t hw_queues_count() { return 1; }
    virtual future<> link_ready() { return make_ready_future<>(); }
    virtual std::unique_ptr<qp> init_local_queue(boost::program_options::variables_map opts, uint16_t qid) = 0;
    virtual unsigned hash2qid(uint32_t hash) {
        return hash % hw_queues_count();
    }
    void set_local_queue(std::unique_ptr<qp> dev);
    template <typename Func>
    unsigned forward_dst(unsigned src_cpuid, Func&& hashfn) {
        auto& qp = queue_for_cpu(src_cpuid);
        if (!qp._sw_reta) {
            return src_cpuid;
        }
        auto hash = hashfn() >> _rss_table_bits;
        auto& reta = *qp._sw_reta;
        return reta[hash % reta.size()];
    }
    virtual unsigned hash2cpu(uint32_t hash) {
        // there is an assumption here that qid == cpu_id which will
        // not necessary be true in the future
        return forward_dst(hash2qid(hash), [hash] { return hash; });
    }
};

}

}
#include <unordered_map>

namespace seastar {

namespace net {

class arp;
class arp_for_protocol;
template <typename L3>
class arp_for;

class arp_for_protocol {
protected:
    arp& _arp;
    uint16_t _proto_num;
public:
    arp_for_protocol(arp& a, uint16_t proto_num);
    virtual ~arp_for_protocol();
    virtual future<> received(packet p) = 0;
    virtual bool forward(forward_hash& out_hash_data, packet& p, size_t off) { return false; }
};

class arp {
    interface* _netif;
    l3_protocol _proto;
    std::unordered_map<uint16_t, arp_for_protocol*> _arp_for_protocol;
    circular_buffer<l3_protocol::l3packet> _packetq;
private:
    struct arp_hdr {
        uint16_t htype;
        uint16_t ptype;

        static arp_hdr read(const char* p) {
            arp_hdr ah;
            ah.htype = consume_be<uint16_t>(p);
            ah.ptype = consume_be<uint16_t>(p);
            return ah;
        }
        static constexpr size_t size() { return 4; }
    };
public:
    explicit arp(interface* netif);
    void add(uint16_t proto_num, arp_for_protocol* afp);
    void del(uint16_t proto_num);
private:
    ethernet_address l2self() { return _netif->hw_address(); }
    future<> process_packet(packet p, ethernet_address from);
    bool forward(forward_hash& out_hash_data, packet& p, size_t off);
    compat::optional<l3_protocol::l3packet> get_packet();
    template <class l3_proto>
    friend class arp_for;
};

template <typename L3>
class arp_for : public arp_for_protocol {
public:
    using l2addr = ethernet_address;
    using l3addr = typename L3::address_type;
private:
    static constexpr auto max_waiters = 512;
    enum oper {
        op_request = 1,
        op_reply = 2,
    };
    struct arp_hdr {
        uint16_t htype;
        uint16_t ptype;
        uint8_t hlen;
        uint8_t plen;
        uint16_t oper;
        l2addr sender_hwaddr;
        l3addr sender_paddr;
        l2addr target_hwaddr;
        l3addr target_paddr;

        static arp_hdr read(const char* p) {
            arp_hdr ah;
            ah.htype = consume_be<uint16_t>(p);
            ah.ptype = consume_be<uint16_t>(p);
            ah.hlen = consume_be<uint8_t>(p);
            ah.plen = consume_be<uint8_t>(p);
            ah.oper = consume_be<uint16_t>(p);
            ah.sender_hwaddr = l2addr::consume(p);
            ah.sender_paddr = l3addr::consume(p);
            ah.target_hwaddr = l2addr::consume(p);
            ah.target_paddr = l3addr::consume(p);
            return ah;
        }
        void write(char* p) const {
            produce_be<uint16_t>(p, htype);
            produce_be<uint16_t>(p, ptype);
            produce_be<uint8_t>(p, hlen);
            produce_be<uint8_t>(p, plen);
            produce_be<uint16_t>(p, oper);
            sender_hwaddr.produce(p);
            sender_paddr.produce(p);
            target_hwaddr.produce(p);
            target_paddr.produce(p);
        }
        static constexpr size_t size() {
            return 8 + 2 * (l2addr::size() + l3addr::size());
        }
    };
    struct resolution {
        std::vector<promise<l2addr>> _waiters;
        timer<> _timeout_timer;
    };
private:
    l3addr _l3self = L3::broadcast_address();
    std::unordered_map<l3addr, l2addr> _table;
    std::unordered_map<l3addr, resolution> _in_progress;
private:
    packet make_query_packet(l3addr paddr);
    virtual future<> received(packet p) override;
    future<> handle_request(arp_hdr* ah);
    l2addr l2self() { return _arp.l2self(); }
    void send(l2addr to, packet p);
public:
    future<> send_query(const l3addr& paddr);
    explicit arp_for(arp& a) : arp_for_protocol(a, L3::arp_protocol_type()) {
        _table[L3::broadcast_address()] = ethernet::broadcast_address();
    }
    future<ethernet_address> lookup(const l3addr& addr);
    void learn(l2addr l2, l3addr l3);
    void run();
    void set_self_addr(l3addr addr) {
        _table.erase(_l3self);
        _table[addr] = l2self();
        _l3self = addr;
    }
    friend class arp;
};

template <typename L3>
packet
arp_for<L3>::make_query_packet(l3addr paddr) {
    arp_hdr hdr;
    hdr.htype = ethernet::arp_hardware_type();
    hdr.ptype = L3::arp_protocol_type();
    hdr.hlen = sizeof(l2addr);
    hdr.plen = sizeof(l3addr);
    hdr.oper = op_request;
    hdr.sender_hwaddr = l2self();
    hdr.sender_paddr = _l3self;
    hdr.target_hwaddr = ethernet::broadcast_address();
    hdr.target_paddr = paddr;
    auto p = packet();
    p.prepend_uninitialized_header(hdr.size());
    hdr.write(p.get_header(0, hdr.size()));
    return p;
}

template <typename L3>
void arp_for<L3>::send(l2addr to, packet p) {
    _arp._packetq.push_back(l3_protocol::l3packet{eth_protocol_num::arp, to, std::move(p)});
}

template <typename L3>
future<>
arp_for<L3>::send_query(const l3addr& paddr) {
    send(ethernet::broadcast_address(), make_query_packet(paddr));
    return make_ready_future<>();
}

class arp_error : public std::runtime_error {
public:
    arp_error(const std::string& msg) : std::runtime_error(msg) {}
};

class arp_timeout_error : public arp_error {
public:
    arp_timeout_error() : arp_error("ARP timeout") {}
};

class arp_queue_full_error : public arp_error {
public:
    arp_queue_full_error() : arp_error("ARP waiter's queue is full") {}
};

template <typename L3>
future<ethernet_address>
arp_for<L3>::lookup(const l3addr& paddr) {
    auto i = _table.find(paddr);
    if (i != _table.end()) {
        return make_ready_future<ethernet_address>(i->second);
    }
    auto j = _in_progress.find(paddr);
    auto first_request = j == _in_progress.end();
    auto& res = first_request ? _in_progress[paddr] : j->second;

    if (first_request) {
        res._timeout_timer.set_callback([paddr, this, &res] {
            // FIXME: future is discarded
            (void)send_query(paddr);
            for (auto& w : res._waiters) {
                w.set_exception(arp_timeout_error());
            }
            res._waiters.clear();
        });
        res._timeout_timer.arm_periodic(std::chrono::seconds(1));
        // FIXME: future is discarded
        (void)send_query(paddr);
    }

    if (res._waiters.size() >= max_waiters) {
        return make_exception_future<ethernet_address>(arp_queue_full_error());
    }

    res._waiters.emplace_back();
    return res._waiters.back().get_future();
}

template <typename L3>
void
arp_for<L3>::learn(l2addr hwaddr, l3addr paddr) {
    _table[paddr] = hwaddr;
    auto i = _in_progress.find(paddr);
    if (i != _in_progress.end()) {
        auto& res = i->second;
        res._timeout_timer.cancel();
        for (auto &&pr : res._waiters) {
            pr.set_value(hwaddr);
        }
        _in_progress.erase(i);
    }
}

template <typename L3>
future<>
arp_for<L3>::received(packet p) {
    auto ah = p.get_header(0, arp_hdr::size());
    if (!ah) {
        return make_ready_future<>();
    }
    auto h = arp_hdr::read(ah);
    if (h.hlen != sizeof(l2addr) || h.plen != sizeof(l3addr)) {
        return make_ready_future<>();
    }
    switch (h.oper) {
    case op_request:
        return handle_request(&h);
    case op_reply:
        arp_learn(h.sender_hwaddr, h.sender_paddr);
        return make_ready_future<>();
    default:
        return make_ready_future<>();
    }
}

template <typename L3>
future<>
arp_for<L3>::handle_request(arp_hdr* ah) {
    if (ah->target_paddr == _l3self
            && _l3self != L3::broadcast_address()) {
        ah->oper = op_reply;
        ah->target_hwaddr = ah->sender_hwaddr;
        ah->target_paddr = ah->sender_paddr;
        ah->sender_hwaddr = l2self();
        ah->sender_paddr = _l3self;
        auto p = packet();
        ah->write(p.prepend_uninitialized_header(ah->size()));
        send(ah->target_hwaddr, std::move(p));
    }
    return make_ready_future<>();
}

}

}

#include <arpa/inet.h>

namespace seastar {

namespace net {

uint16_t ip_checksum(const void* data, size_t len);

struct checksummer {
    __int128 csum = 0;
    bool odd = false;
    void sum(const char* data, size_t len);
    void sum(const packet& p);
    void sum(uint8_t data) {
        if (!odd) {
            csum += data << 8;
        } else {
            csum += data;
        }
        odd = !odd;
    }
    void sum(uint16_t data) {
        if (odd) {
            sum(uint8_t(data >> 8));
            sum(uint8_t(data));
        } else {
            csum += data;
        }
    }
    void sum(uint32_t data) {
        if (odd) {
            sum(uint16_t(data));
            sum(uint16_t(data >> 16));
        } else {
            csum += data;
        }
    }
    void sum_many() {}
    template <typename T0, typename... T>
    void sum_many(T0 data, T... rest) {
        sum(data);
        sum_many(rest...);
    }
    uint16_t get() const;
};

}

}


#include <iostream>

namespace seastar {

namespace net {

template <typename Offset, typename Tag>
class packet_merger {
private:
    static uint64_t& linearizations_ref() {
        static thread_local uint64_t linearization_count;
        return linearization_count;
    }
public:
    std::map<Offset, packet> map;

    static uint64_t linearizations() {
        return linearizations_ref();
    }

    void merge(Offset offset, packet p) {
        bool insert = true;
        auto beg = offset;
        auto end = beg + p.len();
        // Fisrt, try to merge the packet with existing segment
        for (auto it = map.begin(); it != map.end();) {
            auto& seg_pkt = it->second;
            auto seg_beg = it->first;
            auto seg_end = seg_beg + seg_pkt.len();
            // There are 6 cases:
            if (seg_beg <= beg && end <= seg_end) {
                // 1) seg_beg beg end seg_end
                // We already have data in this packet
                return;
            } else if (beg <= seg_beg && seg_end <= end) {
                // 2) beg seg_beg seg_end end
                // The new segment contains more data than this old segment
                // Delete the old one, insert the new one
                it = map.erase(it);
                insert = true;
                break;
            } else if (beg < seg_beg && seg_beg <= end && end <= seg_end) {
                // 3) beg seg_beg end seg_end
                // Merge two segments, trim front of old segment
                auto trim = end - seg_beg;
                seg_pkt.trim_front(trim);
                p.append(std::move(seg_pkt));
                // Delete the old one, insert the new one
                it = map.erase(it);
                insert = true;
                break;
            } else if (seg_beg <= beg && beg <= seg_end && seg_end < end) {
                // 4) seg_beg beg seg_end end
                // Merge two segments, trim front of new segment
                auto trim = seg_end - beg;
                p.trim_front(trim);
                // Append new data to the old segment, keep the old segment
                seg_pkt.append(std::move(p));
                seg_pkt.linearize();
                ++linearizations_ref();
                insert = false;
                break;
            } else {
                // 5) beg end < seg_beg seg_end
                //   or
                // 6) seg_beg seg_end < beg end
                // Can not merge with this segment, keep looking
                it++;
                insert = true;
            }
        }

        if (insert) {
            p.linearize();
            ++linearizations_ref();
            map.emplace(beg, std::move(p));
        }

        // Second, merge adjacent segments after this packet has been merged,
        // becasue this packet might fill a "whole" and make two adjacent
        // segments mergable
        for (auto it = map.begin(); it != map.end();) {
            // The first segment
            auto& seg_pkt = it->second;
            auto seg_beg = it->first;
            auto seg_end = seg_beg + seg_pkt.len();

            // The second segment
            auto it_next = it;
            it_next++;
            if (it_next == map.end()) {
                break;
            }
            auto& p = it_next->second;
            auto beg = it_next->first;
            auto end = beg + p.len();

            // Merge the the second segment into first segment if possible
            if (seg_beg <= beg && beg <= seg_end && seg_end < end) {
                // Merge two segments, trim front of second segment
                auto trim = seg_end - beg;
                p.trim_front(trim);
                // Append new data to the first segment, keep the first segment
                seg_pkt.append(std::move(p));

                // Delete the second segment
                map.erase(it_next);

                // Keep merging this first segment with its new next packet
                // So we do not update the iterator: it
                continue;
            } else if (end <= seg_end) {
                // The first segment has all the data in the second segment
                // Delete the second segment
                map.erase(it_next);
                continue;
            } else if (seg_end < beg) {
                // Can not merge first segment with second segment
                it = it_next;
                continue;
            } else {
                // If we reach here, we have a bug with merge.
                std::cerr << "packet_merger: merge error\n";
                abort();
                break;
            }
        }
    }
};

}

}

#include <unordered_map>

namespace seastar {

namespace net {

struct udp_hdr {
    packed<uint16_t> src_port;
    packed<uint16_t> dst_port;
    packed<uint16_t> len;
    packed<uint16_t> cksum;

    template<typename Adjuster>
    auto adjust_endianness(Adjuster a) {
        return a(src_port, dst_port, len, cksum);
    }
} __attribute__((packed));

struct udp_channel_state {
    queue<udp_datagram> _queue;
    // Limit number of data queued into send queue
    semaphore _user_queue_space = {212992};
    udp_channel_state(size_t queue_size) : _queue(queue_size) {}
    future<> wait_for_send_buffer(size_t len) { return _user_queue_space.wait(len); }
    void complete_send(size_t len) { _user_queue_space.signal(len); }
};

}

}

namespace seastar {

struct ipv6_addr;

namespace net {

class ipv4;
template <ip_protocol_num ProtoNum>
class ipv4_l4;
struct ipv4_address;

template <typename InetTraits>
class tcp;

struct ipv4_address {
    ipv4_address() : ip(0) {}
    explicit ipv4_address(uint32_t ip) : ip(ip) {}
    explicit ipv4_address(const std::string& addr);
    ipv4_address(ipv4_addr addr) {
        ip = addr.ip;
    }

    packed<uint32_t> ip;

    template <typename Adjuster>
    auto adjust_endianness(Adjuster a) { return a(ip); }

    friend bool operator==(ipv4_address x, ipv4_address y) {
        return x.ip == y.ip;
    }
    friend bool operator!=(ipv4_address x, ipv4_address y) {
        return x.ip != y.ip;
    }

    static ipv4_address read(const char* p) {
        ipv4_address ia;
        ia.ip = read_be<uint32_t>(p);
        return ia;
    }
    static ipv4_address consume(const char*& p) {
        auto ia = read(p);
        p += 4;
        return ia;
    }
    void write(char* p) const {
        write_be<uint32_t>(p, ip);
    }
    void produce(char*& p) const {
        produce_be<uint32_t>(p, ip);
    }
    static constexpr size_t size() {
        return 4;
    }
} __attribute__((packed));

static inline bool is_unspecified(ipv4_address addr) { return addr.ip == 0; }

std::ostream& operator<<(std::ostream& os, const ipv4_address& a);

// IPv6
struct ipv6_address {
    using ipv6_bytes = std::array<uint8_t, 16>;

    static_assert(alignof(ipv6_bytes) == 1, "ipv6_bytes should be byte-aligned");
    static_assert(sizeof(ipv6_bytes) == 16, "ipv6_bytes should be 16 bytes");

    ipv6_address();
    explicit ipv6_address(const ::in6_addr&);
    explicit ipv6_address(const ipv6_bytes&);
    explicit ipv6_address(const std::string&);
    ipv6_address(const ipv6_addr& addr);

    // No need to use packed - we only store
    // as byte array. If we want to read as
    // uints or whatnot, we must copy
    ipv6_bytes ip;

    template <typename Adjuster>
    auto adjust_endianness(Adjuster a) { return a(ip); }

    bool operator==(const ipv6_address& y) const {
        return bytes() == y.bytes();
    }
    bool operator!=(const ipv6_address& y) const {
        return !(*this == y);
    }

    const ipv6_bytes& bytes() const {
        return ip;
    }

    bool is_unspecified() const;

    static ipv6_address read(const char*);
    static ipv6_address consume(const char*& p);
    void write(char* p) const;
    void produce(char*& p) const;
    static constexpr size_t size() {
        return sizeof(ipv6_bytes);
    }
} __attribute__((packed));

std::ostream& operator<<(std::ostream&, const ipv6_address&);

}

}

namespace std {

template <>
struct hash<seastar::net::ipv4_address> {
    size_t operator()(seastar::net::ipv4_address a) const { return a.ip; }
};

template <>
struct hash<seastar::net::ipv6_address> {
    size_t operator()(const seastar::net::ipv6_address&) const;
};

}

namespace seastar {

namespace net {

struct ipv4_traits {
    using address_type = ipv4_address;
    using inet_type = ipv4_l4<ip_protocol_num::tcp>;
    struct l4packet {
        ipv4_address to;
        packet p;
        ethernet_address e_dst;
        ip_protocol_num proto_num;
    };
    using packet_provider_type = std::function<compat::optional<l4packet> ()>;
    static void tcp_pseudo_header_checksum(checksummer& csum, ipv4_address src, ipv4_address dst, uint16_t len) {
        csum.sum_many(src.ip.raw, dst.ip.raw, uint8_t(0), uint8_t(ip_protocol_num::tcp), len);
    }
    static void udp_pseudo_header_checksum(checksummer& csum, ipv4_address src, ipv4_address dst, uint16_t len) {
        csum.sum_many(src.ip.raw, dst.ip.raw, uint8_t(0), uint8_t(ip_protocol_num::udp), len);
    }
    static constexpr uint8_t ip_hdr_len_min = ipv4_hdr_len_min;
};

template <ip_protocol_num ProtoNum>
class ipv4_l4 {
public:
    ipv4& _inet;
public:
    ipv4_l4(ipv4& inet) : _inet(inet) {}
    void register_packet_provider(ipv4_traits::packet_provider_type func);
    future<ethernet_address> get_l2_dst_address(ipv4_address to);
    const ipv4& inet() const {
        return _inet;
    }
};

class ip_protocol {
public:
    virtual ~ip_protocol() {}
    virtual void received(packet p, ipv4_address from, ipv4_address to) = 0;
    virtual bool forward(forward_hash& out_hash_data, packet& p, size_t off) { return true; }
};

template <typename InetTraits>
struct l4connid {
    using ipaddr = typename InetTraits::address_type;
    using inet_type = typename InetTraits::inet_type;
    struct connid_hash;

    ipaddr local_ip;
    ipaddr foreign_ip;
    uint16_t local_port;
    uint16_t foreign_port;

    bool operator==(const l4connid& x) const {
        return local_ip == x.local_ip
                && foreign_ip == x.foreign_ip
                && local_port == x.local_port
                && foreign_port == x.foreign_port;
    }

    uint32_t hash(rss_key_type rss_key) {
        forward_hash hash_data;
        hash_data.push_back(hton(foreign_ip.ip));
        hash_data.push_back(hton(local_ip.ip));
        hash_data.push_back(hton(foreign_port));
        hash_data.push_back(hton(local_port));
        return toeplitz_hash(rss_key, hash_data);
    }
};

class ipv4_tcp final : public ip_protocol {
    ipv4_l4<ip_protocol_num::tcp> _inet_l4;
    std::unique_ptr<tcp<ipv4_traits>> _tcp;
public:
    ipv4_tcp(ipv4& inet);
    ~ipv4_tcp();
    virtual void received(packet p, ipv4_address from, ipv4_address to) override;
    virtual bool forward(forward_hash& out_hash_data, packet& p, size_t off) override;
    friend class ipv4;
};

struct icmp_hdr {
    enum class msg_type : uint8_t {
        echo_reply = 0,
        echo_request = 8,
    };
    msg_type type;
    uint8_t code;
    packed<uint16_t> csum;
    packed<uint32_t> rest;
    template <typename Adjuster>
    auto adjust_endianness(Adjuster a) {
        return a(csum);
    }
} __attribute__((packed));


class icmp {
public:
    using ipaddr = ipv4_address;
    using inet_type = ipv4_l4<ip_protocol_num::icmp>;
    explicit icmp(inet_type& inet) : _inet(inet) {
        _inet.register_packet_provider([this] {
            compat::optional<ipv4_traits::l4packet> l4p;
            if (!_packetq.empty()) {
                l4p = std::move(_packetq.front());
                _packetq.pop_front();
                _queue_space.signal(l4p.value().p.len());
            }
            return l4p;
        });
    }
    void received(packet p, ipaddr from, ipaddr to);
private:
    inet_type& _inet;
    circular_buffer<ipv4_traits::l4packet> _packetq;
    semaphore _queue_space = {212992};
};

class ipv4_icmp final : public ip_protocol {
    ipv4_l4<ip_protocol_num::icmp> _inet_l4;
    icmp _icmp;
public:
    ipv4_icmp(ipv4& inet) : _inet_l4(inet), _icmp(_inet_l4) {}
    virtual void received(packet p, ipv4_address from, ipv4_address to) {
        _icmp.received(std::move(p), from, to);
    }
    friend class ipv4;
};

class ipv4_udp : public ip_protocol {
    using connid = l4connid<ipv4_traits>;
    using connid_hash = typename connid::connid_hash;

public:
    static const int default_queue_size;
private:
    static const uint16_t min_anonymous_port = 32768;
    ipv4 &_inet;
    std::unordered_map<uint16_t, lw_shared_ptr<udp_channel_state>> _channels;
    int _queue_size = default_queue_size;
    uint16_t _next_anonymous_port = min_anonymous_port;
    circular_buffer<ipv4_traits::l4packet> _packetq;
private:
    uint16_t next_port(uint16_t port);
public:
    class registration {
    private:
        ipv4_udp &_proto;
        uint16_t _port;
    public:
        registration(ipv4_udp &proto, uint16_t port) : _proto(proto), _port(port) {};

        void unregister() {
            _proto._channels.erase(_proto._channels.find(_port));
        }

        uint16_t port() const {
            return _port;
        }
    };

    ipv4_udp(ipv4& inet);
    udp_channel make_channel(ipv4_addr addr);
    virtual void received(packet p, ipv4_address from, ipv4_address to) override;
    void send(uint16_t src_port, ipv4_addr dst, packet &&p);
    bool forward(forward_hash& out_hash_data, packet& p, size_t off) override;
    void set_queue_size(int size) { _queue_size = size; }

    const ipv4& inet() const {
        return _inet;
    }
};

struct ip_hdr;

struct ip_packet_filter {
    virtual ~ip_packet_filter() {};
    virtual future<> handle(packet& p, ip_hdr* iph, ethernet_address from, bool & handled) = 0;
};

struct ipv4_frag_id {
    struct hash;
    ipv4_address src_ip;
    ipv4_address dst_ip;
    uint16_t identification;
    uint8_t protocol;
    bool operator==(const ipv4_frag_id& x) const {
        return src_ip == x.src_ip &&
               dst_ip == x.dst_ip &&
               identification == x.identification &&
               protocol == x.protocol;
    }
};

struct ipv4_frag_id::hash : private std::hash<ipv4_address>,
    private std::hash<uint16_t>, private std::hash<uint8_t> {
    size_t operator()(const ipv4_frag_id& id) const noexcept {
        using h1 = std::hash<ipv4_address>;
        using h2 = std::hash<uint16_t>;
        using h3 = std::hash<uint8_t>;
        return h1::operator()(id.src_ip) ^
               h1::operator()(id.dst_ip) ^
               h2::operator()(id.identification) ^
               h3::operator()(id.protocol);
    }
};

struct ipv4_tag {};
using ipv4_packet_merger = packet_merger<uint32_t, ipv4_tag>;

class ipv4 {
public:
    using clock_type = lowres_clock;
    using address_type = ipv4_address;
    using proto_type = uint16_t;
    static address_type broadcast_address() { return ipv4_address(0xffffffff); }
    static proto_type arp_protocol_type() { return proto_type(eth_protocol_num::ipv4); }
private:
    interface* _netif;
    std::vector<ipv4_traits::packet_provider_type> _pkt_providers;
    arp _global_arp;
    arp_for<ipv4> _arp;
    ipv4_address _host_address;
    ipv4_address _gw_address;
    ipv4_address _netmask;
    l3_protocol _l3;
    ipv4_tcp _tcp;
    ipv4_icmp _icmp;
    ipv4_udp _udp;
    array_map<ip_protocol*, 256> _l4;
    ip_packet_filter * _packet_filter = nullptr;
    struct frag {
        packet header;
        ipv4_packet_merger data;
        clock_type::time_point rx_time;
        uint32_t mem_size = 0;
        // fragment with MF == 0 inidates it is the last fragment
        bool last_frag_received = false;

        packet get_assembled_packet(ethernet_address from, ethernet_address to);
        int32_t merge(ip_hdr &h, uint16_t offset, packet p);
        bool is_complete();
    };
    std::unordered_map<ipv4_frag_id, frag, ipv4_frag_id::hash> _frags;
    std::list<ipv4_frag_id> _frags_age;
    static constexpr std::chrono::seconds _frag_timeout{30};
    static constexpr uint32_t _frag_low_thresh{3 * 1024 * 1024};
    static constexpr uint32_t _frag_high_thresh{4 * 1024 * 1024};
    uint32_t _frag_mem{0};
    timer<lowres_clock> _frag_timer;
    circular_buffer<l3_protocol::l3packet> _packetq;
    unsigned _pkt_provider_idx = 0;
    metrics::metric_groups _metrics;
private:
    future<> handle_received_packet(packet p, ethernet_address from);
    bool forward(forward_hash& out_hash_data, packet& p, size_t off);
    compat::optional<l3_protocol::l3packet> get_packet();
    bool in_my_netmask(ipv4_address a) const;
    void frag_limit_mem();
    void frag_timeout();
    void frag_drop(ipv4_frag_id frag_id, uint32_t dropped_size);
    void frag_arm(clock_type::time_point now) {
        auto tp = now + _frag_timeout;
        _frag_timer.arm(tp);
    }
    void frag_arm() {
        auto now = clock_type::now();
        frag_arm(now);
    }
public:
    explicit ipv4(interface* netif);
    void set_host_address(ipv4_address ip);
    ipv4_address host_address() const;
    void set_gw_address(ipv4_address ip);
    ipv4_address gw_address() const;
    void set_netmask_address(ipv4_address ip);
    ipv4_address netmask_address() const;
    interface * netif() const {
        return _netif;
    }
    // TODO or something. Should perhaps truly be a list
    // of filters. With ordering. And blackjack. Etc.
    // But for now, a simple single raw pointer suffices
    void set_packet_filter(ip_packet_filter *);
    ip_packet_filter * packet_filter() const;
    void send(ipv4_address to, ip_protocol_num proto_num, packet p, ethernet_address e_dst);
    tcp<ipv4_traits>& get_tcp() { return *_tcp._tcp; }
    ipv4_udp& get_udp() { return _udp; }
    void register_l4(proto_type id, ip_protocol* handler);
    const net::hw_features& hw_features() const { return _netif->hw_features(); }
    static bool needs_frag(packet& p, ip_protocol_num proto_num, net::hw_features hw_features);
    void learn(ethernet_address l2, ipv4_address l3) {
        _arp.learn(l2, l3);
    }
    void register_packet_provider(ipv4_traits::packet_provider_type&& func) {
        _pkt_providers.push_back(std::move(func));
    }
    future<ethernet_address> get_l2_dst_address(ipv4_address to);
};

template <ip_protocol_num ProtoNum>
inline
void ipv4_l4<ProtoNum>::register_packet_provider(ipv4_traits::packet_provider_type func) {
    _inet.register_packet_provider([func = std::move(func)] {
        auto l4p = func();
        if (l4p) {
            l4p.value().proto_num = ProtoNum;
        }
        return l4p;
    });
}

template <ip_protocol_num ProtoNum>
inline
future<ethernet_address> ipv4_l4<ProtoNum>::get_l2_dst_address(ipv4_address to) {
    return _inet.get_l2_dst_address(to);
}

struct ip_hdr {
    uint8_t ihl : 4;
    uint8_t ver : 4;
    uint8_t dscp : 6;
    uint8_t ecn : 2;
    packed<uint16_t> len;
    packed<uint16_t> id;
    packed<uint16_t> frag;
    enum class frag_bits : uint8_t { mf = 13, df = 14, reserved = 15, offset_shift = 3 };
    uint8_t ttl;
    uint8_t ip_proto;
    packed<uint16_t> csum;
    ipv4_address src_ip;
    ipv4_address dst_ip;
    uint8_t options[0];
    template <typename Adjuster>
    auto adjust_endianness(Adjuster a) {
        return a(len, id, frag, csum, src_ip, dst_ip);
    }
    bool mf() { return frag & (1 << uint8_t(frag_bits::mf)); }
    bool df() { return frag & (1 << uint8_t(frag_bits::df)); }
    uint16_t offset() { return frag << uint8_t(frag_bits::offset_shift); }
} __attribute__((packed));

template <typename InetTraits>
struct l4connid<InetTraits>::connid_hash : private std::hash<ipaddr>, private std::hash<uint16_t> {
    size_t operator()(const l4connid<InetTraits>& id) const noexcept {
        using h1 = std::hash<ipaddr>;
        using h2 = std::hash<uint16_t>;
        return h1::operator()(id.local_ip)
            ^ h1::operator()(id.foreign_ip)
            ^ h2::operator()(id.local_port)
            ^ h2::operator()(id.foreign_port);
    }
};

void arp_learn(ethernet_address l2, ipv4_address l3);

}

}
using data_type = shared_ptr<const abstract_type>;
class column_set {};
class schema final : public enable_lw_shared_from_this<schema> {
  struct column {
    bytes name;
    data_type type;
  };

public:
  schema(std::optional<utils::UUID> id, std::string_view ks_name,
         std::string_view cf_name, std::vector<column> partition_key,
         std::vector<column> clustering_key,
         std::vector<column> regular_columns,
         std::vector<column> static_columns, data_type regular_column_name_type,
         std::string_view comment = {});
  ~schema();

public:
};
struct blob_storage {
  struct [[gnu::packed]] ref_type {
    blob_storage *ptr;
    ref_type() {}
    ref_type(blob_storage * ptr) : ptr(ptr) {}
    operator blob_storage *() const { return ptr; }
    blob_storage *operator->() const { return ptr; }
    blob_storage &operator*() const { return *ptr; }
  };
} __attribute__((packed));
class table;
using column_family = table;
class clustering_key_prefix;
template <typename T> class wrapping_range {};
template <typename T> class nonwrapping_range {};
GCC6_CONCEPT(template <template <typename> typename T, typename U>
             concept bool Range =
                 std::is_same<T<U>, wrapping_range<U>>::value ||
                 std::is_same<T<U>, nonwrapping_range<U>>::value;)
namespace std {}
namespace dht {
class decorated_key;
}
template <typename EnumType, EnumType... Items> struct super_enum {};
template <typename Enum> class enum_set {};
namespace tracing {
class trace_state_ptr;
}
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
};
} // namespace query
class range_tombstone final {};
namespace db {
using timeout_clock = seastar::lowres_clock;
}
class clustering_row {};
class static_row {};
class partition_start final {
public:
};
class partition_end final {
public:
};
GCC6_CONCEPT(template <typename T, typename ReturnType>
             concept bool MutationFragmentConsumer() {
               return requires(T t, static_row sr, clustering_row cr,
                               range_tombstone rt, partition_start ph,
                               partition_end pe) {
                 { t.consume(std::move(sr)) }
                 ->ReturnType;
                 { t.consume(std::move(cr)) }
                 ->ReturnType;
                 { t.consume(std::move(rt)) }
                 ->ReturnType;
                 { t.consume(std::move(ph)) }
                 ->ReturnType;
                 { t.consume(std::move(pe)) }
                 ->ReturnType;
               };
             })
class mutation_fragment {};
GCC6_CONCEPT(template <typename F> concept bool StreamedMutationTranformer() {
  return requires(F f, mutation_fragment mf, schema_ptr s) {
    { f(std::move(mf)) }
    ->mutation_fragment;
    { f(s) }
    ->schema_ptr;
  };
})
class mutation final {
  mutation() = default;

public:
  const dht::decorated_key &decorated_key() const;
};
using mutation_opt = optimized_optional<mutation>;
class flat_mutation_reader;
future<mutation_opt>
read_mutation_from_flat_mutation_reader(flat_mutation_reader &reader,
                                        db::timeout_clock::time_point timeout);
class locked_cell;
class frozen_mutation;
class table {
public:
  future<std::vector<locked_cell>>
  lock_counter_cells(const mutation &m, db::timeout_clock::time_point timeout);
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
} // namespace tracing
template <typename Consumer>
inline future<> consume_partitions(flat_mutation_reader &reader,
                                   Consumer consumer,
                                   db::timeout_clock::time_point timeout) {
  using futurator = futurize<std::result_of_t<Consumer(mutation &&)>>;
  return do_with(
      std::move(consumer), [&reader, timeout](Consumer &c) -> future<> {
        return repeat([&reader, &c, timeout]() {
          return read_mutation_from_flat_mutation_reader(reader, timeout)
              .then([&c](mutation_opt &&mo) -> future<stop_iteration> {
                if (!mo) {
                  return make_ready_future<stop_iteration>(stop_iteration::yes);
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
future<mutation_opt> counter_write_query(schema_ptr, const mutation_source &,
                                         const dht::decorated_key &dk,
                                         const query::partition_slice &slice,
                                         tracing::trace_state_ptr trace_ptr);
class locked_cell {};
future<mutation>
database::do_apply_counter_update(column_family &cf, const frozen_mutation &fm,
                                  schema_ptr m_schema,
                                  db::timeout_clock::time_point timeout,
                                  tracing::trace_state_ptr trace_state) {
  auto m = fm.unfreeze(m_schema);
  query::column_id_vector static_columns;
  query::clustering_row_ranges cr_ranges;
  query::column_id_vector regular_columns;
  auto slice = query::partition_slice(
      std::move(cr_ranges), std::move(static_columns),
      std::move(regular_columns), {}, {}, cql_serialization_format::internal(),
      query::max_rows);
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
