#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include "InheritableComponentHandler.generated.h"

namespace ue5parse::runtime {

class UActorComponent;
class USCS_Node;

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
    explicit FComponentKey(const USCS_Node* SCSNode);
    bool Match(const FComponentKey& OtherKey) const;
};

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
