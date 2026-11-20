#pragma once

#include "Engine/Plugins/Runtime/ProceduralMeshComponent/Source/ProceduralMeshComponent/ProceduralMeshComponent.h"

#include "AppleARKitFaceMeshComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAppleARKitFaceMeshComponent : public UProceduralMeshComponent {
    GENERATED_BODY()
public:
    explicit UAppleARKitFaceMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
