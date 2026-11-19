#include "Engine/Source/Runtime/NavigationSystem/NavCollision.h"

namespace ue5parse::runtime {

void UNavCollision::Serialize(FArchive& Ar) {
    //! UE 4.3 to 4.18 construct the nav collision with bGatherConvexGeometry off.
    if (Ar.GetGame() < EGame::GAME_UE4_19)
        bGatherConvexGeometry = false;
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    constexpr int32 VerInitial = 1;
    constexpr int32 VerAreaClass = 2;
    constexpr int32 VerShapeGeoExport = 4;
    constexpr int32 MagicNum = static_cast<int32>(0xA237F237);
    const int64 StreamStartPos = Ar.Tell();
    int32 Version = VerInitial;
    int32 MyMagicNum = 0;
    Ar << MyMagicNum;
    if (MyMagicNum == MagicNum)
        Ar << Version;
    else
        Ar.Seek(StreamStartPos);
    FGuid Guid;
    Ar << Guid;
    bool bCooked = false;
    Ar << bCooked;
    const bool bHasShapes = CylinderCollision.Num() > 0 || BoxCollision.Num() > 0;
    bool bProcessCookedData = bGatherConvexGeometry || (Version >= VerShapeGeoExport ? bHasShapes : !bHasShapes);
    //! UE 4.3 to 4.7 read the cooked data only for convex geometry.
    if (Ar.GetGame() < EGame::GAME_UE4_8)
        bProcessCookedData = bGatherConvexGeometry;
    if (bCooked && bProcessCookedData)
        CookedFormatData.Serialize(Ar);
    if (Version < VerAreaClass)
        return;
    UObject* AreaClassObject = nullptr;
    Ar << AreaClassObject;
    AreaClass = Cast<UClass>(AreaClassObject);
}

}
