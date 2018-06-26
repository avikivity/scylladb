Protocol extensions to the Cassandra Native Protocol
====================================================

This document specifies extensions to the protocol defined
by Cassandra's native_protocol_v4.spec and native_protocol_v5.spec.
The extensions are designed so that a driver supporting them can
continue to interoperate with Cassandra and other compatible servers
with no configuration needed; the driver can discover the extensions
and enable them conditionally.

An extension can be discovered by using the OPTIONS request; the
returned SUPPORTED response will have zero or more options beginning
with SCYLLA indicating extensions defined in this documented, in
addition to options documented by Cassandra. How to the extension
is further explained in this document.

# Intranode sharding

This extension allows the driver to discover how Scylla internally
partitions data among logical cores. It can then create at least
one connection per logical core, and send queries directly to the
logical core that will serve them, greatly improving load balancing
and efficiency.

The extension is supported if the SCYLLA_INTRANODE_SHARDING key
is present in the [string map] returned by the SUPPORTED response
(the value can be ignored).

If the extension is reported as supported, the driver should send
a second OPTIONS request with a custom payload of type
`com.scylladb.ShardInfoRequest` and an empty value. The server will
respond with a normal SUPPORTED response with a custom payload of
type `com.scylladb.ShardInfoResponse` and a value encoded as
`<shard><partitioner><nr_shards><algorithm><params>`, where

  - `<shard>` is an [int], the zero-based shard number this connection
    is connected to;
  - `<partitioner>` is a [string] containing the fully-qualified name
    of the partitioner in use (i.e.
    `org.apache.cassandra.partitioners.Murmur3Partitioner`)l
  - `<nr_shards>` is an [int] number of local shards on this node; the
    relation `0 <= shard < nr_shards` holds;
  - `<algorithm>` is a [string] name of an algorithm used to select how
    partitions are mapped into shards (described below)
  - `<params>` is a [bytes] containing parameters to the algorithm (also
    described below)

Currently, only one `<algorithm>` is defined, `biased-token-round-robin`.
Its `<params>` structures is defined to be a single [int], called
`<ignore_msb>` (for most significant bits). To apply the algorithm,
perform the following steps:

  - subtract the minimum token value from the partition's token
    in order to bias it: `biased_token = token - (-2**63)`
  - shift `biased_token` left by `ignore_msb` bits, discarding any
    bits beyond the 63rd:
      `biased_token = (biased_token << ignore_msb) % (2**64)`
  - multiply by `nr_shards` and perform a truncating division by 2**64:
    `shard = (biased_token * nr_shards) / 2**64`

in C, these operations can be efficiently perfomed in three steps:

    uint64_t biased_token = token + ((uint64_t)1 << 63);
    biased_token <<= ignore_msb;
    int shard = ((unsigned __int128)biased_token * nr_shards) >> 64;

It is recommended that drivers open connections until they have at
least one connection per shard, then close excess connections.
