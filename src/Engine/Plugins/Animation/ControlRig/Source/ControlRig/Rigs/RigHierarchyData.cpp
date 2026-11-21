#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/Rigs/RigHierarchyData.h"

#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigObjectVersion.h"

namespace ue5parse::runtime {

using Version = FControlRigObjectVersion;

FArchive& operator<<(FArchive& Ar, FRigHierarchyKey& Key) {
    bool HasElement = false;
    bool HasComponent = false;
    Ar << HasElement;
    if (HasElement)
        Ar << Key.Element.emplace();
    Ar << HasComponent;
    if (HasComponent) {
        auto& Component = Key.Component.emplace();
        Ar << Component.Name << Component.Element;
    }
    return Ar;
}

namespace {

void LoadPreviousMap(FArchive& Ar, TArray<TPair<FRigHierarchyKey, FRigHierarchyKey>>& Map) {
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyPreviousNameAndParentMapUsingHierarchyKey, EGame::GAME_UE5_6)) {
        Ar << Map;
        return;
    }
    const int32 Count = Ar.ReadCount(16);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); ++Index) {
        auto& Pair = Map[Map.AddDefaulted()];
        Ar << Pair.Key.Element.emplace() << Pair.Value.Element.emplace();
    }
}

}

void FRigHierarchyData::LoadComponents(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(12);
    if (Count == 0)
        return;
    Ar << ComponentTypes;
    for (int32 Index = 0; Index < Count && !Ar.IsError(); ++Index) {
        auto& Component = Components.emplace_back();
        int64 End = 0;
        Ar << Component.StructIndex << End;
        const int64 Position = Ar.GetCookedPosition();
        if (!ComponentTypes.IsValidIndex(Component.StructIndex) || End < Position || End - Position > Ar.GetRemaining()) {
            Ar.SetError(ErrorCode::InvalidData, "FRigHierarchyComponentRecord.Bounds");
            return;
        }
        const auto View = Ar.ReadView(End - Position);
        Component.Bytes.assign(View.begin(), View.end());
    }
}

void FRigHierarchyData::LoadElements(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(8);
    const bool SeparatePass = Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyIndirectElementStorage, EGame::GAME_UE5_5);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); ++Index) {
        auto& Element = Elements.emplace_back();
        Ar << Element.Key;
        if (!SeparatePass)
            Element.LoadStatic(Ar, Settings);
    }
    if (SeparatePass)
        for (auto& Element : Elements) {
            Element.LoadStatic(Ar, Settings);
            if (Ar.IsError())
                return;
        }
    for (auto& Element : Elements) {
        Element.LoadParents(Ar);
        if (Ar.IsError())
            return;
    }
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyStoringPreviousNames, EGame::GAME_UE5_2)) {
        LoadPreviousMap(Ar, PreviousNames);
        LoadPreviousMap(Ar, PreviousParents);
    }
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyStoresElementMetadata, EGame::GAME_UE5_4))
        Ar << ElementMetadata;
    if (Ar.CustomVerAtLeast(Version::GUID, Version::RigHierarchyStoresComponents, EGame::GAME_UE5_6))
        LoadComponents(Ar);
}

void FRigHierarchyData::Load(FArchive& Ar) {
    Settings.Load(Ar);
    if (Ar.IsError())
        return;
    if (!Settings.bUseCompressedArchive) {
        LoadElements(Ar);
        return;
    }
    FRigHierarchyElementArchive Buffer;
    Buffer.Load(Ar);
    if (Ar.IsError())
        return;
    FRigHierarchyMemoryReader Reader(Buffer, Ar);
    LoadElements(Reader);
    if (Reader.IsError())
        Ar.SetError(Reader.GetError().code, Reader.GetError().where);
    else if (!Reader.AtEnd())
        Ar.SetError(ErrorCode::InvalidData, "FRigHierarchyData.TrailingBytes");
    if (!Components.empty())
        ComponentArchiveNames = std::move(Buffer.UniqueNames);
}

}
