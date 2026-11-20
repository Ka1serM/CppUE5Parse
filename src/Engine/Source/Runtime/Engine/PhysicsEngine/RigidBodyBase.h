#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "RigidBodyBase.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class ARigidBodyBase : public AActor {
    GENERATED_BODY()
public:
    explicit ARigidBodyBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
