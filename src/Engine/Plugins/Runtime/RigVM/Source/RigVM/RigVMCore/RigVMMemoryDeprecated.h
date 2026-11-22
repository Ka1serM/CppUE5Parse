#pragma once

#include "Engine/Plugins/Runtime/RigVM/Source/RigVM/RigVMCore/RigVMMemoryCommon.h"
#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class UScriptStruct;

enum class ERigVMRegisterType : uint8 {
    Plain,
    String,
    Name,
    Struct,
    Invalid,
};

struct FRigVMRegister {
    ERigVMRegisterType Type = ERigVMRegisterType::Invalid;
    uint32 ByteIndex = 0;
    uint16 ElementSize = 0;
    uint16 ElementCount = 0;
    uint16 SliceCount = 1;
    uint8 AlignmentBytes = 0;
    uint16 TrailingBytes = 0;
    FName Name;
    int32 ScriptStructIndex = -1;
    bool bIsArray = false;
    bool bIsDynamic = false;

    bool IsDynamic() const { return bIsDynamic; }
    bool IsNestedDynamic() const { return bIsDynamic && bIsArray; }
    bool Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigVMRegister& Register) {
        Register.Serialize(Ar);
        return Ar;
    }
};

struct FRigVMRegisterOffset {
    TArray<int32> Segments;
    ERigVMRegisterType Type = ERigVMRegisterType::Invalid;
    FName CPPType;
    UScriptStruct* ScriptStruct = nullptr;
    UScriptStruct* ParentScriptStruct = nullptr;
    int32 ArrayIndex = 0;
    uint16 ElementSize = 0;
    FString CachedSegmentPath;

    bool Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigVMRegisterOffset& Offset) {
        Offset.Serialize(Ar);
        return Ar;
    }
};

//! UE imports the values of a struct register from text into its memory; the port keeps the values in the form that the archive stores.
struct FRigVMRegisterValues {
    TArray<uint8> Bytes;
    TArray<FName> Names;
    TArray<FString> Strings;
};

struct FRigVMMemoryContainer {
    bool bUseNameMap = true;
    ERigVMMemoryType MemoryType = ERigVMMemoryType::Work;
    TArray<FRigVMRegister> Registers;
    TArray<FRigVMRegisterOffset> RegisterOffsets;
    TArray<FString> ScriptStructPaths;
    TArray<TArray<FRigVMRegisterValues>> RegisterValues;

    bool Serialize(FArchive& Ar);
    friend FArchive& operator<<(FArchive& Ar, FRigVMMemoryContainer& Container) {
        Container.Serialize(Ar);
        return Ar;
    }
};

}
