#include "Engine/Source/Runtime/GeometryCore/DynamicMesh/DynamicMeshAttributeSet.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

template <class LayerType>
void SerializeLayers(FArchive& Ar, TArray<LayerType>& Layers) {
    const int32 Num = Ar.ReadCount(1);
    Layers.Empty(Num);
    for (int32 Index = 0; Index < Num && !Ar.IsError(); Index++)
        Layers[Layers.AddDefaulted()].Serialize(Ar);
}

template <class AttributeType>
void SerializeOptional(FArchive& Ar, TOptional<AttributeType>& Attribute, bool bHasAttribute) {
    Attribute.Reset();
    if (bHasAttribute)
        Attribute.Emplace().Serialize(Ar);
}

FName SerializeAttributeKey(FArchive& Ar) {
    FName Key;
    if (Ar.GetGame() < EGame::GAME_UE5_1) {
        Ar << Key;
        return Key;
    }
    FString KeyString;
    Ar << KeyString;
    return MakeName(KeyString);
}

uint32 ReadLabelIndex(FArchive& Ar, uint8 IndexSize) {
    if (IndexSize == 0)
        return Ar.Read<uint8>();
    if (IndexSize == 1)
        return Ar.Read<uint16>();
    return Ar.Read<uint32>();
}

}

template <class AttributeType>
void FDynamicMeshAttributeSet::SerializeNamedAttributes(FArchive& Ar, TMap<FName, AttributeType>& Attributes) {
    Attributes.Empty();
    const int32 NumAttributes = Ar.ReadCount(1);
    for (int32 Index = 0; Index < NumAttributes && !Ar.IsError(); Index++) {
        const FName Key = SerializeAttributeKey(Ar);
        bool bIsValid = false;
        Ar << bIsValid;
        if (bIsValid)
            Attributes.Add(Key).Serialize(Ar);
    }
}

void FDynamicMeshTriangleLabelAttribute::Serialize(FArchive& Ar) {
    Labels.Empty();
    const uint32 NumNames = Ar.Read<uint32>();
    if (NumNames == 0 || Ar.IsError())
        return;
    const int32 NumUniqueNames = Ar.ReadCount(1);
    if (NumNames > static_cast<uint32>(std::numeric_limits<int32>::max()) || NumUniqueNames == 0) {
        Ar.SetError(ErrorCode::InvalidData, "FDynamicMeshAttributeSet.SerializeLabels.Num");
        return;
    }
    TArray<FName> UniqueNames;
    for (int32 Index = 0; Index < NumUniqueNames && !Ar.IsError(); Index++) {
        FString NameString;
        Ar << NameString;
        UniqueNames.Add(MakeName(NameString));
    }
    if (Ar.IsError())
        return;
    const bool bSingle = NumUniqueNames == 1;
    if (bSingle || static_cast<uint32>(NumUniqueNames) == NumNames) {
        for (uint32 Index = 0; Index < NumNames; Index++)
            Labels.Add(bSingle ? UniqueNames[0] : UniqueNames[static_cast<int32>(Index)]);
        return;
    }
    const uint8 IndexSize = Ar.Read<uint8>();
    for (uint32 Index = 0; Index < NumNames && !Ar.IsError(); Index++) {
        const uint32 NameIndex = ReadLabelIndex(Ar, IndexSize);
        if (NameIndex >= static_cast<uint32>(NumUniqueNames)) {
            Ar.SetError(ErrorCode::InvalidData, "FDynamicMeshAttributeSet.SerializeLabels.Index");
            return;
        }
        Labels.Add(UniqueNames[static_cast<int32>(NameIndex)]);
    }
}

void FDynamicMeshAttributeSet::Serialize(FArchive& Ar) {
    bool bUseCompression = false;
    Ar << bUseCompression;
    SerializeLayers(Ar, UVLayers);
    SerializeLayers(Ar, NormalLayers);
    SerializeLayers(Ar, PolygroupLayers);
    WeightLayers.Empty();
    if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::DynamicMeshAttributesWeightMapsAndNames, EGame::GAME_UE5_1))
        SerializeLayers(Ar, WeightLayers);
    SerializeOptional(Ar, ColorLayer, Ar.Read<int32>() > 0);
    bool bHasMaterialID = false;
    Ar << bHasMaterialID;
    SerializeOptional(Ar, MaterialIDAttrib, bHasMaterialID);
    SerializeNamedAttributes(Ar, SkinWeightAttributes);
    MorphTargetAttributes.Empty();
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::DynamicMeshAttributesMorphTargets, EGame::GAME_UE5_7))
        SerializeNamedAttributes(Ar, MorphTargetAttributes);
    Bones.Reset();
    if (Ar.CustomVerAtLeast(FFortniteReleaseBranchCustomObjectVersion::GUID, FFortniteReleaseBranchCustomObjectVersion::DynamicMeshAttributesSerializeBones,
                            EGame::GAME_UE5_5)) {
        bool bHasBones = false;
        Ar << bHasBones;
        if (bHasBones) {
            FDynamicMeshBones& Value = Bones.Emplace();
            Value.BoneNameAttrib.Serialize(Ar);
            Value.BoneParentIndexAttrib.Serialize(Ar);
            Value.BonePoseAttrib.Serialize(Ar);
            Value.BoneColorAttrib.Serialize(Ar);
        }
    }
    SculptLayers = {};
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::DynamicMeshSerializeSculptLayers, EGame::GAME_UE5_6)) {
        const int32 NumLayers = Ar.ReadCount(1);
        if (NumLayers > 0)
            SculptLayers.Serialize(Ar, NumLayers);
    }
    TriangleLabelAttributes.Empty();
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::DynamicMeshAttributesTriangleLabels, EGame::GAME_UE5_8))
        SerializeNamedAttributes(Ar, TriangleLabelAttributes);
}

}
