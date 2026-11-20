#pragma once

#include "Engine/Plugins/Experimental/SlateIM/Source/SlateIMInGame/SlateIMInGameWidgetBase.h"

#include "SlateIMInGameWindow.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ASlateIMInGameWindow : public ASlateIMInGameWidgetBase {
    GENERATED_BODY()
public:
    explicit ASlateIMInGameWindow(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FName WindowName;
    UPROPERTY()
    FString WindowTitle;
    UPROPERTY()
    FVector2f WindowSize = FVector2f(500, 500);
};

}
