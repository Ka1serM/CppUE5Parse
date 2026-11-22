#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMObjectVersion.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "RigVMNodeLayout.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FRigVMPinCategory {
    GENERATED_BODY()
    UPROPERTY()
    FString Path;
    UPROPERTY()
    TArray<FString> Elements;
    UPROPERTY()
    bool bExpandedByDefault = true;

    friend FArchive& operator<<(FArchive& Ar, FRigVMPinCategory& Category) {
        Ar << Category.Path << Category.Elements;
        if (FRigVMObjectVersion::AtLeast(Ar, FRigVMObjectVersion::FunctionHeaderLayoutStoresCategoryExpansion))
            Ar << Category.bExpandedByDefault;
        return Ar;
    }
};

USTRUCT()
struct FRigVMNodeLayout {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FRigVMPinCategory> Categories;
    UPROPERTY()
    TMap<FString, int32> PinIndexInCategory;
    UPROPERTY()
    TMap<FString, FString> DisplayNames;

    friend FArchive& operator<<(FArchive& Ar, FRigVMNodeLayout& Layout);
};

}
