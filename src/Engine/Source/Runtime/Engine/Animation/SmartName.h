#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "SmartName.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSmartName {
    GENERATED_BODY()
    UPROPERTY()
    FName DisplayName;

    FSmartName() = default;
    explicit FSmartName(FName InDisplayName) : DisplayName(InDisplayName) {}
    bool Serialize(FArchive& Ar) {
        Ar << DisplayName;
        return true;
    }
    bool operator==(const FSmartName& Other) const { return DisplayName == Other.DisplayName; }
};

template <>
struct TStructOpsTypeTraits<FSmartName> : TStructOpsTypeTraitsBase2<FSmartName> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
