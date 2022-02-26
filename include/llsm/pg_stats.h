#pragma once

#include <cstdint>
#include <mutex>

namespace llsm {
namespace pg {

// This class stores counters used by the page-grouped LLSM database.
//
// This class only supports counters that can be aggregated across threads. The
// counters are meant to be manipulated using thread local instances and then
// aggregated when needed (e.g., when an experiment has completed).
class PageGroupedDBStats {
 public:
  static PageGroupedDBStats& Local() {
    static thread_local PageGroupedDBStats local;
    return local;
  }

  template <typename Callable>
  static void RunOnGlobal(const Callable& c) {
    std::unique_lock<std::mutex> lock(class_mutex_);
    c(global_);
  }

  PageGroupedDBStats(const PageGroupedDBStats&) = delete;
  PageGroupedDBStats& operator=(const PageGroupedDBStats&) = delete;

  uint64_t GetCacheHits() const { return cache_hits_; }
  uint64_t GetCacheMisses() const { return cache_misses_; }
  uint64_t GetCacheCleanEvictions() const { return cache_clean_evictions_; }
  uint64_t GetCacheDirtyEvictions() const { return cache_dirty_evictions_; }

  uint64_t GetOverflowsCreated() const { return overflows_created_; }
  uint64_t GetRewrites() const { return rewrites_; }
  uint64_t GetRewrittenPages() const { return rewritten_pages_; }

  uint64_t GetSegments() const { return segments_; }
  uint64_t GetFreeListEntries() const { return free_list_entries_; }
  uint64_t GetFreeListBytes() const { return free_list_bytes_; }
  uint64_t GetSegmentIndexBytes() const { return segment_index_bytes_; }
  uint64_t GetLockManagerBytes() const { return lock_manager_bytes_; }
  uint64_t GetCacheBytes() const { return cache_bytes_; }

  void BumpCacheHits() { ++cache_hits_; }
  void BumpCacheMisses() { ++cache_misses_; }
  void BumpCacheCleanEvictions() { ++cache_clean_evictions_; }
  void BumpCacheDirtyEvictions() { ++cache_dirty_evictions_; }

  void BumpOverflowsCreated() { ++overflows_created_; }
  void BumpRewrites() { ++rewrites_; }
  void BumpRewrittenPages(uint64_t delta = 1) { rewritten_pages_ += delta; }

  void SetSegments(uint64_t segments) { segments_ = segments; }
  void SetFreeListEntries(uint64_t entries) { free_list_entries_ = entries; }
  void SetFreeListBytes(uint64_t bytes) { free_list_bytes_ = bytes; }
  void SetSegmentIndexBytes(uint64_t bytes) { segment_index_bytes_ = bytes; }
  void SetLockManagerBytes(uint64_t bytes) { lock_manager_bytes_ = bytes; }
  void SetCacheBytes(uint64_t bytes) { cache_bytes_ = bytes; }

  // Threads must call this method to post their counter values to the global
  // `PageGroupedDBStats` instance.
  void PostToGlobal() const;
  void Reset();

 private:
  PageGroupedDBStats();

  static std::mutex class_mutex_;
  static PageGroupedDBStats global_;

  // Record-cache related counters.
  uint64_t cache_hits_;
  uint64_t cache_misses_;
  uint64_t cache_clean_evictions_;
  uint64_t cache_dirty_evictions_;

  // Segment rewrite related counters.
  uint64_t overflows_created_;
  uint64_t rewrites_;
  uint64_t rewritten_pages_;

  // Size-related stats. These are meant to be set once.
  uint64_t segments_;
  uint64_t free_list_entries_;
  uint64_t free_list_bytes_;
  uint64_t segment_index_bytes_;
  uint64_t lock_manager_bytes_;
  // The size footprint of the cache (in bytes).
  uint64_t cache_bytes_;
};

}  // namespace pg
}  // namespace llsm
