#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectTypes.h"

#include "SmartObjectComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USmartObjectComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit USmartObjectComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FGuid ComponentGuid;
    UPROPERTY()
    TObjectPtr<UObject> DefinitionAsset_DEPRECATED = nullptr;
    UPROPERTY()
    FSmartObjectHandle RegisteredHandle;
    UPROPERTY()
    bool bCanBePartOfCollection = true;
};

}
