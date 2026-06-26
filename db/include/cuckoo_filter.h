#ifndef HYBRID_DB_CUCKOO_FILTER_H
#define HYBRID_DB_CUCKOO_FILTER_H

#include <vector>
#include <cstdint>
#include <string>

namespace hybrid_db {

class CuckooFilter {
public:
    explicit CuckooFilter(size_t capacity);
    ~CuckooFilter() = default;

    // Returns true if insertion succeeds, false if the filter is full.
    bool insert(const std::string& key);
    
    // Returns true if the filter might contain the key, false if it definitely does not.
    bool contains(const std::string& key) const;
    
    // Returns true if the key was successfully removed, false if not found.
    bool remove(const std::string& key);

private:
    struct Bucket {
        static constexpr size_t BUCKET_SIZE = 4;
        uint16_t fingerprints[BUCKET_SIZE] = {0};
    };

    std::vector<Bucket> buckets_;
    size_t num_buckets_;
    size_t num_items_;
    
    static constexpr size_t MAX_KICKS = 500;

    uint16_t get_fingerprint(const std::string& key) const;
    size_t hash(const std::string& key) const;
    size_t alt_hash(size_t index, uint16_t fingerprint) const;
};

} // namespace hybrid_db

#endif // HYBRID_DB_CUCKOO_FILTER_H
