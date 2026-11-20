#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "PreCullTrianglesVolume.generated.h"

namespace ue5parse::runtime {

UCLASS()
class APreCullTrianglesVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit APreCullTrianglesVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
