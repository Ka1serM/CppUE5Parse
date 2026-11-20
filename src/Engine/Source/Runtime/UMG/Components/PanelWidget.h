#pragma once

#include "Engine/Source/Runtime/UMG/Components/Widget.h"

#include "PanelWidget.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UPanelWidget : public UWidget {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<UPanelSlot>> Slots;
};

}
