#include "elias_fano.h"
#include <stdexcept>
#include <algorithm>

namespace hybrid_db {

EliasFano::EliasFano(const std::vector<uint64_t>& data) {
    if (data.empty()) {
        num_elements_ = 0;
        max_value_ = 0;
        lower_bits_ = 0;
        return;
    }
    build(data);
}

void EliasFano::build(const std::vector<uint64_t>& data) {
    num_elements_ = data.size();
    max_value_ = data.back();

    if (max_value_ == 0) {
        lower_bits_ = 0;
    } else {
        lower_bits_ = std::max(0, static_cast<int>(std::log2(static_cast<double>(max_value_) / num_elements_)));
    }

    size_t lower_bits_mask = (1ULL << lower_bits_) - 1;
    
    // Store lower bits
    size_t total_lower_bits = num_elements_ * lower_bits_;
    size_t lower_array_size = (total_lower_bits + 63) / 64;
    lower_bits_array_.resize(lower_array_size, 0);

    // Store upper bits (unary)
    size_t num_upper_buckets = (max_value_ >> lower_bits_) + 1;
    size_t upper_array_size = (num_elements_ + num_upper_buckets + 63) / 64;
    upper_bits_array_.resize(upper_array_size, 0);

    size_t lower_bit_pos = 0;
    size_t upper_bit_pos = 0;
    uint64_t current_bucket = 0;

    for (size_t i = 0; i < num_elements_; ++i) {
        uint64_t val = data[i];
        
        // Lower bits
        if (lower_bits_ > 0) {
            uint64_t lower_val = val & lower_bits_mask;
            size_t word_idx = lower_bit_pos / 64;
            size_t bit_idx = lower_bit_pos % 64;
            
            lower_bits_array_[word_idx] |= (lower_val << bit_idx);
            if (bit_idx + lower_bits_ > 64) {
                lower_bits_array_[word_idx + 1] |= (lower_val >> (64 - bit_idx));
            }
            lower_bit_pos += lower_bits_;
        }

        // Upper bits
        uint64_t upper_val = val >> lower_bits_;
        while (current_bucket < upper_val) {
            upper_bit_pos++; // 0 bit for bucket transition
            current_bucket++;
        }
        
        // 1 bit for element
        size_t word_idx = upper_bit_pos / 64;
        size_t bit_idx = upper_bit_pos % 64;
        upper_bits_array_[word_idx] |= (1ULL << bit_idx);
        upper_bit_pos++;
    }
}

uint64_t EliasFano::read_lower_bits(size_t index) const {
    if (lower_bits_ == 0) return 0;
    
    size_t bit_pos = index * lower_bits_;
    size_t word_idx = bit_pos / 64;
    size_t bit_idx = bit_pos % 64;
    
    uint64_t mask = (1ULL << lower_bits_) - 1;
    uint64_t result = lower_bits_array_[word_idx] >> bit_idx;
    
    if (bit_idx + lower_bits_ > 64) {
        result |= (lower_bits_array_[word_idx + 1] << (64 - bit_idx));
    }
    
    return result & mask;
}

uint64_t EliasFano::get(size_t index) const {
    if (index >= num_elements_) {
        throw std::out_of_range("Index out of bounds");
    }

    uint64_t lower = read_lower_bits(index);

    // Find the index-th 1 in upper_bits_array_
    size_t ones_count = 0;
    size_t bit_pos = 0;
    
    for (size_t i = 0; i < upper_bits_array_.size(); ++i) {
        uint64_t word = upper_bits_array_[i];
        int ones_in_word = __builtin_popcountll(word);
        
        if (ones_count + ones_in_word > index) {
            // The 1 we are looking for is in this word
            for (int b = 0; b < 64; ++b) {
                if ((word >> b) & 1) {
                    if (ones_count == index) {
                        bit_pos += b;
                        break;
                    }
                    ones_count++;
                }
            }
            break;
        } else {
            ones_count += ones_in_word;
            bit_pos += 64;
        }
    }

    uint64_t upper = bit_pos - index;
    return (upper << lower_bits_) | lower;
}

size_t EliasFano::size() const {
    return num_elements_;
}

} // namespace hybrid_db
