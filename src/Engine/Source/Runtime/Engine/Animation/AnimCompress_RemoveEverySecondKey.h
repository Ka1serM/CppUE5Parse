#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCompress.h"

#include "AnimCompress_RemoveEverySecondKey.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimCompress_RemoveEverySecondKey : public UAnimCompress {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bStartAtSecondKey = false;

    int32 MinKeys = 10;
};

}
