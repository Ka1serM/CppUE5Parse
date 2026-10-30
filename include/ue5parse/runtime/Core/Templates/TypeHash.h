#pragma once

#include <cstring>
#include <type_traits>

#include "ue5parse/runtime/Core/Containers/Array.h"
#include "ue5parse/runtime/Core/UObject/NameTypes.h"

// GetTypeHash of the plain types (UE's Templates/TypeHash.h) and HashCombine.

namespace ue5parse::runtime {

inline uint32 HashCombine(uint32 A, uint32 C) {
    uint32 B = 0x9e3779b9;
    A += B;
    A -= B; A -= C; A ^= (C >> 13);
    B -= C; B -= A; B ^= (A << 8);
    C -= A; C -= B; C ^= (B >> 13);
    A -= B; A -= C; A ^= (C >> 12);
    B -= C; B -= A; B ^= (A << 16);
    C -= A; C -= B; C ^= (B >> 5);
    A -= B; A -= C; A ^= (C >> 3);
    B -= C; B -= A; B ^= (A << 10);
    C -= A; C -= B; C ^= (B >> 15);
    return C;
}

template <class T>
    requires(std::is_integral_v<T> || std::is_enum_v<T>)
inline uint32 GetTypeHash(T Value) {
    if constexpr (sizeof(T) <= 4) {
        return static_cast<uint32>(Value);
    } else {
        uint64 Bits = static_cast<uint64>(Value);
        return static_cast<uint32>(Bits) + static_cast<uint32>(Bits >> 32) * 23;
    }
}
inline uint32 GetTypeHash(float Value) {
    uint32 Bits;
    std::memcpy(&Bits, &Value, 4);
    return Bits;
}
inline uint32 GetTypeHash(double Value) {
    uint64 Bits;
    std::memcpy(&Bits, &Value, 8);
    return GetTypeHash(Bits);
}
inline uint32 GetTypeHash(const void* Pointer) {
    return GetTypeHash(static_cast<uint64>(reinterpret_cast<uintptr_t>(Pointer)) >> 4);
}
template <class T>
inline uint32 GetTypeHash(T* Pointer) {
    return GetTypeHash(static_cast<const void*>(Pointer));
}
inline uint32 GetTypeHash(FName Name) { return static_cast<uint32>(FNameHasher()(Name)); }

}
