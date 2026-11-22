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

    friend FArchive& operator<<(FArchive& Ar, FRigVMTag& Data) {
        Ar << Data.Name;
        Ar << Data.Label;
        Ar << Data.ToolTip;
        Ar << Data.Color;
        Ar << Data.bShowInUserInterface;
        Ar << Data.bMarksSubjectAsInvalid;
        return Ar;
    }
};

USTRUCT()
struct FRigVMVariant {
    GENERATED_BODY()
    UPROPERTY()
    FGuid Guid;
    UPROPERTY()
    TArray<FRigVMTag> Tags;

    friend FArchive& operator<<(FArchive& Ar, FRigVMVariant& Data) {
        Ar << Data.Guid;
        Ar << Data.Tags;
        return Ar;
    }
};

}
