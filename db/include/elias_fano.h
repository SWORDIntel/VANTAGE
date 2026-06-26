#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cmath>

namespace hybrid_db {

class EliasFano {
public:
    EliasFano(const std::vector<uint64_t>& data);

    uint64_t get(size_t index) const;
    size_t size() const;

private:
    size_t num_elements_;
    uint64_t max_value_;
    size_t lower_bits_;

    std::vector<uint64_t> lower_bits_array_;
    std::vector<uint64_t> upper_bits_array_;

    void build(const std::vector<uint64_t>& data);
    uint64_t read_lower_bits(size_t index) const;
};

} // namespace hybrid_db
