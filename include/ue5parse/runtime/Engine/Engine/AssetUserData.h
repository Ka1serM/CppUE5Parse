#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/CoreUObject/UObject/ObjectPtr.h"

#include "AssetUserData.generated.h"

namespace ue5parse::runtime {

// UAssetUserData: data that an asset carries for a system (UE's Engine/AssetUserData.h).
UCLASS(Abstract)
class UAssetUserData : public UObject {
    GENERATED_BODY()
};

// The first user data of a class in an asset's list; null when it has none (UE's IInterface_AssetUserData::GetAssetUserData).
template <class T>
const T* FindAssetUserData(const TArray<TObjectPtr<UAssetUserData>>& AssetUserData) {
    for (const UAssetUserData* Data : AssetUserData)
        if (const T* Found = Cast<T>(Data)) return Found;
    return nullptr;
}

}
