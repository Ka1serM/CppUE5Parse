#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"
#include "ue5parse/runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include "InheritableComponentHandler.generated.h"

namespace ue5parse::runtime {

class UActorComponent;
class USCS_Node;

// FComponentKey: a component of a parent Blueprint class, by its owner class and variable.
USTRUCT()
struct FComponentKey {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UClass> OwnerClass = nullptr;
    UPROPERTY()
    FName SCSVariableName;
    UPROPERTY()
    FGuid AssociatedGuid;

    FComponentKey() = default;
    // The key of a construction script node: its owner class and variable.
    explicit FComponentKey(const USCS_Node* SCSNode);
    FName GetSCSVariableName() const { return SCSVariableName; }
    bool Match(const FComponentKey& OtherKey) const;
};

// FComponentOverrideRecord: the template that replaces an inherited component's.
USTRUCT()
struct FComponentOverrideRecord {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UClass> ComponentClass = nullptr;
    UPROPERTY()
    TObjectPtr<UActorComponent> ComponentTemplate = nullptr;
    UPROPERTY()
    FComponentKey ComponentKey;
    UPROPERTY()
    FBlueprintCookedComponentInstancingData CookedComponentInstancingData;
};

// UInheritableComponentHandler: the components of parent Blueprints that a Blueprint class overrides (UE's
// Engine/InheritableComponentHandler.h).
UCLASS()
class UInheritableComponentHandler : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FComponentOverrideRecord> Records;

    const TArray<FComponentOverrideRecord>& GetRecords() const { return Records; }
    UActorComponent* GetOverridenComponentTemplate(const FComponentKey& Key) const;
};

}
