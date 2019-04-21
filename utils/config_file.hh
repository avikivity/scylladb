/*
 * Copyright (C) 2017 ScyllaDB
 *
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

#pragma once

#include <unordered_map>
#include <iosfwd>
#include <string_view>

#include <boost/program_options.hpp>

#include <seastar/core/sstring.hh>
#include <seastar/core/future.hh>

#include "seastarx.hh"

namespace seastar { class file; }
namespace seastar::json { class json_return_type; }
namespace YAML { class Node; }

namespace utils {

namespace bpo = boost::program_options;


// This is an extension point to allow converting types that json::formatter::to_json
// doesn't support.
template <typename T>
const T&
config_value_as_json(const T& v) {
    return v;
}

class config_file {
public:
    typedef std::unordered_map<sstring, sstring> string_map;
    typedef std::vector<sstring> string_list;

    enum class value_status {
        Used,
        Unused,
        Invalid,
    };

    enum class config_source : uint8_t {
        None,
        SettingsFile,
        CommandLine
    };

    struct config_src {
        std::string_view _name, _desc;
        std::string_view _type_name;
    public:
        config_src(std::string_view name, std::string_view type_name, std::string_view desc)
            : _name(name)
            , _desc(desc)
            , _type_name(type_name)
        {}
        virtual ~config_src() {}

        const std::string_view & name() const {
            return _name;
        }
        const std::string_view & desc() const {
            return _desc;
        }
        const std::string_view& type_name() const {
            return _type_name;
        }

        virtual void add_command_line_option(
                        bpo::options_description_easy_init&, const std::string_view&,
                        const std::string_view&) = 0;
        virtual void set_value(const YAML::Node&) = 0;
        virtual value_status status() const = 0;
        virtual config_source source() const = 0;
        virtual json::json_return_type value_as_json() const = 0;
    };

    template<typename T>
    struct named_value : public config_src {
    private:
        friend class config;
        std::string_view _name, _desc;
        T _value = T();
        config_source _source = config_source::None;
        value_status _value_status;
    public:
        typedef T type;
        typedef named_value<T> MyType;

        named_value(config_file* file, std::string_view name, std::string_view type_name, value_status vs, const T& t = T(), std::string_view desc = {},
                std::initializer_list<T> allowed_values = {})
            : config_src(name, type_name, desc)
            , _value(t)
            , _value_status(vs)
        {
            file->add(*this);
        }
        value_status status() const override {
            return _value_status;
        }
        config_source source() const override {
            return _source;
        }
        bool is_set() const {
            return _source > config_source::None;
        }
        MyType & operator()(const T& t) {
            _value = t;
            return *this;
        }
        MyType & operator()(T&& t, config_source src = config_source::None) {
            _value = std::move(t);
            if (src > config_source::None) {
                _source = src;
            }
            return *this;
        }
        const T& operator()() const {
            return _value;
        }
        T& operator()() {
            return _value;
        }

        void add_command_line_option(bpo::options_description_easy_init&,
                        const std::string_view&, const std::string_view&) override;
        void set_value(const YAML::Node&) override;
        virtual json::json_return_type value_as_json() const override;
    };

    typedef std::reference_wrapper<config_src> cfg_ref;

    config_file(std::initializer_list<cfg_ref> = {});

    void add(cfg_ref);
    void add(std::initializer_list<cfg_ref>);
    void add(const std::vector<cfg_ref> &);

    boost::program_options::options_description get_options_description();
    boost::program_options::options_description get_options_description(boost::program_options::options_description);

    boost::program_options::options_description_easy_init&
    add_options(boost::program_options::options_description_easy_init&);

    /**
     * Default behaviour for yaml parser is to throw on
     * unknown stuff, invalid opts or conversion errors.
     *
     * Error handling function allows overriding this.
     *
     * error: <option name>, <message>, <optional value_status>
     *
     * The last arg, opt value_status will tell you the type of
     * error occurred. If not set, the option found does not exist.
     * If invalid, it is invalid. Otherwise, a parse error.
     *
     */
    using error_handler = std::function<void(const sstring&, const sstring&, std::optional<value_status>)>;

    void read_from_yaml(const sstring&, error_handler = {});
    void read_from_yaml(const char *, error_handler = {});
    future<> read_from_file(const sstring&, error_handler = {});
    future<> read_from_file(file, error_handler = {});

    using configs = std::vector<cfg_ref>;

    configs set_values() const;
    configs unset_values() const;
    const configs& values() const {
        return _cfgs;
    }
private:
    configs
        _cfgs;
};

extern template struct config_file::named_value<seastar::log_level>;

}

