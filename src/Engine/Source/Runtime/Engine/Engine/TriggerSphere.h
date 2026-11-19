#pragma once

#include "Engine/Source/Runtime/Engine/Engine/TriggerBase.h"

#include "TriggerSphere.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ATriggerSphere : public ATriggerBase {
    GENERATED_BODY()
public:
    explicit ATriggerSphere(const FObjectInitializer& ObjectInitializer);
};

}
