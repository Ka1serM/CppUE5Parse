#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

namespace ue5parse::runtime {

struct FKeyHandleMap {
    uint8 Unused = 0;
    void Empty() {}
    int32 Num() const { return 0; }
};

}
