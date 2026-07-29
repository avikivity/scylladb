---
applyTo: "**"
---

# ScyllaDB Code Review

Apply these instructions when reviewing changes: your own before committing
or pushing, a branch or commit range, or a pull request. When writing code,
use them as a guide to what review will check.

These are additional checks. They don't replace the reviewing tool's own
practices for correctness, performance, and readability, nor its process for
choosing what to review and how to report findings.

## Prerequisites: load project conventions

Read the relevant instruction files before reviewing (do not load all at once):

- **Always:** `.github/copilot-instructions.md` — build/test commands, code &
  test philosophy, new-file rules (SPDX header, license ref).
- **Always:** `docs/dev/review-checklist.md`.
- **If the diff touches `*.cc` / `*.hh`:** `.github/instructions/cpp.instructions.md`
  — memory management, Seastar patterns, error handling, naming, formatting,
  forbidden constructs.
- **If the diff touches `*.py`:** `.github/instructions/python.instructions.md`.

These files are the source of truth for style. This file only adds review
emphasis on top of them.

## Review dimensions

Consider at least:

### Exposition and collateral
- Why vs what: each commit should explain why it is doing what it does, not just
  what (unless trivial).
- Note: single-patch series lose the cover letter when getting merged, so the commit
  message should include everything.
- Series title and patch titles should be descriptive, in imperative present tense.
- Series description (cover letter) should contain examples (for user-facing changes)
  and performance measurements (for hot-path changes). Performance collateral can include
  perf-simple-query run in relevant mode (or other microbenchmark). Prefer insn/op to
  throughput to isolate CPU frequency fluctuations. Note: ignore this for in-progress
  work that does not yet have a cover letter.
- New tests should come with a declaration of how long they take to run, and something
  about their stability.
- Changes that affect compatibility should explain how mixed-cluster operation and
  downgrades are supported.
- The amount of detail in cover letters and patch changelogs should be related to the
  complexity of the patch. A simple fix doesn't need a full page of exposition. Note
  it's okay and desirable to a one-liner that fixes a complicated problem to have a long
  explanation, and for a long but boilerplate patch to have a short explanation; it
  needs to be related to complexity not length. This also applies to comments, long
  comments are okay when some subtlety needs to be explained but otherwise avoid
  trivialities.

### Compatibility
- Maintain dialect compatibility for CQL and Alternator with previous versions.
- Maintain mixed-version compatibility: IDL should be extended, not changed. New verbs
  should be guarded by cluster features. The guards should be at the entry point: the entry
  point checks cluster feature and sets a per-request flag to use the new feature, rather
  than every node checking for itself.
- Maintain disk-format compatibility, and system-schema compatibility.
- CQL should maintain compatibility with Cassandra where possible, and keep the spirit
  of SQL as much as possible. Where Cassandra made unreasonable choices in obscure corner
  cases, it is okay to diverge.

### Documentation
- New CQL features should be documented in the CQL reference and in docs/cql/cql-extensions.md.
- Protocol extensions should be documented in docs/dev/protocol-extensions.md.
- Any user-facing feature needs to be documented in user-visible documentation in docs/.

### Correctness
- Logic errors, off-by-one, missing error paths.
- Exception safety across `co_await` suspension points; futures not discarded. Every asynchronous
  operation is eventually waited on (for example by service stop).
- Cross-shard safety: no shared mutable state without message passing; correct
  use of `foreign_ptr`.
- Resource accounting: limit memory consumption of asynchronous operations, by limiting concurrency
  (in simple cases) or accounting for resources (in complex cases with high variability).
- Bisectability: does each commit build and pass its own tests? When a series has
  several commits, review each one on its own; verify by inspection.
- Use on_internal_error() for recoverable errors where some non-data-path action encountered
  inconsistent state and needs to bail out, but if it's not executed again no harm is done.
  Use SCYLLA_ASSERT() where invariants are broken in the data path and there is no hope of
  recovery, and data corruption is possible.

### Performance (weigh heavily in hot paths — data read/write, inner loops)
- Unnecessary copies of large objects; missing `const&` / `&&` / `string_view`.
- Containers that might host large amounts of data should use chunked variants
  to avoid exceeding 128kB allocations. Similarly managed_bytes vs contiguous blobs.
- Reactor stalls: long loops without `maybe_yield()`; blocking calls.
- "Expected exceptions" - e.g. timeouts. Should use boost::outcome or std::expected
  (as determined by surrounding code, with preference to std::expected). This includes
  both future-returning and non-future-returning functions.
- A plain `co_await` (including `co_return co_await`) rethrows a failed future as a C++
  exception. On the read, write, or query path, or in a function newly converted to a
  coroutine, flag a plain `co_await` of a future that can fail with an expected error
  (e.g. a timeout) when the surrounding code uses `coroutine::try_future`, `as_future`, or
  `coroutine::exception`. Suggest `coroutine::try_future(...)` and running the
  exception-counting tests (e.g. `database_test/replica_read_timeout_no_exception`).
- While typically schemas are simple, need to support schemas with a moderate amount of
  use defined types and collections, with moderate nesting (1-2). The focus for complicated
  schemas should be on correctness, not performance.

### Cluster management
- The design point is hundreds of nodes, ten thousand shards, hundreds of thousands of
  tablets, hundreds of keyspaces and tables (total). Memory consumption for metadata should
  not affect a shard's normal role as a coordinator/replica shard.
- Every cluster operation should be recoverable. Register intent to perform, push the
  operation forward, mark the operation as complete. A failover to another leader will
  continue the work. The old leader will be fenced away.
- New configuration items should use the cluster config facility (docs/cql/cluster-config.rst),
  not scylla.yaml. The cluster should respond immediately to configuration changes without
  requiring a restart.

### Observability
- Major features need observability, as virtual tables, nodetool commands, or metrics
  (or a combination) depending on whether state evolution or current complex state
  needs to be presented. Metrics are an expensive resource so need to balance metric
  count.

### Build
- Keep header entanglements to a minimum.
- Keep configure.py and cmake build systems in sync.
- Use forward declarations to reduce rebuilds.
- Avoid implementations in headers for anything but trivial functions. LTO/PGO allows
  inlining across translation units.
- Avoid layering violations (note: a module layering document will be provided later).
- External dependencies should be pinned to a specific version (if downloaded dynamically)
  or frozen in the frozen toolchain image (if installed from Fedora).
- Avoid macros, with a view to future conversion of the tree to C++20 modules.

### Tests
- Bug fixes include a regression test that fails before / passes after, with the
  GitHub/JIRA issue referenced in a comment.
- New tests are stable (run `--repeat 100 --max-failures 1`), quick, no sleeps,
  deterministic (no unseeded random), minimal resources (single node if enough).
- Debug-mode iteration counts reduced where needed.

### New-file hygiene
- SPDX header with `LicenseRef-ScyllaDB-Source-Available-1.1`, current year.
- Files imported from other projects should retain attribution and license, add
  entry to licenses/.

