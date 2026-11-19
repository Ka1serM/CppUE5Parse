#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "ModelComponent.generated.h"

namespace ue5parse::runtime {

class UBodySetup;
class UMaterialInterface;
class UModel;
class UModelComponent;

struct FModelElement {
    TObjectPtr<UModelComponent> Component = nullptr;
    TObjectPtr<UMaterialInterface> Material = nullptr;
    TArray<uint16> Nodes;
    FGuid MapBuildDataId;

    friend FArchive& operator<<(FArchive& Ar, FModelElement& Element);
};

UCLASS()
class UModelComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UBodySetup> ModelBodySetup = nullptr;

    TObjectPtr<UModel> Model = nullptr;
    int32 ComponentIndex = 0;
    TArray<uint16> Nodes;
    TArray<FModelElement> Elements;

    explicit UModelComponent(const FObjectInitializer& ObjectInitializer);
    void Serialize(FArchive& Ar) override;
};

}
