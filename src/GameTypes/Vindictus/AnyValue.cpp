#include "GameTypes/Vindictus/AnyValue.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"

namespace ue5parse::runtime {

bool FAnyValue::Serialize(FArchive& Ar) {
    Ar.SkipBytes(1);
    FPackageIndex StructIndex;
    Ar << StructIndex;
    if (!Ar.IsError())
        SerializeStructByObject(Ar, StructIndex, NonConstStruct);
    return true;
}

}
