#include "ue5parse/runtime/Engine/Animation/BlendSpace.h"

#include "ue5parse/runtime/Engine/Animation/BlendSpace1D.h"

namespace ue5parse::runtime {

UBlendSpace::UBlendSpace(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DimensionIndices = {0, 1};
}

UBlendSpace1D::UBlendSpace1D(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DimensionIndices = {0};
}

}
