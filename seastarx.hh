/*
 * Copyright (C) 2017-present ScyllaDB
 */

/*
 * SPDX-License-Identifier: LicenseRef-ScyllaDB-Source-Available-1.1
 */

#pragma once

#include <coroutine>
// Operators defined in headers at namespace std (e.g. operator<< for
// seastar::lazy_eval, std::exception_ptr) are not visible via the seastar
// module import.  Pull them in textually (before `import seastar;` to avoid
// std-library redefinition diagnostics) so BOOST_CHECK_MESSAGE and similar
// can find them.
#include <iosfwd>
#include <exception>
#include <system_error>
namespace std {
std::ostream& operator<<(std::ostream&, const std::exception_ptr&);
std::ostream& operator<<(std::ostream&, const std::exception&);
std::ostream& operator<<(std::ostream&, const std::system_error&);
}

import fmt;
import seastar;

// Explicit using declarations for seastar symbols.
// We avoid `using namespace seastar;` because `import seastar;` exports
// seastar::file_handle, which conflicts with the POSIX struct file_handle
// from <bits/fcntl-linux.h>.  By listing symbols individually we can
// exclude file_handle and file_handle_impl.

// Core types and utilities
using seastar::app_template;
using seastar::logger;
using seastar::log_level;
using seastar::future;
using seastar::Future;
using seastar::promise;
using seastar::shared_future;
using seastar::shared_promise;
using seastar::make_ready_future;
using seastar::make_exception_future;
using seastar::now;
using seastar::current_exception_as_future;
using seastar::futurize;
using seastar::futurize_invoke;
using seastar::futurize_t;
using seastar::is_future;
using seastar::thread;
using seastar::thread_attributes;
using seastar::reactor;
using seastar::engine;
using seastar::local_engine;
using seastar::engine_is_ready;
using seastar::smp;
using seastar::shard_id;
using seastar::this_shard_id;
using seastar::this_smp;
using seastar::this_smp_shard_count;
using seastar::this_smp_all_shards;

// Async
using seastar::async;

// Memory management
using seastar::temporary_buffer;
using seastar::deleter;
using seastar::free_deleter;
using seastar::shared_ptr;
using seastar::lw_shared_ptr;
using seastar::make_shared;
using seastar::make_lw_shared;
using seastar::weak_ptr;
using seastar::enable_shared_from_this;
using seastar::enable_lw_shared_from_this;
using seastar::foreign_ptr;
using seastar::make_foreign;
using seastar::weakly_referencable;
using seastar::allocate_aligned_buffer;
using seastar::static_pointer_cast;
using seastar::dynamic_pointer_cast;
using seastar::shared_ptr_make_helper;
using seastar::shared_ptr_value_hash;
using seastar::shared_ptr_equal_by_value;
using seastar::indirect_hash;
using seastar::indirect_less;
using seastar::lw_shared_ptr_deleter;
using seastar::make_deleter;
// NOTE: make_sstring has internal linkage (static) and cannot be
// and seastar::make_sstring at call sites.
using seastar::subscription;

// Synchronization primitives
using seastar::basic_semaphore;
using seastar::semaphore;
using seastar::broken_semaphore;
using seastar::semaphore_timed_out;
using seastar::semaphore_aborted;
using seastar::named_semaphore_aborted;
using seastar::semaphore_units;
using seastar::named_semaphore;
using seastar::named_semaphore_timed_out;
using seastar::broken_named_semaphore;
using seastar::named_semaphore_exception_factory;
using seastar::get_units;
using seastar::consume_units;
using seastar::try_get_units;
using seastar::with_semaphore;
using seastar::gate;
using seastar::gate_closed_exception;
using seastar::named_gate;
using seastar::with_gate;
using seastar::try_with_gate;
using seastar::condition_variable;
using seastar::broken_condition_variable;
using seastar::condition_variable_timed_out;
using seastar::rwlock;
using seastar::basic_rwlock;
using seastar::shared_mutex;
using seastar::abort_source;
using seastar::abort_requested_exception;

// Containers
using seastar::circular_buffer;
using seastar::circular_buffer_fixed_capacity;
using seastar::chunked_fifo;
using seastar::queue;
using seastar::basic_sstring;
using seastar::sstring;
using seastar::uninitialized_string;
using seastar::expiring_fifo;
// NOT: using seastar::pipe;       — conflicts with POSIX pipe()
using seastar::pipe_reader;
using seastar::pipe_writer;
using seastar::checked_ptr;
using seastar::checked_ptr_is_null_exception;

