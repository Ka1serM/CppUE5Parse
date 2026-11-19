#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "UserDefinedStruct.generated.h"

namespace ue5parse::runtime {

UENUM()
enum EUserDefinedStructureStatus : int32 { UDSS_UpToDate, UDSS_Dirty, UDSS_Error, UDSS_Duplicate, UDSS_MAX };

//! UE 5.8 keeps an ID for each member property of a user defined struct.
USTRUCT()
struct FUserDefinedPropertyID {
    GENERATED_BODY()
    UPROPERTY()
    FGuid ID;
    UPROPERTY()
    FName Name;
};

UCLASS()
class UUserDefinedStruct : public UScriptStruct {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<EUserDefinedStructureStatus> Status = UDSS_UpToDate;
    UPROPERTY()
    FGuid Guid;
    UPROPERTY()
    TArray<FUserDefinedPropertyID> PropertyIDs;

    void Serialize(FArchive& Ar) override;
    void FinishDestroy() override;
    void InitializeStruct(void* Dest, int32 ArrayDim = 1) const override;

private:
    uint8* DefaultStructInstance = nullptr;
};

}
