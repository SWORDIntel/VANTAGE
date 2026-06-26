#include "lsm_tree.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdexcept>
#include <cstring>

namespace {
    void deallocate_memtable_shm(hybrid_db::MemTable* ptr) {
        if (ptr) {
            ptr->~MemTable();
            munmap(ptr, sizeof(hybrid_db::MemTable));
        }
    }

    std::shared_ptr<hybrid_db::MemTable> create_shm_memtable() {
        unlink("/dev/shm/sentinel_active_session");
        int fd = open("/dev/shm/sentinel_active_session", O_CREAT | O_RDWR, 0666);
        if (fd == -1) throw std::runtime_error("Failed to open shm file");
        if (ftruncate(fd, sizeof(hybrid_db::MemTable)) == -1) {
            close(fd);
            throw std::runtime_error("Failed to truncate shm file");
        }
        void* ptr = mmap(nullptr, sizeof(hybrid_db::MemTable), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        close(fd);
        if (ptr == MAP_FAILED) throw std::runtime_error("Failed to mmap shm file");
        return std::shared_ptr<hybrid_db::MemTable>(new (ptr) hybrid_db::MemTable(), deallocate_memtable_shm);
    }
}

namespace hybrid_db {

void MemTable::put(const std::string& key, const std::string& value) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    auto it = table_.find(key);
    if (it != table_.end()) {
        current_size_ -= it->second.size();
        current_size_ += value.size();
        it->second = value;
    } else {
        current_size_ += key.size() + value.size();
        table_[key] = value;
    }
}

std::optional<std::string> MemTable::get(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    auto it = table_.find(key);
    if (it != table_.end()) {
        return it->second;
    }
    return std::nullopt;
}

size_t MemTable::approximate_size() const {
    return current_size_.load();
}

void MemTable::clear() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    table_.clear();
    current_size_ = 0;
}

void MemTable::commit_to_zfs_disk(const char* zfs_dataset_path) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    std::string buffer;
    size_t num_entries = table_.size();
    buffer.append(reinterpret_cast<const char*>(&num_entries), sizeof(num_entries));
    for (const auto& [k, v] : table_) {
        size_t k_size = k.size();
        buffer.append(reinterpret_cast<const char*>(&k_size), sizeof(k_size));
        buffer.append(k.data(), k_size);
        size_t v_size = v.size();
        buffer.append(reinterpret_cast<const char*>(&v_size), sizeof(v_size));
        buffer.append(v.data(), v_size);
    }
    
    const size_t ONE_MB = 1048576;
    size_t remainder = buffer.size() % ONE_MB;
    if (remainder != 0) {
        buffer.append(ONE_MB - remainder, '\0');
    }
    
    int fd = open(zfs_dataset_path, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (fd != -1) {
        const char* data = buffer.data();
        size_t to_write = buffer.size();
        size_t written = 0;
        while (written < to_write) {
            ssize_t res = write(fd, data + written, to_write - written);
            if (res < 0) {
                if (errno == EINTR) continue;
                break;
            }
            written += res;
        }
        close(fd);
    }
}

MemTableManager::MemTableManager(size_t max_memtable_size)
    : active_memtable_(create_shm_memtable()), max_memtable_size_(max_memtable_size) {}

void MemTableManager::put(const std::string& key, const std::string& value) {
    std::shared_ptr<MemTable> current_active;
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        current_active = active_memtable_;
    }
    
    current_active->put(key, value);
    maybe_flush();
}

std::optional<std::string> MemTableManager::get(const std::string& key) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
    if (auto val = active_memtable_->get(key)) {
        return val;
    }
    
    // Search in immutable memtables from newest to oldest
    for (auto it = immutable_memtables_.rbegin(); it != immutable_memtables_.rend(); ++it) {
        if (auto val = (*it)->get(key)) {
            return val;
        }
    }
    
    return std::nullopt;
}

void MemTableManager::maybe_flush() {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    if (active_memtable_->approximate_size() >= max_memtable_size_) {
        // Move active to immutable
        immutable_memtables_.push_back(active_memtable_);
        // Create new active memtable
        active_memtable_ = create_shm_memtable();
        
        // In a real implementation, we would trigger an asynchronous background thread
        // here to flush the immutable_memtables_ to SSTables on disk.
    }
}

} // namespace hybrid_db
