#pragma once

#include "Engine/Source/Runtime/Engine/Materials/MaterialFunctionInterface.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialTranslatorProfile.h"

#include "MaterialFunction.generated.h"

namespace ue5parse::runtime {

class UMaterialExpression;

UCLASS()
class UMaterialFunction : public UMaterialFunctionInterface {
    GENERATED_BODY()
public:
    UPROPERTY()
    FString Description;
    UPROPERTY()
    bool bExposeToLibrary = false;
    UPROPERTY()
    TArray<FString> LibraryCategories;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialExpression>> FunctionExpressions;
    UPROPERTY()
    bool bReentrantFlag = false;
    UPROPERTY()
    TArray<FText> LibraryCategoriesText;
    UPROPERTY()
    bool bPrefixParameterNames = false;
    UPROPERTY()
    bool bEnableExecWire = false;
    UPROPERTY()
    bool bEnableNewHLSLGenerator = false;
    UPROPERTY()
    FString UserExposedCaption;
    UPROPERTY()
    bool bEnableNewHLSLGeneratorOverridden = false;
    UPROPERTY()
    bool bMaterialTranslatorProfileOverridden = false;
    UPROPERTY()
    EMaterialTranslatorProfile MaterialTranslatorProfile = EMaterialTranslatorProfile::Compatibility;
};

}
