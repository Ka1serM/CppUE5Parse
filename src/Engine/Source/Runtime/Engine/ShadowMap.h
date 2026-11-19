#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FShadowMapRef {
    uint32 ShadowMapType = 0;

    friend FArchive& operator<<(FArchive& Ar, FShadowMapRef& ShadowMap) {
        Ar << ShadowMap.ShadowMapType;
        if (ShadowMap.ShadowMapType != 0)
            Ar.SetError(ErrorCode::UnsupportedVersion, "FShadowMapRef.ShadowMapType");
        return Ar;
    }
};

}
