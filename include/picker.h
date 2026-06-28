#pragma once

#include "common.h"
#include <random>
#include <vector>
#include <cstddef>
#include <atomic>

namespace picker {

class RandomPicker {
public:
    RandomPicker();
    explicit RandomPicker(std::uint64_t seed);

    void reseed();
    void reseed(std::uint64_t seed);

    std::vector<Person> sample(const std::vector<Person>& pool, std::size_t k) const;
    std::vector<std::size_t> sampleIndices(std::size_t total, std::size_t k) const;

    std::uint64_t currentSeed() const { return seed_; }

private:
    std::uint64_t seed_;
    mutable std::random_device rd_;
};

}
