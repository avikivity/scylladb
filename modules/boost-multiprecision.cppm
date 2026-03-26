/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for Boost.Multiprecision.

module;

#include <climits>

#include <boost/multiprecision/cpp_int.hpp>

export module boost:multiprecision;

export namespace boost::multiprecision {
    using boost::multiprecision::cpp_int;
    using boost::multiprecision::limb_type;

    // boost::multiprecision::bits_per_limb is a non-inline `constexpr`
    // variable, so it has internal linkage and cannot be re-exported from
    // a module.  Provide an externally-linked replica under a different
    // name so importer TUs can reach it without falling back to a textual
    // include of <boost/multiprecision/cpp_int.hpp>.
    inline constexpr std::size_t bits_per_limb_v =
        sizeof(limb_type) * CHAR_BIT;
    using boost::multiprecision::cpp_rational;
    using boost::multiprecision::uint128_t;
    using boost::multiprecision::pow;
    using boost::multiprecision::abs;
    using boost::multiprecision::sign;
    using boost::multiprecision::msb;
    using boost::multiprecision::export_bits;
    using boost::multiprecision::import_bits;
    using boost::multiprecision::numerator;
    using boost::multiprecision::denominator;
    using boost::multiprecision::operator+;
    using boost::multiprecision::operator-;
    using boost::multiprecision::operator*;
    using boost::multiprecision::operator/;
    using boost::multiprecision::operator%;
    using boost::multiprecision::operator==;
    using boost::multiprecision::operator!=;
    using boost::multiprecision::operator<;
    using boost::multiprecision::operator>;
    using boost::multiprecision::operator<=;
    using boost::multiprecision::operator>=;
    using boost::multiprecision::operator<<;
    using boost::multiprecision::operator>>;
    using boost::multiprecision::operator&;
    using boost::multiprecision::operator|;
    using boost::multiprecision::operator^;

    // bits_per_limb is a constexpr variable with internal linkage in
    // Boost, so it cannot be exported from a module.  It remains
    // available through textual #include in headers.
}

// boost::multiprecision::backends::cpp_int_backend::do_assign_float<F> is
// instantiated whenever an importer constructs or assigns to a cpp_int from
// an arithmetic type.  Its body calls `eval_left_shift(*this, ...)` and
// `eval_right_shift(*this, ...)` *unqualified*, relying on ADL through
// cpp_int_backend's namespace (boost::multiprecision::backends).  Unlike
// eval_add / eval_subtract — which the same function pulls in with explicit
// `using default_ops::eval_*;` declarations — the shift helpers have no
// such using-declaration in Boost.  With a textual include the corresponding
// overloads in boost::multiprecision::backends are simply visible at the
// instantiation point, but with `import boost;` clang fails ADL on them
// (likely a clang/modules reachability limitation for entities declared in
// the global module fragment).  Re-exporting the shift helpers (and the
// add/subtract counterparts for symmetry) from the partition makes them
// reachable to ADL in importer translation units.
export namespace boost::multiprecision::backends {
    using boost::multiprecision::backends::eval_abs;
    using boost::multiprecision::backends::eval_add;
    using boost::multiprecision::backends::eval_bit_flip;
    using boost::multiprecision::backends::eval_bit_set;
    using boost::multiprecision::backends::eval_bit_test;
    using boost::multiprecision::backends::eval_bit_unset;
    using boost::multiprecision::backends::eval_bitwise_and;
    using boost::multiprecision::backends::eval_bitwise_or;
    using boost::multiprecision::backends::eval_bitwise_xor;
    using boost::multiprecision::backends::eval_complement;
    using boost::multiprecision::backends::eval_convert_to;
    using boost::multiprecision::backends::eval_decrement;
    using boost::multiprecision::backends::eval_divide;
    using boost::multiprecision::backends::eval_eq;
    using boost::multiprecision::backends::eval_gcd;
    using boost::multiprecision::backends::eval_get_sign;
    using boost::multiprecision::backends::eval_gt;
    using boost::multiprecision::backends::eval_increment;
    using boost::multiprecision::backends::eval_integer_modulus;
    using boost::multiprecision::backends::eval_is_zero;
    using boost::multiprecision::backends::eval_lcm;
    using boost::multiprecision::backends::eval_left_shift;
    using boost::multiprecision::backends::eval_lsb;
    using boost::multiprecision::backends::eval_lt;
    using boost::multiprecision::backends::eval_modulus;
    using boost::multiprecision::backends::eval_msb;
    using boost::multiprecision::backends::eval_multiply;
    using boost::multiprecision::backends::eval_qr;
    using boost::multiprecision::backends::eval_right_shift;
    using boost::multiprecision::backends::eval_subtract;
    using boost::multiprecision::backends::divide_unsigned_helper;
}
