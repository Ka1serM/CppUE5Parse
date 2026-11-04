#pragma once

#include <string>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FSHAHash {
    uint8 Hash[20] = {};

    bool operator==(const FSHAHash&) const = default;
    std::string ToString() const;
};

inline FArchive& operator<<(FArchive& Ar, FSHAHash& Value) {
    Ar.Serialize(Value.Hash, sizeof(Value.Hash));
    return Ar;
}

struct FSHAHashHasher {
    std::size_t operator()(const FSHAHash& Value) const {
        uint64 Prefix = 0;
        std::memcpy(&Prefix, Value.Hash, sizeof(Prefix));
        return static_cast<std::size_t>(Prefix);
    }
};

}
