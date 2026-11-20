#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "GeometryMaskCanvasActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AGeometryMaskCanvasActor : public AActor {
    GENERATED_BODY()
public:
    explicit AGeometryMaskCanvasActor(const FObjectInitializer& ObjectInitializer);
};

}
