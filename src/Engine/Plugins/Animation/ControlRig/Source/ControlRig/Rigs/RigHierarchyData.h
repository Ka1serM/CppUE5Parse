#pragma once

#include <optional>

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyElements.h"

namespace ue5parse::runtime {

struct FRigComponentKey {
    FName Name;
    FRigElementKey Element;
};

struct FRigHierarchyKey {
    std::optional<FRigElementKey> Element;
    std::optional<FRigComponentKey> Component;

    friend FArchive& operator<<(FArchive& Ar, FRigHierarchyKey& Key);
};

struct FRigHierarchyComponentRecord {
    int32 StructIndex = 0;
    std::vector<std::byte> Bytes;
};

struct FRigHierarchyData {
    FRigHierarchySerializationSettings Settings;
    std::vector<FRigHierarchyElement> Elements;
    TArray<TPair<FRigHierarchyKey, FRigHierarchyKey>> PreviousNames;
    TArray<TPair<FRigHierarchyKey, FRigHierarchyKey>> PreviousParents;
    TArray<TPair<FRigElementKey, TArray<FRigMetadataEntry>>> ElementMetadata;
    TArray<FString> ComponentTypes;
    TArray<FName> ComponentArchiveNames;
    std::vector<FRigHierarchyComponentRecord> Components;

    void Load(FArchive& Ar);

private:
    void LoadElements(FArchive& Ar);
    void LoadComponents(FArchive& Ar);
};

}
