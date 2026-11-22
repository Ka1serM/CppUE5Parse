#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "BendCollectionComponents.generated.h"

namespace ue5parse::runtime {

//! Days Gone saves no data that CUE4Parse reads for its Bend collection components, so their exports stay empty.
UCLASS()
class UCustomBendClass : public UObject {
    GENERATED_BODY()
public:
    explicit UCustomBendClass(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override {
        if (Ar.GetGame() != EGame::GAME_DaysGone)
            Super::Serialize(Ar);
    }
};

UCLASS()
class UBendBlockingVolumeCollectionComponent : public UCustomBendClass {
    GENERATED_BODY()
public:
    explicit UBendBlockingVolumeCollectionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UBendDecalCollectionComponent : public UCustomBendClass {
    GENERATED_BODY()
public:
    explicit UBendDecalCollectionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UBendNavLinkCollectionComponent : public UCustomBendClass {
    GENERATED_BODY()
public:
    explicit UBendNavLinkCollectionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UBendStaticMeshCollectionComponent : public UCustomBendClass {
    GENERATED_BODY()
public:
    explicit UBendStaticMeshCollectionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
