#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "Text3DCharacterTransform.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UText3DCharacterTransform : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UText3DCharacterTransform(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
