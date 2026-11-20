#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

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

    friend FArchive& operator<<(FArchive& Ar, FRigVMOperand& Operand) { return Ar << Operand.MemoryType << Operand.RegisterIndex << Operand.RegisterOffset; }
};

}
