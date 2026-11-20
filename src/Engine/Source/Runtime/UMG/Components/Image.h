#pragma once

#include "Engine/Source/Runtime/SlateCore/Styling/SlateBrush.h"
#include "Engine/Source/Runtime/UMG/Components/Widget.h"

#include "Image.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UImage : public UWidget {
    GENERATED_BODY()
public:
    UPROPERTY()
    FSlateBrush Brush;
};

}
