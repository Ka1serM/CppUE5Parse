#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"

#include "ShadowMapTexture2D.generated.h"

namespace ue5parse::runtime {

UENUM()
enum EShadowMapFlags : uint8 { SMF_None = 0, SMF_Streamed = 0x00000001 };

UCLASS()
class UShadowMapTexture2D : public UTexture2D {
    GENERATED_BODY()
public:
    UShadowMapTexture2D(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TEnumAsByte<EShadowMapFlags> ShadowmapFlags = SMF_None;
};

}
