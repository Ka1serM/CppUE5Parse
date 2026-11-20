#pragma once

#include <cstddef>
#include <string>
#include <variant>

#include "Engine/Source/Runtime/Core/Misc/SecureHash.h"

namespace ue5parse::runtime {

struct FShaderHash {
    uint64 Hash = 0u;

    bool operator==(const FShaderHash&) const = default;
};

//! UE 5.8 identifies compiled shaders and shader maps by the 8-byte FShaderHash instead of the 20-byte FSHAHash.
using FShaderCodeHash = std::variant<FSHAHash, FShaderHash>;

FArchive& operator<<(FArchive& Ar, FShaderCodeHash& Value);
std::string ToString(const FShaderCodeHash& Value);

struct FShaderCodeHashHasher {
    std::size_t operator()(const FShaderCodeHash& Value) const;
};

}
