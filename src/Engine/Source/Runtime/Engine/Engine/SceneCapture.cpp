#include "Engine/Source/Runtime/Engine/Engine/SceneCapture.h"

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

namespace ue5parse::runtime {

ASceneCapture::ASceneCapture(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    //! UE 4.21 and older create the camera mesh as the root, so their cooked actors import Default__<Class>:CamMesh0.
    MeshComp_DEPRECATED = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(FName("CamMesh0"));
    MeshComp_DEPRECATED->bHiddenInGame = true;
    MeshComp_DEPRECATED->CastShadow = false;
    SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComponent"));
    RootComponent = SceneComponent;
}

}
