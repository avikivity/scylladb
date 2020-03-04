/*
 * Copyright (C) 2017 ScyllaDB
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

#include <seastar/util/bool_class.hh>
#include <seastar/core/future.hh>

#include "position_in_partition.hh"
#include "mutation_fragment.hh"
#include "tracing/trace_state.hh"

#include <seastar/util/gcc6-concepts.hh>
#include <seastar/core/thread.hh>
#include "db/timeout_clock.hh"

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
    // Documented in mutation_reader::forwarding in mutation_reader.hh.
    class partition_range_forwarding_tag;
    using partition_range_forwarding = bool_class<partition_range_forwarding_tag>;

    flat_mutation_reader(std::unique_ptr<impl> impl) noexcept;

    future<mutation_fragment_opt> operator()(db::timeout_clock::time_point timeout);



    // Skips to the next partition.
    //
    // Skips over the remaining fragments of the current partitions. If the
    // reader is currently positioned at a partition boundary (partition
    // start) nothing is done.
    // Only skips within the current partition range, i.e. if the current
    // partition is the last in the range the reader will be at EOS.
    //
    // Can be used to skip over entire partitions if interleaved with
    // `operator()()` calls.
    void next_partition();

    future<> fill_buffer(db::timeout_clock::time_point timeout);

    // Changes the range of partitions to pr. The range can only be moved
    // forwards. pr.begin() needs to be larger than pr.end() of the previousl
    // used range (i.e. either the initial one passed to the constructor or a
    // previous fast forward target).
    // pr needs to be valid until the reader is destroyed or fast_forward_to()
    // is called again.
    future<> fast_forward_to(const dht::partition_range& pr, db::timeout_clock::time_point timeout);
    // Skips to a later range of rows.
    // The new range must not overlap with the current range.
    //
    // In forwarding mode the stream does not return all fragments right away,
    // but only those belonging to the current clustering range. Initially
    // current range only covers the static row. The stream can be forwarded
    // (even before end-of- stream) to a later range with fast_forward_to().
    // Forwarding doesn't change initial restrictions of the stream, it can
    // only be used to skip over data.
    //
    // Monotonicity of positions is preserved by forwarding. That is fragments
    // emitted after forwarding will have greater positions than any fragments
    // emitted before forwarding.
    //
    // For any range, all range tombstones relevant for that range which are
    // present in the original stream will be emitted. Range tombstones
    // emitted before forwarding which overlap with the new range are not
    // necessarily re-emitted.
    //
    // When forwarding mode is not enabled, fast_forward_to()
    // cannot be used.
    future<> fast_forward_to(position_range cr, db::timeout_clock::time_point timeout);
    bool is_end_of_stream() const;
    bool is_buffer_empty() const;
    bool is_buffer_full() const;
    mutation_fragment pop_mutation_fragment();
    void unpop_mutation_fragment(mutation_fragment mf);
    const schema_ptr& schema() const;
    void set_max_buffer_size(size_t size);
    // Resolves with a pointer to the next fragment in the stream without consuming it from the stream,
    // or nullptr if there are no more fragments.
    // The returned pointer is invalidated by any other non-const call to this object.
    future<mutation_fragment*> peek(db::timeout_clock::time_point timeout);
    // A peek at the next fragment in the buffer.
    // Cannot be called if is_buffer_empty() returns true.
    const mutation_fragment& peek_buffer() const;
    // The actual buffer size of the reader.
    // Altough we consistently refer to this as buffer size throught the code
    // we really use "buffer size" as the size of the collective memory
    // used by all the mutation fragments stored in the buffer of the reader.
    size_t buffer_size() const;
    // Detach the internal buffer of the reader.
    // Roughly equivalent to depleting it by calling pop_mutation_fragment()
    // until is_buffer_empty() returns true.
    // The reader will need to allocate a new buffer on the next fill_buffer()
    // call.
    circular_buffer<mutation_fragment> detach_buffer();
    // Moves the buffer content to `other`.
    //
    // If the buffer of `other` is empty this is very efficient as the buffers
    // are simply swapped. Otherwise the content of the buffer is moved
    // fragmuent-by-fragment.
    // Allows efficient implementation of wrapping readers that do no
    // transformation to the fragment stream.
    void move_buffer_content_to(impl& other);

    // Causes this reader to conform to s.
    // Multiple calls of upgrade_schema() compose, effects of prior calls on the stream are preserved.
    void upgrade_schema(const schema_ptr& s);
};

using flat_mutation_reader_opt = optimized_optional<flat_mutation_reader>;

template<typename Impl, typename... Args>
flat_mutation_reader make_flat_mutation_reader(Args &&... args) {
    return flat_mutation_reader(std::make_unique<Impl>(std::forward<Args>(args)...));
}


// Creates a stream which is like r but with transformation applied to the elements.
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

/// Make a reader that enables the wrapped reader to work with multiple ranges.
///
/// \param ranges An range vector that has to contain strictly monotonic
///     partition ranges, such that successively calling
///     `flat_mutation_reader::fast_forward_to()` with each one is valid.
///     An range vector range with 0 or 1 elements is also valid.
/// \param fwd_mr It is only respected when `ranges` contains 0 or 1 partition
///     ranges. Otherwise the reader is created with
///     mutation_reader::forwarding::yes.
flat_mutation_reader
make_flat_multi_range_reader(schema_ptr s, mutation_source source, const dht::partition_range_vector& ranges,
                             const query::partition_slice& slice, const io_priority_class& pc = default_priority_class(),
                             tracing::trace_state_ptr trace_state = nullptr,
                             flat_mutation_reader::partition_range_forwarding fwd_mr = flat_mutation_reader::partition_range_forwarding::yes);

/// Make a reader that enables the wrapped reader to work with multiple ranges.
///
/// Generator overload. The ranges returned by the generator have to satisfy the
/// same requirements as the `ranges` param of the vector overload.
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

// Calls the consumer for each element of the reader's stream until end of stream
// is reached or the consumer requests iteration to stop by returning stop_iteration::yes.
// The consumer should accept mutation as the argument and return stop_iteration.
// The returned future<> resolves when consumption ends.
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

/// A reader that emits partitions in reverse.
///
/// 1. Static row is still emitted first.
/// 2. Range tombstones are ordered by their end position.
/// 3. Clustered rows and range tombstones are emitted in descending order.
/// Because of 2 and 3 the guarantee that a range tombstone is emitted before
/// any mutation fragment affected by it still holds.
/// Ordering of partitions themselves remains unchanged.
///
/// \param original the reader to be reversed, has to be kept alive while the
///     reversing reader is in use.
/// \param max_memory_consumption the maximum amount of memory the reader is
///     allowed to use for reversing. The reverse reader reads entire partitions
///     into memory, before reversing them. Since partitions can be larger than
///     the available memory, we need to enforce a limit on memory consumption.
///     If the read uses more memory then this limit, the read is aborted.
///
/// FIXME: reversing should be done in the sstable layer, see #1413.
flat_mutation_reader
make_reversing_reader(flat_mutation_reader& original, size_t max_memory_consumption);
