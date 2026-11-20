#pragma once

#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Engine/Engine/DataAsset.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/DataLayer/DataLayerType.h"

#include "DataLayerAsset.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EDataLayerLoadFilter : uint8 {
    None,
    ClientOnly,
    ServerOnly,
};

UCLASS()
class UDataLayerAsset : public UDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    EDataLayerType DataLayerType = EDataLayerType::Runtime;
    UPROPERTY()
    bool bSupportsActorFilters = false;
    UPROPERTY()
    FColor DebugColor;
    UPROPERTY()
    EDataLayerLoadFilter LoadFilter = EDataLayerLoadFilter::None;
};

}
