#include "picker.h"
#include "logger.h"

#include <chrono>
#include <stdexcept>
#include <algorithm>

namespace picker {

RandomPicker::RandomPicker() {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto rnd = (static_cast<std::uint64_t>(rd_()) << 32) ^ static_cast<std::uint64_t>(rd_());
    seed_ = static_cast<std::uint64_t>(now) ^ rnd;
    if (seed_ == 0) seed_ = 1;
}

RandomPicker::RandomPicker(std::uint64_t seed) : seed_(seed == 0 ? 1 : seed) {}

void RandomPicker::reseed() {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    auto rnd = (static_cast<std::uint64_t>(rd_()) << 32) ^ static_cast<std::uint64_t>(rd_());
    seed_ = static_cast<std::uint64_t>(now) ^ rnd;
    if (seed_ == 0) seed_ = 1;
}

void RandomPicker::reseed(std::uint64_t seed) {
    seed_ = seed == 0 ? 1 : seed;
}

std::vector<std::size_t> RandomPicker::sampleIndices(std::size_t total, std::size_t k) const {
    if (total == 0) return {};
    if (k >= total) {
        std::vector<std::size_t> all(total);
        for (std::size_t i = 0; i < total; ++i) all[i] = i;
        return all;
    }

    std::seed_seq seq{static_cast<std::uint32_t>(seed_ & 0xFFFFFFFFu),
                      static_cast<std::uint32_t>((seed_ >> 32) & 0xFFFFFFFFu),
                      static_cast<std::uint32_t>(rd_()),
                      static_cast<std::uint32_t>(rd_())};
    (void)seq;
    std::mt19937_64 gen(seed_);

    std::vector<std::size_t> pool(total);
    for (std::size_t i = 0; i < total; ++i) pool[i] = i;
    std::shuffle(pool.begin(), pool.end(), gen);
    pool.resize(k);
    std::sort(pool.begin(), pool.end());
    return pool;
}

std::vector<Person> RandomPicker::sample(const std::vector<Person>& pool, std::size_t k) const {
    auto idxs = sampleIndices(pool.size(), k);
    std::vector<Person> out;
    out.reserve(idxs.size());
    for (auto i : idxs) out.push_back(pool[i]);
    return out;
}

}
