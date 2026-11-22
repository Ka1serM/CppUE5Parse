#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/Templates/TypeHash.h"

namespace ue5parse::runtime {

enum class ERigVMMemoryType : uint8 {
    Work,
    Literal,
    External,
    Debug,
    Invalid,
};

struct FRigVMOperand {
    ERigVMMemoryType MemoryType = ERigVMMemoryType::Invalid;
    uint16 RegisterIndex = UINT16_MAX;
    uint16 RegisterOffset = UINT16_MAX;

    bool operator==(const FRigVMOperand& Other) const = default;
    friend uint32 GetTypeHash(const FRigVMOperand& Operand) {
        return HashCombine(HashCombine(GetTypeHash(static_cast<uint8>(Operand.MemoryType)), GetTypeHash(Operand.RegisterIndex)),
                           GetTypeHash(Operand.RegisterOffset));
    }
    friend FArchive& operator<<(FArchive& Ar, FRigVMOperand& Operand) { return Ar << Operand.MemoryType << Operand.RegisterIndex << Operand.RegisterOffset; }
};

}
