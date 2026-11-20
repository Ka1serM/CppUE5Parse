#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/UMG/Components/Visual.h"

#include "PanelSlot.generated.h"

namespace ue5parse::runtime {

class UPanelWidget;
class UWidget;

UCLASS()
class UPanelSlot : public UVisual {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPanelWidget> Parent = nullptr;
    UPROPERTY()
    TObjectPtr<UWidget> Content = nullptr;
};

}
