#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"

namespace ue5parse::runtime {

class FInstancedStructContainer {
public:
    int32 Num() const { return Items.Num(); }
    bool IsValidIndex(int32 Index) const { return Items.IsValidIndex(Index); }
    const FInstancedStruct& operator[](int32 Index) const { return Items[Index]; }
    bool operator==(const FInstancedStructContainer& Other) const { return Items == Other.Items; }
    bool Serialize(FArchive& Ar);

private:
    TArray<FInstancedStruct> Items;
};

template <>
struct TStructOpsTypeTraits<FInstancedStructContainer> : TStructOpsTypeTraitsBase2<FInstancedStructContainer> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
