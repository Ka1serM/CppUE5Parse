#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimSubsystem.h"
#include "Engine/Source/Runtime/Engine/Animation/ExposedValueHandler.h"

#include "AnimSubsystem_Base.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAnimSubsystem_Base : public FAnimSubsystem {
    GENERATED_BODY()
    void OnPostLoadDefaults(UClass* InClass) override;
    const TArray<FExposedValueHandler>& GetExposedValueHandlers() const { return ExposedValueHandlers; }
    void PatchValueHandlers(UClass* InClass);

private:
    //! UE 5.0 to 5.3 save the handlers, UE 5.4 builds them from the sparse class data.
    UPROPERTY()
    TArray<FExposedValueHandler> ExposedValueHandlers;
};

}
