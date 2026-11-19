#include "Engine/Source/Runtime/Engine/Engine/FontFace.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

void UFontFace::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    bool bCooked = false;
    if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::AddedCookedBoolFontFaceAssets, EGame::GAME_UE5_0))
        Ar << bCooked;
    if (!Ar.CustomVerAtLeast(FEditorObjectVersion::GUID, FEditorObjectVersion::AddedInlineFontFaceAssets, EGame::GAME_UE4_15)) {
        FontFaceData.Data = std::move(FontFaceData_DEPRECATED);
        return;
    }
    bool bLoadInlineData = false;
    Ar << bLoadInlineData;
    if (bLoadInlineData)
        FontFaceData.Serialize(Ar);
}

}
