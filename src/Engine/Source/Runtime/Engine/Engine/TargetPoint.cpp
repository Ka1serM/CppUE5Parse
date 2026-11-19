#include "Engine/Source/Runtime/Engine/Engine/TargetPoint.h"

namespace ue5parse::runtime {

ATargetPoint::ATargetPoint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
}

}
