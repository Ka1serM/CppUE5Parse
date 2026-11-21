#include "Engine/Source/Runtime/Engine/Animation/MorphTarget.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FMorphTargetDelta& D) {
    Ar << D.PositionDelta;
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::MORPHTARGET_CPU_TANGENTZDELTA_FORMATCHANGE) {
        uint32 Packed = 0;
        Ar << Packed;
        auto Unpack = [Packed](uint32 Shift) { return static_cast<float>((Packed >> Shift) & 255) / 127.5f - 1.0f; };
        D.TangentZDelta = {Unpack(0), Unpack(8), Unpack(16)};
    } else
        Ar << D.TangentZDelta;
    return Ar << D.SourceIdx;
}

FArchive& operator<<(FArchive& Ar, FMorphTargetLODModel& M) {
    using FMain = FFortniteMainBranchObjectVersion;
    using FSpecial = FUE5SpecialProjectStreamObjectVersion;
    bool HasSections = Ar.CustomVerAtLeast(FEditorObjectVersion::GUID, FEditorObjectVersion::AddedMorphTargetSectionIndices, EGame::GAME_UE4_19);
    bool HasGenerated = HasSections && Ar.CustomVerAtLeast(FMain::GUID, FMain::SaveGeneratedMorphTargetByEngine, EGame::GAME_UE4_20);
    bool Stripped = false;
    if (HasGenerated && Ar.CustomVerAtLeast(FSpecial::GUID, FSpecial::StripMorphTargetSourceDataForCookedBuilds, EGame::GAME_UE5_0))
        Ar << Stripped;
    if (Stripped)
        Ar << M.NumVertices;
    else {
        Ar << M.Vertices;
        M.NumVertices = M.Vertices.Num();
    }
    Ar << M.NumBaseMeshVerts;
    if (HasSections)
        Ar << M.SectionIndices;
    if (HasGenerated)
        Ar << M.bGeneratedByEngine;
    if (Ar.CustomVerAtLeast(FMain::GUID, FMain::MorphTargetCustomImport, EGame::GAME_UE5_5))
        Ar << M.SourceFilename;
    if (Ar.GetGame() >= EGame::GAME_UE5_6 && M.SourceFilename.StartsWith(FString("GeneratedByEngine_"))) {
        M.bGeneratedByEngine = true;
        M.SourceFilename.Empty();
    }
    return Ar;
}

void UMorphTarget::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsAudioVisualDataStripped())
        return;
    bool Cooked = false;
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::MorphTargetCookedCPUDataCompressed, EGame::GAME_UE5_7))
        Ar << Cooked;
    Ar << MorphLODModels;
    if (!Cooked || Ar.IsError())
        return;
    bool StoreCompressedVertices = false;
    Ar << StoreCompressedVertices;
    if (StoreCompressedVertices)
        Ar << CompressedLODModels;
}

}
