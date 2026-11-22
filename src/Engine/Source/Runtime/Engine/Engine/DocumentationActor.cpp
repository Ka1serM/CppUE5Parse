#include "Engine/Source/Runtime/Engine/Engine/DocumentationActor.h"

namespace ue5parse::runtime {

ADocumentationActor::ADocumentationActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
}

}
