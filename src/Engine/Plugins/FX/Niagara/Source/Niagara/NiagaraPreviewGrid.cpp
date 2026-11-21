#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraPreviewGrid.h"

namespace ue5parse::runtime {

ANiagaraPreviewGrid::ANiagaraPreviewGrid(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
}

}
