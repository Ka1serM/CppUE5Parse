#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCompress_BitwiseCompressOnly.h"

#include "AnimCompress_LeastDestructive.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAnimCompress_LeastDestructive : public UAnimCompress_BitwiseCompressOnly {
    GENERATED_BODY()
};

}
