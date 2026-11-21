#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "ChaosSolverActor.generated.h"

namespace ue5parse::runtime {

class UBillboardComponent;
class UChaosGameplayEventDispatcher;

UENUM()
enum class EClusterConnectionTypeEnum : uint8 {
    Chaos_PointImplicit,
    Chaos_DelaunayTriangulation,
    Chaos_MinimalSpanningSubsetDelaunayTriangulation,
    Chaos_PointImplicitAugmentedWithMinimalDelaunay,
    Chaos_BoundsOverlapFilteredDelaunayTriangulation,
    Chaos_None,
    Chaos_EClsuterCreationParameters_Max
};

UCLASS()
class AChaosSolverActor : public AActor {
    GENERATED_BODY()
public:
    explicit AChaosSolverActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UChaosGameplayEventDispatcher> GameplayEventDispatcherComponent = nullptr;
};

}
