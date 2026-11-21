#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/ConvexElem.h"

#include "ProceduralMeshComponent.generated.h"

namespace ue5parse::runtime {

class UBodySetup;

USTRUCT()
struct FProcMeshTangent {
    GENERATED_BODY()

    UPROPERTY()
    FVector TangentX{1.0, 0.0, 0.0};
    UPROPERTY()
    bool bFlipTangentY = false;
};

USTRUCT()
struct FProcMeshVertex {
    GENERATED_BODY()

    UPROPERTY()
    FVector Position{0.0, 0.0, 0.0};
    UPROPERTY()
    FVector Normal{0.0, 0.0, 1.0};
    UPROPERTY()
    FProcMeshTangent Tangent;
    UPROPERTY()
    FColor Color{255, 255, 255, 255};
    UPROPERTY()
    FVector2D UV0{0.0, 0.0};
    UPROPERTY()
    FVector2D UV1{0.0, 0.0};
    UPROPERTY()
    FVector2D UV2{0.0, 0.0};
    UPROPERTY()
    FVector2D UV3{0.0, 0.0};
};

USTRUCT()
struct FProcMeshSection {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FProcMeshVertex> ProcVertexBuffer;
    UPROPERTY()
    TArray<uint32> ProcIndexBuffer;
    UPROPERTY()
    FBox SectionLocalBox;
    UPROPERTY()
    bool bEnableCollision = false;
    UPROPERTY()
    bool bSectionVisible = true;
};

UCLASS()
class UProceduralMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UProceduralMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    bool bUseComplexAsSimpleCollision = true;
    UPROPERTY()
    bool bUseAsyncCooking = false;
    UPROPERTY()
    TObjectPtr<UBodySetup> ProcMeshBodySetup = nullptr;
    UPROPERTY()
    TArray<FProcMeshSection> ProcMeshSections;
    UPROPERTY()
    TArray<FKConvexElem> CollisionConvexElems;
    UPROPERTY()
    FBoxSphereBounds LocalBounds;
    UPROPERTY()
    TArray<TObjectPtr<UBodySetup>> AsyncBodySetupQueue;
};

}
