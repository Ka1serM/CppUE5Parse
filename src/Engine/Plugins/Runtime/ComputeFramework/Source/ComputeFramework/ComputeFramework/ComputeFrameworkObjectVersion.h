#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FComputeFrameworkObjectVersion {
    static constexpr FGuid GUID{0x6304A3E7, 0x00594F59, 0x8CFC21BD, 0x7721FD4E};
    enum Type : int32 {
        InitialVersion = 0,
    };
};

}
