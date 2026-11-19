#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FLightMapRef {
    uint32 LightMapType = 0;

    friend FArchive& operator<<(FArchive& Ar, FLightMapRef& LightMap) {
        Ar << LightMap.LightMapType;
        if (LightMap.LightMapType != 0)
            Ar.SetError(ErrorCode::UnsupportedVersion, "FLightMapRef.LightMapType");
        return Ar;
    }
};

}
