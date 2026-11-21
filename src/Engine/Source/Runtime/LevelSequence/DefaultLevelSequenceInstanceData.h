#pragma once

#include "Engine/Source/Runtime/Core/Math/Transform.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "DefaultLevelSequenceInstanceData.generated.h"

namespace ue5parse::runtime {

class AActor;

UCLASS()
class UDefaultLevelSequenceInstanceData : public UObject {
    GENERATED_BODY()
public:
    explicit UDefaultLevelSequenceInstanceData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<AActor> TransformOriginActor = nullptr;
    UPROPERTY()
    FTransform TransformOrigin;
};

}
