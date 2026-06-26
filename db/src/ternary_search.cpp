#include "ternary_search.h"

namespace hybrid_db {

int64_t TernarySearch::search(const std::vector<int64_t>& data, int64_t target) {
    return search(data.data(), data.size(), target);
}

int64_t TernarySearch::search(const int64_t* data, size_t size, int64_t target) {
    if (size == 0) return -1;

    int64_t left = 0;
    int64_t right = size - 1;

    // Fast-branching ternary search splitting the array into 3 partitions.
    while (left <= right) {
        int64_t third = (right - left) / 3;
        int64_t mid1 = left + third;
        int64_t mid2 = right - third;

        int64_t val1 = data[mid1];
        int64_t val2 = data[mid2];

        if (target == val1) {
            return mid1;
        }
        if (target == val2) {
            return mid2;
        }

        if (target < val1) {
            // Target is in the first third
            right = mid1 - 1;
        } else if (target > val2) {
            // Target is in the third third
            left = mid2 + 1;
        } else {
            // Target is in the middle third
            left = mid1 + 1;
            right = mid2 - 1;
        }
    }

    return -1; // Not found
}

} // namespace hybrid_db
