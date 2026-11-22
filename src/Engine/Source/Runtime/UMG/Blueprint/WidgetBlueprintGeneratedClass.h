#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"
#include "Engine/Source/Runtime/Engine/Engine/BlueprintGeneratedClass.h"

#include "WidgetBlueprintGeneratedClass.generated.h"

namespace ue5parse::runtime {

class UUserWidget;
class UWidgetTree;

UCLASS()
class UWidgetBlueprintGeneratedClass : public UBlueprintGeneratedClass {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UWidgetTree> WidgetTree = nullptr;
    //! UE 4.16 to 4.25 cook a template instance of the widget.
    UPROPERTY()
    bool bAllowTemplate = false;
    UPROPERTY()
    bool bValidTemplate = false;
    UPROPERTY()
    TSoftObjectPtr<UUserWidget> TemplateAsset;
};

}