// Scheduling
using seastar::scheduling_group;
using seastar::scheduling_supergroup;
using seastar::current_scheduling_group;
using seastar::default_scheduling_group;
using seastar::create_scheduling_group;
using seastar::destroy_scheduling_group;
using seastar::rename_scheduling_group;
using seastar::create_scheduling_supergroup;
using seastar::destroy_scheduling_supergroup;
using seastar::max_scheduling_groups;
using seastar::make_scheduling_group_key_config;
using seastar::need_preempt;
using seastar::smp_service_group;
using seastar::smp_service_group_config;
using seastar::default_smp_service_group;
using seastar::create_smp_service_group;
using seastar::destroy_smp_service_group;
using seastar::smp_submit_to_options;
using seastar::inheriting_execution_stage;
using seastar::inheriting_concrete_execution_stage;
using seastar::scheduling_group_key;
using seastar::scheduling_group_key_config;
using seastar::scheduling_group_key_create;
using seastar::scheduling_group_get_specific;
using seastar::reduce_scheduling_group_specific;

// Idle CPU handler
using seastar::idle_cpu_handler;
using seastar::idle_cpu_handler_result;
using seastar::work_waiting_on_reactor;

// Timers and clocks
using seastar::timer;
using seastar::lowres_clock;
using seastar::lowres_system_clock;
using seastar::manual_clock;
using seastar::steady_clock_type;

// Sharding
using seastar::sharded;
using seastar::sharded_parameter;
using seastar::async_sharded_service;
using seastar::peering_sharded_service;
using seastar::no_sharded_instance_exception;

// I/O (file_handle and file_handle_impl excluded — conflicts with POSIX)
using seastar::file;
using seastar::file_open_options;
using seastar::open_flags;
using seastar::access_flags;
using seastar::file_permissions;
using seastar::follow_symlink;
using seastar::directory_entry_type;
using seastar::directory_entry;
using seastar::list_directory_generator_type;
// NOT: using seastar::file_handle;
using seastar::file_handle_impl;
using seastar::file_impl;
using seastar::layered_file_impl;
using seastar::file_input_stream_options;
using seastar::file_input_stream_history;
using seastar::file_output_stream_options;
using seastar::io_intent;
using seastar::input_stream;
using seastar::output_stream;
using seastar::output_stream_options;
using seastar::data_sink;
using seastar::data_source;
using seastar::data_source_impl;
using seastar::data_sink_impl;
using seastar::stat_data;
using seastar::open_file_dma;
using seastar::open_directory;
using seastar::check_direct_io_support;
using seastar::file_exists;
using seastar::remove_file;
using seastar::rename_file;
using seastar::file_accessible;
using seastar::file_type;
using seastar::memory_allocator;
using seastar::with_shared;
using seastar::get_shared_lock;
using seastar::with_lock;
using seastar::get_unique_lock;
using seastar::make_file_data_sink;
using seastar::make_file_data_source;
using seastar::link_file;
using seastar::make_directory;
using seastar::rename_flags;
using seastar::sync_directory;
using seastar::touch_directory;
using seastar::recursive_touch_directory;
using seastar::recursive_remove_directory;
using seastar::file_stat;
using seastar::file_size;
using seastar::fs_avail;
using seastar::make_file_input_stream;
using seastar::make_file_output_stream;
using seastar::with_file;
using seastar::with_file_close_on_failure;
using seastar::copy;
using seastar::consumption_result;
using seastar::stop_consuming;
using seastar::skip_bytes;
using seastar::continue_consuming;

// Networking
using seastar::socket;
using seastar::server_socket;
using seastar::connected_socket;
using seastar::accept_result;
using seastar::socket_address;
using seastar::listen_options;
using seastar::ipv4_addr;
using seastar::ipv6_addr;
using seastar::transport;
using seastar::listen;
using seastar::connect;
using seastar::make_ipv4_address;

// Sleep
using seastar::sleep;
using seastar::sleep_abortable;
using seastar::sleep_aborted;

