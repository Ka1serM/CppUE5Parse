#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectTypes.h"
#include "Engine/Source/Runtime/GameplayTags/GameplayTagContainer.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"

#include "SmartObjectCollection.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSmartObjectCollectionEntry {
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTagContainer Tags;
    UPROPERTY()
    FSoftObjectPath Path_DEPRECATED;
    UPROPERTY()
    FTransform Transform;
    UPROPERTY()
    FBox Bounds;
    UPROPERTY()
    FSmartObjectHandle Handle;
    UPROPERTY()
    uint32 DefinitionIdx = 0xFFFFFFFFu;
};

USTRUCT()
struct FSmartObjectContainer {
    GENERATED_BODY()

    UPROPERTY()
    FBox Bounds;
    UPROPERTY()
    TArray<FSmartObjectCollectionEntry> CollectionEntries;
    UPROPERTY()
    TMap<FSmartObjectHandle, FSoftObjectPath> RegisteredIdToObjectMap_DEPRECATED;
    UPROPERTY()
    TMap<FSmartObjectHandle, TObjectPtr<UObject>> HandleToComponentMappings;
    UPROPERTY()
    TObjectPtr<UObject> Owner = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> Definitions_DEPRECATED;
};

UCLASS()
class ASmartObjectCollection : public AActor {
    GENERATED_BODY()
public:
    explicit ASmartObjectCollection(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FBox Bounds;
    UPROPERTY()
    TArray<FSmartObjectCollectionEntry> CollectionEntries;
    UPROPERTY()
    TMap<FSmartObjectHandle, FSoftObjectPath> RegisteredIdToObjectMap;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> Definitions;
    UPROPERTY()
    bool bBuildOnDemand_DEPRECATED = true;
    UPROPERTY()
    bool bBuildCollectionAutomatically = false;
};

}
