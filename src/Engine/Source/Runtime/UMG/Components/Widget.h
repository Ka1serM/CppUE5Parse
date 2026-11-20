#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/UMG/Components/Visual.h"

#include "Widget.generated.h"

namespace ue5parse::runtime {

class UPanelSlot;

UCLASS(Abstract)
class UWidget : public UVisual {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPanelSlot> Slot = nullptr;
};

}
