#include "Engine/Source/Runtime/Engine/Engine/Polys.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FPoly& Poly) {
    Ar << Poly.Base << Poly.Normal << Poly.TextureU << Poly.TextureV << Poly.Vertices << Poly.PolyFlags << Poly.Actor << Poly.ItemName;
    return Ar << Poly.Material << Poly.iLink << Poly.iBrushPoly << Poly.LightMapScale << Poly.LightmassSettings << Poly.RulesetVariation;
}

void UPolys::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd())
        return;
    UObject* ElementOwner = nullptr;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::BSP_UNDO_FIX) {
        Ar << ElementOwner << Element;
        return;
    }
    int32 DbNum = 0;
    int32 DbMax = 0;
    Ar << DbNum << DbMax << ElementOwner;
    Element.Empty(DbNum);
    for (int32 i = 0; i < DbNum && !Ar.IsError(); i++)
        Ar << Element[Element.AddDefaulted()];
}

}
