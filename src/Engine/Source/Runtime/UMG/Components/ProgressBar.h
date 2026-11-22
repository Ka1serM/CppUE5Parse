#pragma once

#include "Engine/Source/Runtime/UMG/Components/Widget.h"

#include "ProgressBar.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UProgressBar : public UWidget {
    GENERATED_BODY()
public:
    UPROPERTY()
    float Percent = 0;
};

}
