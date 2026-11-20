#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionCustomOutput.h"

#include "MaterialExpressionLandscapeGrassOutput.generated.h"

namespace ue5parse::runtime {

class ULandscapeGrassType;

USTRUCT()
struct FGrassInput {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    TObjectPtr<ULandscapeGrassType> GrassType = nullptr;
    UPROPERTY()
    FExpressionInput Input;
};

UCLASS()
class UMaterialExpressionLandscapeGrassOutput : public UMaterialExpressionCustomOutput {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FGrassInput> GrassTypes;
};

}
