#include "Engine/Source/Runtime/SlateCore/Fonts/CompositeFont.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

void FFontFaceData::Serialize(FArchive& Ar) {
    Ar << Data;
    PreprocessedFontGeometries.Reset();
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::AddedPreprocessedFontGeometry, EGame::GAME_UE5_6))
        Ar << PreprocessedFontGeometries;
}

bool FFontData::Serialize(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FEditorObjectVersion::GUID, FEditorObjectVersion::AddedFontFaceAssets, EGame::GAME_UE4_15))
        return false;
    bool bIsCooked = false;
    Ar << bIsCooked;
    if (!bIsCooked) {
        StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this));
        return true;
    }
    Ar << FontFaceAsset;
    if (FontFaceAsset == nullptr)
        Ar << FontFilename << Hinting << LoadingPolicy;
    if (Ar.GetGame() >= EGame::GAME_UE4_20)
        Ar << SubFaceIndex;
    return true;
}

}
