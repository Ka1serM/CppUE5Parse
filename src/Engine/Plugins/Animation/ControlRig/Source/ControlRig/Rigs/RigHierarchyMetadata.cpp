#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyMetadata.h"

#include <array>
#include <string_view>

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FRigMetadataEntry& Entry) {
    Ar << Entry.ItemName << Entry.TypeName << Entry.Name << Entry.Type;
    if (Ar.IsError())
        return Ar;
    static constexpr std::array<std::string_view, 20> Names{
        "Bool", "BoolArray",
        "Float", "FloatArray",
        "Int32", "Int32Array",
        "Name", "NameArray",
        "Vector", "VectorArray",
        "Rotator", "RotatorArray",
        "Quat", "QuatArray",
        "Transform", "TransformArray",
        "LinearColor", "LinearColorArray",
        "RigElementKey", "RigElementKeyArray"
    };
    std::string_view TypeName = Entry.TypeName.getText();
    TypeName.remove_prefix(TypeName.rfind(':') + 1);
    if (Entry.Type >= Names.size() || Names[Entry.Type] != TypeName) {
        Ar.SetError(ErrorCode::InvalidData, "FRigMetadataEntry.Type");
        return Ar;
    }
    switch (Entry.Type) {
        case 0: return Ar << Entry.Value.emplace<bool>();
        case 1: return Ar << Entry.Value.emplace<TArray<bool>>();
        case 2: return Ar << Entry.Value.emplace<float>();
        case 3: return Ar << Entry.Value.emplace<TArray<float>>();
        case 4: return Ar << Entry.Value.emplace<int32>();
        case 5: return Ar << Entry.Value.emplace<TArray<int32>>();
        case 6: return Ar << Entry.Value.emplace<FName>();
        case 7: return Ar << Entry.Value.emplace<TArray<FName>>();
        case 8: return Ar << Entry.Value.emplace<FVector>();
        case 9: return Ar << Entry.Value.emplace<TArray<FVector>>();
        case 10: return Ar << Entry.Value.emplace<FRotator>();
        case 11: return Ar << Entry.Value.emplace<TArray<FRotator>>();
        case 12: return Ar << Entry.Value.emplace<FQuat>();
        case 13: return Ar << Entry.Value.emplace<TArray<FQuat>>();
        case 14: return Ar << Entry.Value.emplace<FTransform>();
        case 15: return Ar << Entry.Value.emplace<TArray<FTransform>>();
        case 16: return Ar << Entry.Value.emplace<FLinearColor>();
        case 17: return Ar << Entry.Value.emplace<TArray<FLinearColor>>();
        case 18: return Ar << Entry.Value.emplace<FRigElementKey>();
        case 19: return Ar << Entry.Value.emplace<TArray<FRigElementKey>>();
    }
    return Ar;
}

}
