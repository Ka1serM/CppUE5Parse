#pragma once

#include "Engine/Plugins/Runtime/ProceduralMeshComponent/Source/ProceduralMeshComponent/ProceduralMeshComponent.h"

#include "AppleARKitFaceMeshComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

UENUM()
enum class EARFaceComponentTransformMixing : uint8 { ComponentOnly, ComponentLocationTrackedRotation, ComponentWithTracked, TrackingOnly };

UCLASS()
class UAppleARKitFaceMeshComponent : public UProceduralMeshComponent {
    GENERATED_BODY()
public:
    explicit UAppleARKitFaceMeshComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool bWantsMeshUpdates = false;
    UPROPERTY()
    bool bWantsCollision = false;
    UPROPERTY()
    bool bAutoBindToLocalFaceMesh = false;
    UPROPERTY()
    EARFaceComponentTransformMixing TransformSetting = EARFaceComponentTransformMixing::ComponentOnly;
    UPROPERTY()
    bool bFlipTrackedRotation = false;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> FaceMaterial = nullptr;
    UPROPERTY()
    FName LiveLinkSubjectName;
};

}
