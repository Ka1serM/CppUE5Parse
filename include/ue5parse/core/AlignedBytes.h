#pragma once

#include <cstddef>
#include <new>
#include <vector>

namespace ue5parse {

template <class T, std::size_t Alignment>
struct AlignedAllocator {
    using value_type = T;
    template <class U>
    struct rebind {
        using other = AlignedAllocator<U, Alignment>;
    };

    AlignedAllocator() = default;
    template <class U>
    AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}

    T* allocate(std::size_t count) { return static_cast<T*>(::operator new(count * sizeof(T), std::align_val_t(Alignment))); }
    void deallocate(T* data, std::size_t) noexcept { ::operator delete(data, std::align_val_t(Alignment)); }
    bool operator==(const AlignedAllocator&) const = default;
};

// Bytes at a 16-byte aligned address, for data that is read in place with SIMD loads, such as ACL's compressed tracks.
using AlignedBytes = std::vector<std::byte, AlignedAllocator<std::byte, 16>>;

}
