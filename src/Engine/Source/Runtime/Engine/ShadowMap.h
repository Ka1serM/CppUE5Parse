#pragma once

#include <memory>

#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

namespace ue5parse::runtime {

class UShadowMapTexture2D;

class FShadowMap {
public:
    enum : uint32 { SMT_None = 0, SMT_2D = 2 };

    TArray<FGuid> LightGuids;
};

class FShadowMap2D : public FShadowMap {
public:
    TObjectPtr<UShadowMapTexture2D> Texture;
    FVector2D CoordinateScale;
    FVector2D CoordinateBias;
    bool bChannelValid[4] = {};
    FVector4f InvUniformPenumbraSize;

    void Serialize(FArchive& Ar);
};

using FShadowMapRef = std::shared_ptr<FShadowMap2D>;

FArchive& operator<<(FArchive& Ar, FShadowMapRef& ShadowMap);

}
