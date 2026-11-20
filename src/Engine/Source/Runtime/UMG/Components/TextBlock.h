#pragma once

#include "Engine/Source/Runtime/Core/Internationalization/Text.h"
#include "Engine/Source/Runtime/UMG/Components/TextWidgetTypes.h"

#include "TextBlock.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UTextBlock : public UTextLayoutWidget {
    GENERATED_BODY()
public:
    UPROPERTY()
    FText Text;
};

}
