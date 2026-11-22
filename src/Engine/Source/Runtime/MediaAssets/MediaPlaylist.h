#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "MediaPlaylist.generated.h"

namespace ue5parse::runtime {

class UMediaSource;

UCLASS()
class UMediaPlaylist : public UObject {
    GENERATED_BODY()
public:
    explicit UMediaPlaylist(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TArray<TObjectPtr<UMediaSource>> Items;
};

}
