#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "gflags/gflags.h"
#include "llsm/options.h"
#include "llsm/pg_options.h"
#include "rocksdb/options.h"

// This header declares all the common configuration flags used across the LLSM
// benchmarks as well as a few utility functions that use these flags.

// Which database(s) to use in the benchmark {all, rocksdb, llsm, kvell,
// pg_llsm}.
DECLARE_string(db);

// The path where the database(s) should be stored.
DECLARE_string(db_path);

// The number of times to repeat the experiment.
DECLARE_uint32(trials);

// The seed any pseudorandom number generator should use (to ensure
// reproducibility).
DECLARE_uint32(seed);

// The size of the records in the benchmark dataset, in bytes.
DECLARE_uint32(record_size_bytes);

// The size of the database's in-memory cache, in MiB.
// For LLSM, this is the size of its buffer pool.
// For RocksDB, this is the size of its block cache.
DECLARE_uint64(cache_size_mib);

// The number background threads that the database can use.
DECLARE_uint32(bg_threads);

// Whether or not to use direct I/O.
DECLARE_bool(use_direct_io);

// The size of the memtable before it should be flushed, in MiB.
DECLARE_uint64(memtable_size_mib);

// How full each LLSM page should be, as a value between 1 and 100
// inclusive.
DECLARE_uint32(llsm_page_fill_pct);

// The minimum number of operations to a given page that need to be encoutered
// while flushing a memtable in order to trigger a flush.
DECLARE_uint64(io_threshold);

// The maximum number of times that a given operation can be deferred to a
// future flush.
DECLARE_uint64(max_deferrals);

// Whether or not to auto-tune deferral parameters
DECLARE_bool(deferral_autotuning);

// Whether or not to auto-tune memory allocation
DECLARE_bool(memory_autotuning);

// If true, all writes will bypass the write-ahead log.
DECLARE_bool(bypass_wal);

// If true, benchmark information will be printed to stderr.
DECLARE_bool(verbose);

// The number of requests between latency measurements (i.e., measure latency
// every N-th request).
DECLARE_uint32(latency_sample_period);

// The minimum length of an overflow chain for which reorganization is
// triggered.
DECLARE_uint64(reorg_length);

// If true, LLSM will use an ALEXModel. Otherwise, it will use a BTreeModel.
DECLARE_bool(use_alex);

// The number of bloom filter bits to use in RocksDB. Set to 0 to disable the
// use of bloom filters.
DECLARE_uint32(rdb_bloom_bits);

// The number of bytes to include in a prefix bloom filter. This is only used
// when bloom filters are enabled (see the flag above). Set to 0 to disable the
// use of prefix bloom filters.
DECLARE_uint32(rdb_prefix_bloom_size);

// Page grouping configuration options.
DECLARE_bool(pg_use_segments);
DECLARE_uint64(records_per_page_goal);
DECLARE_uint64(records_per_page_delta);
DECLARE_bool(pg_use_memory_based_io);
DECLARE_bool(pg_bypass_cache);

// If true, the record cache will try to batch writes for the same page when
// writing out a dirty entry.
DECLARE_bool(rec_cache_batch_writeout);

namespace llsm {
namespace bench {

// An enum that represents the `db` flag above.
enum class DBType : uint32_t {
  kAll = 0,
  kLLSM = 1,
  kRocksDB = 2,
  kLeanStore = 3,
  kKVell = 4,
  kPGLLSM = 5
};

// Returns the `DBType` enum value associated with a given string.
// - "all" maps to `kAll`
// - "llsm" maps to `kLLSM`
// - "rocksdb" maps to `kRocksDB`
// - "leanstore" maps to `kLeanStore`
// - "kvell" maps to `kKVell`
// - "pg_llsm" maps to `kPGLLSM`
// All other strings map to an empty `std::optional`.
std::optional<DBType> ParseDBType(const std::string& candidate);

// Returns options that can be used to start RocksDB with the configuration
// specified by the flags set above.
rocksdb::Options BuildRocksDBOptions();

// Returns options that can be used to start LLSM with the configuration
// specified by the flags set above.
llsm::Options BuildLLSMOptions();

// Returns options that can be used to start page-grouped LLSM with the
// configuration specified by the flags set above.
llsm::pg::PageGroupedDBOptions BuildPGLLSMOptions();

// Appends a human-readable timestamp to the provided `prefix` string.
// e.g.: AppendTimestamp("test") -> "test+2021-05-10+11-10-12".
// The first set of three numbers represent the date (YYYY-MM-DD). The second
// set of three numbers represent the local time (HH-MM-SS).
std::string AppendTimestamp(const std::string& prefix);

// Gets a default value for the --output_path flag. The returned value has a
// timestamp appended.
std::string GetDefaultOutputPath();

// Gets a default value for the --db_path flag. The returned value has a
// timestamp appended.
std::string GetDefaultDBPath();

}  // namespace bench
}  // namespace llsm
