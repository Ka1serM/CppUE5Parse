#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "AssetUserData.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UAssetUserData : public UObject {
    GENERATED_BODY()
};

template <class T>
const T* FindAssetUserData(const TArray<TObjectPtr<UAssetUserData>>& AssetUserData) {
    for (const UAssetUserData* Data : AssetUserData)
        if (const T* Found = Cast<T>(Data)) return Found;
    return nullptr;
}

}
