#pragma once

#include "Engine/Source/Runtime/Engine/Engine/TriggerBase.h"

#include "TriggerBox.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ATriggerBox : public ATriggerBase {
    GENERATED_BODY()
public:
    explicit ATriggerBox(const FObjectInitializer& ObjectInitializer);
};

}
