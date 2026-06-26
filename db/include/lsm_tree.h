#pragma once
#include <map>
#include <string>
#include <shared_mutex>
#include <mutex>
#include <optional>
#include <memory>
#include <atomic>
#include <vector>

namespace hybrid_db {

class MemTable {
public:
    MemTable() : current_size_(0) {}
    ~MemTable() = default;

    void put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;
    
    size_t approximate_size() const;
    void clear();

    void commit_to_zfs_disk(const char* zfs_dataset_path) const;

private:
    std::map<std::string, std::string> table_;
    mutable std::shared_mutex mutex_;
    std::atomic<size_t> current_size_;
};

class MemTableManager {
public:
    MemTableManager(size_t max_memtable_size = 64 * 1024 * 1024);
    
    void put(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;

private:
    void maybe_flush();

    std::shared_ptr<MemTable> active_memtable_;
    std::vector<std::shared_ptr<MemTable>> immutable_memtables_;
    mutable std::shared_mutex mutex_;
    size_t max_memtable_size_;
};

} // namespace hybrid_db
