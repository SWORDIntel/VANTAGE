#include "cuckoo_filter.h"
#include <functional>
#include <random>

namespace hybrid_db {

CuckooFilter::CuckooFilter(size_t capacity) {
    num_buckets_ = capacity / Bucket::BUCKET_SIZE;
    if (num_buckets_ == 0) num_buckets_ = 1;
    buckets_.resize(num_buckets_);
    num_items_ = 0;
}

uint16_t CuckooFilter::get_fingerprint(const std::string& key) const {
    std::hash<std::string> hasher;
    size_t h = hasher(key);
    uint16_t f = h & 0xFFFF;
    f += (f == 0); // fingerprint cannot be 0, as 0 indicates an empty slot
    return f;
}

size_t CuckooFilter::hash(const std::string& key) const {
    std::hash<std::string> hasher;
    return hasher(key) % num_buckets_;
}

size_t CuckooFilter::alt_hash(size_t index, uint16_t fingerprint) const {
    // A simple hash function for fingerprint to mix the bits
    size_t h = fingerprint * 0x5bd1e995;
    return (index ^ h) % num_buckets_;
}

bool CuckooFilter::insert(const std::string& key) {
    uint16_t f = get_fingerprint(key);
    size_t i1 = hash(key);
    size_t i2 = alt_hash(i1, f);

    // Try to insert into bucket i1
    for (size_t i = 0; i < Bucket::BUCKET_SIZE; ++i) {
        if (buckets_[i1].fingerprints[i] == 0) {
            buckets_[i1].fingerprints[i] = f;
            num_items_++;
            return true;
        }
    }

    // Try to insert into bucket i2
    for (size_t i = 0; i < Bucket::BUCKET_SIZE; ++i) {
        if (buckets_[i2].fingerprints[i] == 0) {
            buckets_[i2].fingerprints[i] = f;
            num_items_++;
            return true;
        }
    }

    // Must kick an existing item
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, 1);
    std::uniform_int_distribution<size_t> slot_dist(0, Bucket::BUCKET_SIZE - 1);

    size_t cur_i = dist(gen) ? i1 : i2;
    for (size_t n = 0; n < MAX_KICKS; ++n) {
        size_t slot = slot_dist(gen);
        uint16_t kicked_f = buckets_[cur_i].fingerprints[slot];
        buckets_[cur_i].fingerprints[slot] = f;
        
        f = kicked_f;
        cur_i = alt_hash(cur_i, f);

        for (size_t i = 0; i < Bucket::BUCKET_SIZE; ++i) {
            if (buckets_[cur_i].fingerprints[i] == 0) {
                buckets_[cur_i].fingerprints[i] = f;
                num_items_++;
                return true;
            }
        }
    }
    
    // Filter is full and max kicks exceeded
    return false;
}

bool CuckooFilter::contains(const std::string& key) const {
    uint16_t f = get_fingerprint(key);
    size_t i1 = hash(key);
    size_t i2 = alt_hash(i1, f);

    for (size_t i = 0; i < Bucket::BUCKET_SIZE; ++i) {
        if (buckets_[i1].fingerprints[i] == f) return true;
    }
    for (size_t i = 0; i < Bucket::BUCKET_SIZE; ++i) {
        if (buckets_[i2].fingerprints[i] == f) return true;
    }
    return false;
}

bool CuckooFilter::remove(const std::string& key) {
    uint16_t f = get_fingerprint(key);
    size_t i1 = hash(key);
    size_t i2 = alt_hash(i1, f);

    for (size_t i = 0; i < Bucket::BUCKET_SIZE; ++i) {
        if (buckets_[i1].fingerprints[i] == f) {
            buckets_[i1].fingerprints[i] = 0;
            num_items_--;
            return true;
        }
    }
    for (size_t i = 0; i < Bucket::BUCKET_SIZE; ++i) {
        if (buckets_[i2].fingerprints[i] == f) {
            buckets_[i2].fingerprints[i] = 0;
            num_items_--;
            return true;
        }
    }
    return false;
}

} // namespace hybrid_db
