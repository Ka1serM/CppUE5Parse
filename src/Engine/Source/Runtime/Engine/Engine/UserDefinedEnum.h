#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "UserDefinedEnum.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UUserDefinedEnum : public UEnum {
    GENERATED_BODY()
public:
    UPROPERTY()
    TMap<FName, FText> DisplayNameMap;
    //! UE 4.14 and older keep the display names by enumerator index.
    UPROPERTY()
    TArray<FText> DisplayNames;
};

}
