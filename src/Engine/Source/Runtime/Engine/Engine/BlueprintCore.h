#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "BlueprintCore.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBlueprintCore : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TSubclassOf<UObject> GeneratedClass;
    UPROPERTY()
    bool bLegacyNeedToPurgeSkelRefs = false;
    UPROPERTY()
    bool bLegacyGeneratedClassIsAuthoritative = false;
    UPROPERTY()
    FGuid BlueprintGuid;

    void Serialize(FArchive& Ar) override;
};

}
