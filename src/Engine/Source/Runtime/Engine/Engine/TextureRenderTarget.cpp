#include "Engine/Source/Runtime/Engine/Engine/TextureRenderTarget.h"

namespace ue5parse::runtime {

UTextureRenderTarget::UTextureRenderTarget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    NeverStream = true;
    SRGB = true;
    LODGroup = TEXTUREGROUP_RenderTarget;
}

}
