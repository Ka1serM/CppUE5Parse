#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMObjectVersion.h"
#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMVariant.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

namespace ue5parse::runtime {

struct FRigVMGraphFunctionIdentifier {
    FString LibraryNodePath;
    FSoftObjectPath HostObject;

    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionIdentifier& Identifier);
};

struct FRigVMGraphFunctionDependency {
    FRigVMGraphFunctionIdentifier Identifier;
    uint32 Hash = 0;

    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionDependency& Dependency) { return Ar << Dependency.Identifier << Dependency.Hash; }
};

struct FRigVMGraphFunctionArgument {
    FName Name;
    FName DisplayName;
    FName CPPType;
    FSoftObjectPath CPPTypeObject;
    bool bIsArray = false;
    uint8 Direction = 0;
    FString DefaultValue;
    bool bIsConst = false;
    TMap<FString, FText> PathToTooltip;
    bool bIsInputVariable = false;

    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionArgument& Argument);
};

struct FRigVMExternalVariableDef {
    FGuid Guid;
    FName Name;
    FName BaseCPPType;
    FSoftObjectPath CPPTypeObject;
    bool bIsArray = false;
    bool bIsPublic = false;
    bool bIsReadOnly = false;
    int32 Size = 0;

    friend FArchive& operator<<(FArchive& Ar, FRigVMExternalVariableDef& Variable);
};

struct FRigVMPinCategory {
    FString Path;
    TArray<FString> Elements;
    bool bExpandedByDefault = true;

    friend FArchive& operator<<(FArchive& Ar, FRigVMPinCategory& Category);
};

struct FRigVMNodeLayout {
    TArray<FRigVMPinCategory> Categories;
    TMap<FString, int32> PinIndexInCategory;
    TMap<FString, FString> DisplayNames;

    friend FArchive& operator<<(FArchive& Ar, FRigVMNodeLayout& Layout);
};

struct FRigVMGraphFunctionHeader {
    FRigVMGraphFunctionIdentifier LibraryPointer;
    FRigVMVariant Variant;
    FName Name;
    FString NodeTitle;
    FLinearColor NodeColor;
    FText Tooltip_DEPRECATED;
    FString Description;
    FString Category;
    FString Keywords;
    TArray<FRigVMGraphFunctionArgument> Arguments;
    TArray<FRigVMGraphFunctionDependency> Dependencies;
    TArray<FRigVMExternalVariableDef> ExternalVariables;
    FRigVMNodeLayout Layout;

    friend FArchive& operator<<(FArchive& Ar, FRigVMGraphFunctionHeader& Header);
};

}
