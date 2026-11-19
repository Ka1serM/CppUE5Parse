#pragma once

#include "Engine/Source/Runtime/Engine/Engine/TriggerBase.h"

#include "TriggerCapsule.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ATriggerCapsule : public ATriggerBase {
    GENERATED_BODY()
public:
    explicit ATriggerCapsule(const FObjectInitializer& ObjectInitializer);
};

}
