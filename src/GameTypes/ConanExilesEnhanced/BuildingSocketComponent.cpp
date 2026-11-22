#include "GameTypes/ConanExilesEnhanced/BuildingSocketComponent.h"

namespace ue5parse::runtime {

namespace {

template <typename T>
void SerializeOptionalByteArray(FArchive& Ar, TArray<T>& Array) {
    bool bHasArray = false;
    Ar << bHasArray;
    if (!bHasArray || Ar.IsError())
        return;
    const int32 Count = Ar.ReadCount(1);
    Array.Empty(Count);
    if (Count == 0)
        return;
    Array.AddUninitialized(Count);
    Ar.Serialize(Array.GetData(), Count);
}

}

FArchive& operator<<(FArchive& Ar, FSocketStaticData& Data) {
    SerializeOptionalByteArray(Ar, Data.SocketTypes);
    SerializeOptionalByteArray(Ar, Data.TargetSocketTypes);
    Ar << Data.AttachToCost << Data.AttachCost;
    SerializeOptionalByteArray(Ar, Data.OverrideSocketRotations);
    return Ar;
}

void UBuildingSocketComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    Ar.SkipBytes(4);
    Ar << SocketStaticData;
}

}
