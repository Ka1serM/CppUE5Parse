#pragma once

#include "Engine/Plugins/Experimental/SlateIM/Source/SlateIMInGame/SlateIMInGameWidgetBase.h"

#include "SlateIMInGameNomadTab.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ASlateIMInGameNomadTab : public ASlateIMInGameWidgetBase {
    GENERATED_BODY()
public:
    explicit ASlateIMInGameNomadTab(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FName TabName;
    UPROPERTY()
    FText TabTitle;
};

}
