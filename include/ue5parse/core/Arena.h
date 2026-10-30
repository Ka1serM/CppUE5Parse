#pragma once

#include <cstddef>
#include <memory_resource>
#include <span>
#include <string_view>
#include <type_traits>

namespace ue5parse {

// Bump allocation in large blocks, freed all at once. Only trivially destructible types, so that freeing never needs
// to run destructors. One thread at a time.
class Arena {
public:
    explicit Arena(std::size_t initialSize = 64 * 1024) : resource(initialSize) {}
    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    template <class T>
        requires std::is_trivially_destructible_v<T>
    std::span<T> allocate(std::size_t count) {
        if (count == 0) return {};
        return {static_cast<T*>(resource.allocate(count * sizeof(T), alignof(T))), count};
    }

    std::span<std::byte> allocateBytes(std::size_t size, std::size_t alignment = 16) {
        if (size == 0) return {};
        return {static_cast<std::byte*>(resource.allocate(size, alignment)), size};
    }

    std::string_view copy(std::string_view text) {
        std::span<char> storage = allocate<char>(text.size());
        for (std::size_t i = 0; i < text.size(); i++) storage[i] = text[i];
        return {storage.data(), storage.size()};
    }

    void reset() { resource.release(); }

private:
    std::pmr::monotonic_buffer_resource resource;
};

}
