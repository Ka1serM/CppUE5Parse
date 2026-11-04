#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"

namespace ue5parse::runtime {

struct FRHIResourceTableEntry {
    static constexpr uint32 GetEndOfStreamToken() { return 0xffffffffu; }
    static constexpr uint16 GetUniformBufferIndex(uint32 Data) { return static_cast<uint16>((Data >> 24) & 0xffu); }
    static constexpr uint16 GetResourceIndex(uint32 Data) { return static_cast<uint16>((Data >> 8) & 0xffffu); }
    static constexpr uint16 GetBindIndex(uint32 Data) { return static_cast<uint16>(Data & 0xffu); }
};

}
