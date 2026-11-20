#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Curves/CurveLinearColor.h"

#include "SpecularProfile.generated.h"

namespace ue5parse::runtime {

class UTexture2D;

UENUM()
enum class ESpecularProfileFormat : uint8 { ViewLightVector, HalfVector };

USTRUCT()
struct FSpecularProfileStruct {
    GENERATED_BODY()
    UPROPERTY()
    ESpecularProfileFormat Format = ESpecularProfileFormat::ViewLightVector;
    UPROPERTY()
    FRuntimeCurveLinearColor ViewColor;
    UPROPERTY()
    FRuntimeCurveLinearColor LightColor;
    UPROPERTY()
    TObjectPtr<UTexture2D> Texture = nullptr;

    FSpecularProfileStruct();
};

UCLASS()
class USpecularProfile : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FSpecularProfileStruct Settings;
    UPROPERTY()
    FGuid Guid;
};

}
