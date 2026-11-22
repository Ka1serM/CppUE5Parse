#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/UMG/Components/Widget.h"

#include "WidgetTree.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UWidgetTree : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UWidget> RootWidget = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UWidget>> AllWidgets;
};

}
