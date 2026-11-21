#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FControlRigObjectVersion {
    static constexpr FGuid GUID{0xA7820CFB, 0x20A74359, 0x8C542C14, 0x9623CF50};
    enum Type : int32 {
        SwitchedToRigVM = 3,
        StoreFunctionsInGeneratedClass = 24,
        OverridesStoreTOCDataForProperties = 40,
        OverridesStoreDatSkipOffsetAsInt64 = 41,
        OverridesStorePathAndLeafPropertyOnly = 42,
        OverridesStoreLeafPropertyHashOnly = 43,
    };
};

}
