#pragma once

#include "Engine/Plugins/Runtime/WorldConditions/Source/WorldConditions/WorldConditionSchema.h"
#include "Engine/Plugins/Runtime/WorldConditions/Source/WorldConditions/WorldConditionTypes.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStructContainer.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"

#include "WorldConditionQuery.generated.h"

namespace ue5parse::runtime {

struct FWorldConditionCustomVersion {
    static constexpr FGuid GUID{0x2C28AC22, 0x15CF46FE, 0xBD19F011, 0x652A3C05};
    enum Type : int32 {
        StructSharedDefinition = 1,
    };
};

USTRUCT()
struct FWorldConditionEditable {
    GENERATED_BODY()

    UPROPERTY()
    uint8 ExpressionDepth = 0;
    UPROPERTY()
    EWorldConditionOperator Operator = EWorldConditionOperator::And;
    UPROPERTY()
    bool bInvert = false;
    UPROPERTY()
    FInstancedStruct Condition;
};

UCLASS()
class UWorldConditionQuerySharedDefinition : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FInstancedStructContainer Conditions;
    UPROPERTY()
    TSubclassOf<UWorldConditionSchema> SchemaClass = nullptr;
};

USTRUCT()
struct FWorldConditionQuerySharedDefinition {
    GENERATED_BODY()

    UPROPERTY()
    FInstancedStructContainer Conditions;
    UPROPERTY()
    TSubclassOf<UWorldConditionSchema> SchemaClass = nullptr;
};

USTRUCT()
struct FWorldConditionQueryDefinition {
    GENERATED_BODY()

    //! Before 5.3 the shared definition is an object, from 5.3 the struct follows the tagged properties.
    UPROPERTY()
    TObjectPtr<UWorldConditionQuerySharedDefinition> SharedDefinition = nullptr;
    UPROPERTY()
    TSubclassOf<UWorldConditionSchema> SchemaClass = nullptr;
    UPROPERTY()
    TArray<FWorldConditionEditable> EditableConditions;

    bool bHasSharedDefinitionStruct = false;
    FWorldConditionQuerySharedDefinition SharedDefinitionStruct;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FWorldConditionQueryDefinition> : TStructOpsTypeTraitsBase2<FWorldConditionQueryDefinition> {
    enum { WithSerializer = true };
};

}
