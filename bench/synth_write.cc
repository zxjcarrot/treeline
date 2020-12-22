#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "bench/data.h"
#include "bench/guard.h"
#include "bench/timing.h"
#include "gflags/gflags.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"
#include "rocksdb/table.h"
#include "rocksdb/write_batch.h"

namespace fs = std::filesystem;

namespace {

static const std::string kAll = "all";
static const std::string kRocksDB = "rocksdb";
static const std::string kLLSM = "llsm";

DEFINE_string(db, kAll, "Which database(s) to use {all, rocksdb, llsm}.");
DEFINE_string(db_path, "", "The path where the database(s) should be stored.");
DEFINE_uint32(trials, 1, "The number of times to repeat the experiment.");

DEFINE_uint64(data_mib, 64, "The amount of user data to write, in MiB.");
DEFINE_uint64(cache_size_mib, 64,
              "The size of the database's in memory cache, in MiB.");
DEFINE_uint64(block_size_kib, 64, "The size of a block, in KiB.");
DEFINE_uint64(write_batch_size, 1024,
              "The size of a write batch used by RocksDB.");
DEFINE_uint32(bg_threads, 1,
              "The number background threads that RocksDB/LLSM should use.");
DEFINE_bool(use_direct_io, true, "Whether or not to use direct I/O.");

std::chrono::nanoseconds RunRocksDBExperiment(
    const llsm::bench::U64Dataset& dataset) {
  rocksdb::DB* db = nullptr;
  rocksdb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true;
  options.compression = rocksdb::CompressionType::kNoCompression;
  options.use_direct_reads = FLAGS_use_direct_io;
  options.use_direct_io_for_flush_and_compaction = FLAGS_use_direct_io;
  options.PrepareForBulkLoad();
  options.IncreaseParallelism(FLAGS_bg_threads);

  rocksdb::BlockBasedTableOptions table_options;
  table_options.block_size = FLAGS_block_size_kib * 1024;
  table_options.checksum = rocksdb::kNoChecksum;
  options.table_factory.reset(
      rocksdb::NewBlockBasedTableFactory(table_options));

  const std::string dbname = FLAGS_db_path + "/rocksdb";
  rocksdb::Status status = rocksdb::DB::Open(options, dbname, &db);
  if (!status.ok()) {
    throw std::runtime_error("Failed to open RocksDB: " + status.ToString());
  }

  const llsm::bench::CallOnExit guard([db]() { delete db; });
  return llsm::bench::MeasureRunTime([db, &dataset]() {
    rocksdb::WriteOptions woptions;
    woptions.disableWAL = true;
    rocksdb::Status status;
    rocksdb::WriteBatch batch;
    for (const auto& record : dataset) {
      status = batch.Put(
          rocksdb::Slice(record.key().data(), record.key().size()),
          rocksdb::Slice(record.value().data(), record.value().size()));
      if (!status.ok()) {
        throw std::runtime_error("Failed to write record to RocksDB instance.");
      }
      if (batch.Count() >= FLAGS_write_batch_size) {
        status = db->Write(woptions, &batch);
        if (!status.ok()) {
          throw std::runtime_error("Failed to write batch to RocksDB.");
        }
        batch.Clear();
      }
    }
    if (batch.Count() > 0) {
      status = db->Write(woptions, &batch);
      if (!status.ok()) {
        throw std::runtime_error("Failed to write batch to RocksDB.");
      }
    }
    // Ensure any remaining in-memory data is flushed to disk
    const rocksdb::FlushOptions foptions;
    status = db->Flush(foptions);
    if (!status.ok()) {
      throw std::runtime_error("Failed to flush memtable at the end.");
    }
  });
}

std::chrono::nanoseconds RunLLSMExperiment(
    const llsm::bench::U64Dataset& dataset) {
  return std::chrono::nanoseconds(0);
}

void PrintExperimentResult(const std::string& db,
                           std::chrono::nanoseconds run_time) {
  const std::chrono::duration<double> run_time_s = run_time;
  const double throughput_mib_per_s = FLAGS_data_mib / run_time_s.count();
  std::cout << db << "," << FLAGS_data_mib << "," << FLAGS_bg_threads << ","
            << throughput_mib_per_s << std::endl;
}

}  // namespace

int main(int argc, char* argv[]) {
  gflags::SetUsageMessage(
      "Measure the write throughput when writing a fixed amount of synthetic "
      "data into an empty database.");
  gflags::ParseCommandLineFlags(&argc, &argv, /* remove_flags */ true);
  if (FLAGS_data_mib == 0) {
    std::cerr << "ERROR: --data_mib must be greater than 0." << std::endl;
    return 1;
  }
  if (FLAGS_db_path.empty()) {
    std::cerr << "ERROR: --db_path must be specified." << std::endl;
    return 1;
  }
  if (fs::exists(FLAGS_db_path)) {
    std::cerr << "ERROR: The provided --db_path already exists." << std::endl;
    return 1;
  }

  const llsm::bench::U64Dataset dataset =
      llsm::bench::GenerateOrderedData(FLAGS_data_mib);

  std::cout << "db,data_size_mib,bg_threads,throughput_mib_per_s" << std::endl;

  for (uint32_t i = 0; i < FLAGS_trials; ++i) {
    fs::create_directory(FLAGS_db_path);

    if (FLAGS_db == kAll || FLAGS_db == kRocksDB) {
      PrintExperimentResult(kRocksDB, RunRocksDBExperiment(dataset));
    }
    if (FLAGS_db == kAll || FLAGS_db == kLLSM) {
      PrintExperimentResult(kLLSM, RunLLSMExperiment(dataset));
    }

    fs::remove_all(FLAGS_db_path);
  }

  return 0;
}
