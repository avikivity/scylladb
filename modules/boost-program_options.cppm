/*
 * Copyright (C) 2026-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.0
 */

// C++20 module partition for Boost.Program_options.

module;

#include <boost/program_options.hpp>

export module boost:program_options;

export namespace boost::program_options {
    using boost::program_options::variables_map;
    using boost::program_options::options_description;
    using boost::program_options::options_description_easy_init;
    using boost::program_options::positional_options_description;
    using boost::program_options::parsed_options;
    using boost::program_options::option_description;
    using boost::program_options::variable_value;
    using boost::program_options::value_semantic;

    // Free functions
    using boost::program_options::value;
    using boost::program_options::bool_switch;
    using boost::program_options::store;
    using boost::program_options::notify;
    using boost::program_options::command_line_parser;
    using boost::program_options::parse_command_line;
    using boost::program_options::parse_config_file;

    using boost::program_options::option;
    using boost::program_options::typed_value;

    // Exceptions
    using boost::program_options::invalid_option_value;
    using boost::program_options::error;

    namespace command_line_style {
        using boost::program_options::command_line_style::allow_long;
        using boost::program_options::command_line_style::allow_short;
        using boost::program_options::command_line_style::allow_dash_for_short;
        using boost::program_options::command_line_style::allow_slash_for_short;
        using boost::program_options::command_line_style::long_allow_adjacent;
        using boost::program_options::command_line_style::long_allow_next;
        using boost::program_options::command_line_style::short_allow_adjacent;
        using boost::program_options::command_line_style::short_allow_next;
        using boost::program_options::command_line_style::allow_sticky;
        using boost::program_options::command_line_style::allow_guessing;
        using boost::program_options::command_line_style::long_case_insensitive;
        using boost::program_options::command_line_style::short_case_insensitive;
        using boost::program_options::command_line_style::case_insensitive;
        using boost::program_options::command_line_style::allow_long_disguise;
        using boost::program_options::command_line_style::unix_style;
        using boost::program_options::command_line_style::default_style;
    }
}
