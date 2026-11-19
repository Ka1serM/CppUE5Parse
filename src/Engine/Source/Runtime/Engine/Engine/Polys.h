#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "Polys.generated.h"

namespace ue5parse::runtime {

class ABrush;
class UMaterialInterface;

struct FPoly {
    FVector3f Base;
    FVector3f Normal;
    FVector3f TextureU;
    FVector3f TextureV;
    TArray<FVector3f> Vertices;
    uint32 PolyFlags = 0;
    TObjectPtr<ABrush> Actor = nullptr;
    FName ItemName;
    TObjectPtr<UMaterialInterface> Material = nullptr;
    int32 iLink = INDEX_NONE;
    int32 iBrushPoly = INDEX_NONE;
    float LightMapScale = 32.0f;
    FLightmassPrimitiveSettings LightmassSettings;
    FName RulesetVariation;

    friend FArchive& operator<<(FArchive& Ar, FPoly& Poly);
};

UCLASS()
class UPolys : public UObject {
    GENERATED_BODY()
public:
    TArray<FPoly> Element;

    void Serialize(FArchive& Ar) override;
};

}
