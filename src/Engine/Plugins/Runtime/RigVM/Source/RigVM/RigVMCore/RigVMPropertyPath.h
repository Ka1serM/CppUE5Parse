#pragma once

#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FRigVMPropertyPathDescription {
    int32 PropertyIndex = -1;
    FString HeadCPPType;
    FString SegmentPath;

    friend FArchive& operator<<(FArchive& Ar, FRigVMPropertyPathDescription& Path) { return Ar << Path.PropertyIndex << Path.HeadCPPType << Path.SegmentPath; }
};

}
