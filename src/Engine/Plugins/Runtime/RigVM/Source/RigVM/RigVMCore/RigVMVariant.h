#pragma once

#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "RigVMVariant.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FRigVMTag {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FString Label;
    UPROPERTY()
    FText ToolTip;
    UPROPERTY()
    FLinearColor Color = FLinearColor::White;
    UPROPERTY()
    bool bShowInUserInterface = true;
    UPROPERTY()
    bool bMarksSubjectAsInvalid = false;
};

USTRUCT()
struct FRigVMVariant {
    GENERATED_BODY()
    UPROPERTY()
    FGuid Guid;
    UPROPERTY()
    TArray<FRigVMTag> Tags;
};

}
