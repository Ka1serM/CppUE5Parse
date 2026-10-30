#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"

#include "UserDefinedStruct.generated.h"

namespace ue5parse::runtime {

enum EUserDefinedStructureStatus : int32 { UDSS_UpToDate, UDSS_Dirty, UDSS_Error, UDSS_Duplicate, UDSS_MAX };

// UUserDefinedStruct: a struct made in the editor, with its default values (UE's Engine/UserDefinedStruct.h). A new
// value of the struct starts as the default instance.
UCLASS()
class UUserDefinedStruct : public UScriptStruct {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<EUserDefinedStructureStatus> Status = UDSS_UpToDate;
    UPROPERTY()
    FGuid Guid;

    void Serialize(FArchive& Ar) override;
    void FinishDestroy() override;
    void InitializeStruct(void* Dest, int32 ArrayDim = 1) const override;
    // The default values, laid out as the struct; null before they are read.
    const uint8* GetDefaultInstance() const { return DefaultStructInstance; }

private:
    uint8* DefaultStructInstance = nullptr;
};

}
