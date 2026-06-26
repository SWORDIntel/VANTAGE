#ifndef HYBRID_DB_TERNARY_SEARCH_H
#define HYBRID_DB_TERNARY_SEARCH_H

#include <vector>
#include <cstdint>
#include <cstddef>

namespace hybrid_db {

// A high-performance fallback search for sorted arrays.
class TernarySearch {
public:
    // Performs a vectorized or fast-branching ternary search on a sorted vector.
    // Returns the index of the target if found, or -1 if not found.
    static int64_t search(const std::vector<int64_t>& data, int64_t target);
    
    // Performs search on a raw array.
    static int64_t search(const int64_t* data, size_t size, int64_t target);
};

} // namespace hybrid_db

#endif // HYBRID_DB_TERNARY_SEARCH_H
