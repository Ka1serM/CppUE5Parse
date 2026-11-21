#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "FieldSystem.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UFieldSystem : public UObject {
    GENERATED_BODY()
public:
    explicit UFieldSystem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
