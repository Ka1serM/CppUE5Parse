#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FControlRigObjectVersion {
    static constexpr FGuid GUID{0xA7820CFB, 0x20A74359, 0x8C542C14, 0x9623CF50};
    enum Type : int32 {
        SwitchedToRigVM = 3,
    };
};

}
