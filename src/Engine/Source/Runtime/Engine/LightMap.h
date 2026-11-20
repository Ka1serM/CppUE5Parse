#pragma once

#include <memory>

#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

namespace ue5parse::runtime {

class ULightMapTexture2D;

class FLightMap {
public:
    enum : uint32 { LMT_None = 0, LMT_1D = 1, LMT_2D = 2 };

    TArray<FGuid> LightGuids;
};

class FLightMap2D : public FLightMap {
public:
    static constexpr int32 NUM_STORED_LIGHTMAP_COEF = 4;

    TObjectPtr<ULightMapTexture2D> Textures[2];
    TObjectPtr<ULightMapTexture2D> SkyOcclusionTexture;
    TObjectPtr<ULightMapTexture2D> AOMaterialMaskTexture;
    TObjectPtr<UObject> VirtualTextures[2];
    FVector4f ScaleVectors[NUM_STORED_LIGHTMAP_COEF];
    FVector4f AddVectors[NUM_STORED_LIGHTMAP_COEF];
    FVector2D CoordinateScale;
    FVector2D CoordinateBias;
    bool bShadowChannelValid[4] = {};
    FVector4f InvUniformPenumbraSize;

    void Serialize(FArchive& Ar);

private:
    void SerializeVirtualTextures(FArchive& Ar);
};

using FLightMapRef = std::shared_ptr<FLightMap2D>;

FArchive& operator<<(FArchive& Ar, FLightMapRef& LightMap);

}
