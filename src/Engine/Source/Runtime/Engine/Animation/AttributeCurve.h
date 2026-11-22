#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/Engine/Curves/IndexedCurve.h"

#include "AttributeCurve.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAttributeKey {
    GENERATED_BODY()
    UPROPERTY()
    float Time = 0;
    FInstancedStruct Value;
};

USTRUCT()
struct FAttributeCurve : public FIndexedCurve {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FAttributeKey> Keys;
    UPROPERTY()
    FSoftObjectPath ScriptStructPath;

    int32 GetNumKeys() const override { return Keys.Num(); }
    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FAttributeCurve> : TStructOpsTypeTraitsBase2<FAttributeCurve> {
    enum { WithSerializer = true };
};

}
