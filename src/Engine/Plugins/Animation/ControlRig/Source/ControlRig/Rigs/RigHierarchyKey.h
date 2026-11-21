#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FRigElementKey {
    FName Type;
    FName Name;

    friend FArchive& operator<<(FArchive& Ar, FRigElementKey& Key) {
        return Ar << Key.Type << Key.Name;
    }
};

}
