#include "GameTypes/AssaultFireFuture/AssaultFireFutureStructs.h"

#include "GameTypes/OuterWorlds2/PropertyDataWithObjects.h"

namespace ue5parse::runtime {

bool FAnnotationPointData2::Serialize(FArchive& Ar) {
    if (Ar.GetGame() != EGame::GAME_AssaultFireFuture)
        return false;
    Ar << Type << SomeVector;
    switch (Type) {
    case 4:
        Ar << SomeByte << SomeStructVector;
        break;
    case 5:
        SerializeTaggedPropertiesWithoutStruct(Ar, SomeStruct);
        break;
    case 6:
        Ar << SomeStructPath << SomeStructVector;
        SerializeTaggedPropertiesWithoutStruct(Ar, SomeStruct);
        Ar << SomeStructFloat;
        break;
    default:
        break;
    }
    if (Ar.IsError())
        return true;
    Ar << SomeVector2D;
    SerializeTaggedPropertiesWithoutStruct(Ar, StructFallback2);
    return true;
}

}
