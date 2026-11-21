#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectCollection.h"

#include "SmartObjectPersistentCollection.generated.h"

namespace ue5parse::runtime {

class UBillboardComponent;

UCLASS()
class ASmartObjectPersistentCollection : public AActor {
    GENERATED_BODY()
public:
    explicit ASmartObjectPersistentCollection(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FSmartObjectContainer SmartObjectContainer;
    UPROPERTY()
    TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> RenderingComponent = nullptr;
    UPROPERTY()
    bool bUpdateCollectionOnSmartObjectsChange = true;
    UPROPERTY()
    bool bEnableDebugDrawing = true;
};

}
