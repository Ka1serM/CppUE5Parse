#pragma once

#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectBindingCollection.h"
#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectTypes.h"
#include "Engine/Plugins/Runtime/SmartObjects/Source/SmartObjectsModule/SmartObjectWorldConditionSchema.h"
#include "Engine/Plugins/Runtime/WorldConditions/Source/WorldConditions/WorldConditionQuery.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/PropertyBag.h"
#include "Engine/Source/Runtime/Engine/Engine/DataAsset.h"
#include "Engine/Source/Runtime/GameplayTags/GameplayTagContainer.h"

#include "SmartObjectDefinition.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class USmartObjectBehaviorDefinition : public UObject {
    GENERATED_BODY()
};

USTRUCT()
struct FSmartObjectDefinitionDataProxy {
    GENERATED_BODY()

    UPROPERTY()
    TInstancedStruct<FSmartObjectDefinitionData> Data;
};

USTRUCT()
struct FSmartObjectSlotDefinition {
    GENERATED_BODY()

    UPROPERTY()
    FVector3f Offset = FVector3f(0, 0, 0);
    UPROPERTY()
    FRotator3f Rotation = FRotator3f(0, 0, 0);
    UPROPERTY()
    bool bEnabled = true;
    UPROPERTY()
    FGameplayTagQuery UserTagFilter;
    UPROPERTY()
    FGameplayTagContainer ActivityTags;
    UPROPERTY()
    FGameplayTagContainer RuntimeTags;
    UPROPERTY()
    FWorldConditionQueryDefinition SelectionPreconditions;
    UPROPERTY()
    TArray<TObjectPtr<USmartObjectBehaviorDefinition>> BehaviorDefinitions;
    UPROPERTY()
    TArray<FSmartObjectDefinitionDataProxy> DefinitionData;
    //! Before 5.4 the slot data is an array of instanced structs.
    UPROPERTY()
    TArray<FInstancedStruct> Data;
};

UCLASS()
class USmartObjectDefinition : public UDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    FInstancedPropertyBag Parameters;
    UPROPERTY()
    FGuid ParametersID;
    UPROPERTY()
    FGuid RootID;
    UPROPERTY()
    FSmartObjectBindingCollection BindingCollection;
    //! Before 5.6 the bindings are an array on the definition.
    UPROPERTY()
    TArray<FSmartObjectDefinitionPropertyBinding> PropertyBindings;
    UPROPERTY()
    TArray<FSmartObjectSlotDefinition> Slots;
    UPROPERTY()
    TArray<TObjectPtr<USmartObjectBehaviorDefinition>> DefaultBehaviorDefinitions;
    UPROPERTY()
    FGameplayTagQuery UserTagFilter;
    //! Before 5.2 the object tag filter is runtime data.
    UPROPERTY()
    FGameplayTagQuery ObjectTagFilter;
    UPROPERTY()
    FWorldConditionQueryDefinition Preconditions;
    UPROPERTY()
    FGameplayTagContainer ActivityTags;
    UPROPERTY()
    TArray<FSmartObjectDefinitionDataProxy> DefinitionData;
    UPROPERTY()
    TSubclassOf<USmartObjectWorldConditionSchema> WorldConditionSchemaClass;
    UPROPERTY()
    ESmartObjectTagMergingPolicy ActivityTagsMergingPolicy = ESmartObjectTagMergingPolicy::Override;
    UPROPERTY()
    ESmartObjectTagFilteringPolicy UserTagsFilteringPolicy = ESmartObjectTagFilteringPolicy::Override;
};

}