// Utility functions and types
using seastar::do_with;
using seastar::do_for_each;
using seastar::do_until;
using seastar::keep_doing;
using seastar::repeat;
using seastar::repeat_until_value;
using seastar::stop_iteration;
using seastar::parallel_for_each;
using seastar::max_concurrent_for_each;
using seastar::map_reduce;
using seastar::adder;
using seastar::when_all;
using seastar::when_all_succeed;
using seastar::when_any;
using seastar::yield;
using seastar::to_sstring;
using seastar::make_visitor;
// seastar::visit excluded — ScyllaDB has its own ::visit(const abstract_type&, Func&&)
// seastar::ref / seastar::cref excluded — too close to std::ref / std::cref
using seastar::lazy_deref;
using seastar::lazy_eval;
using seastar::pretty_type_name;
using seastar::log2ceil;
using seastar::log2floor;
using seastar::align_up;
using seastar::align_down;
using seastar::count_leading_zeros;
using seastar::count_trailing_zeros;
using seastar::enum_hash;

// Defer and RAII
using seastar::defer;
using seastar::deferred_close;
using seastar::deferred_stop;
using seastar::with_closeable;

// Bool class
using seastar::bool_class;
using seastar::optimized_optional;
using seastar::noncopyable_function;
using seastar::value_of;

// Byte-order / endian helpers
using seastar::cpu_to_be;
using seastar::be_to_cpu;
using seastar::cpu_to_le;
using seastar::le_to_cpu;
using seastar::read_le;
using seastar::read_be;
using seastar::write_le;
using seastar::write_be;
using seastar::consume_be;
using seastar::produce_be;
using seastar::htonq;
using seastar::ntohq;

// Serialization streams
using seastar::simple_memory_input_stream;
using seastar::simple_input_stream;
using seastar::simple_memory_output_stream;
using seastar::simple_output_stream;
using seastar::measuring_output_stream;
using seastar::fragmented_memory_input_stream;

// Error handling
using seastar::on_internal_error;
using seastar::on_internal_error_noexcept;
using seastar::on_fatal_internal_error;
using seastar::set_abort_on_internal_error;
using seastar::set_abort_on_ebadf;
using seastar::current_backtrace;
using seastar::current_backtrace_tasklocal;
using seastar::throw_with_backtrace;
using seastar::make_backtraced_exception_ptr;
using seastar::simple_backtrace;
using seastar::shared_backtrace;
using seastar::saved_backtrace;
using seastar::tasktrace;
using seastar::handle_signal;

// Exception types
using seastar::nested_exception;
using seastar::timed_out_error;
using seastar::cancelled_error;
using seastar::broken_promise;
using seastar::broken_pipe_exception;

// Resource management
using seastar::with_scheduling_group;
using seastar::with_timeout;

// Print utilities
using seastar::format;

// Logging
using seastar::logger_registry;
using seastar::global_logger_registry;
using seastar::logging_settings;
using seastar::apply_logging_settings;
using seastar::level_name;

// POSIX wrappers
using seastar::chown;

// UDL operators for memory units
using seastar::operator""_KiB;
using seastar::operator""_MiB;
using seastar::operator""_GiB;
using seastar::operator""_TiB;

// Temporary files
using seastar::tmp_file;
using seastar::tmp_dir;
using seastar::make_tmp_file;

// Unix domain
using seastar::unix_domain_addr;

// Additional symbols used by ScyllaDB
using seastar::with_clock;
using seastar::memory_output_stream;
using seastar::abort_on_expiry;
using seastar::http_response_parser;
using seastar::semaphore_default_exception_factory;
using seastar::file_desc;
using seastar::make_socket;
using seastar::deferred_action;
using seastar::indirect_equal_to;

// Sub-namespace aliases — code throughout the tree uses these without
// the seastar:: prefix (e.g. net::inet_address, tls::credentials_builder).
namespace coroutine = seastar::coroutine;
namespace memory = seastar::memory;
namespace metrics = seastar::metrics;
namespace net = seastar::net;
namespace rpc = seastar::rpc;
namespace tls = seastar::tls;
namespace http = seastar::http;
namespace httpd = seastar::httpd;
namespace json = seastar::json;
namespace alien = seastar::alien;
namespace util = seastar::util;
namespace log_cli = seastar::log_cli;
namespace program_options = seastar::program_options;
namespace prometheus = seastar::prometheus;
namespace seastar::testing {}
namespace testing = seastar::testing;
namespace scollectd = seastar::scollectd;
