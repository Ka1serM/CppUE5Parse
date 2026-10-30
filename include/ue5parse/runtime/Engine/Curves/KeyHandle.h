#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"

// Key handles (UE's Curves/KeyHandle.h): editor-only bookkeeping of curve keys. Cooked data has none, so the map is
// empty: one byte in memory, registered as an opaque native struct of size 0 in cooked data.

namespace ue5parse::runtime {

struct FKeyHandleMap {
    uint8 Unused = 0;
    void Empty() {}
    int32 Num() const { return 0; }
};

}
