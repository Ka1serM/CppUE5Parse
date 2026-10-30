#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"

#include "AresWallPenetration.generated.h"

// A native class of VALORANT, declared outside the library as Verylia declares its game classes: it registers itself
// when linked, and the runtime builds AresWallPenetration and its Blueprint subclasses on it.

namespace shootergame {

UCLASS()
class UAresWallPenetration : public ue5parse::runtime::UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    float EnergyReductionMultiplier = 0;
    UPROPERTY()
    bool bParticipatesInAirDropoff = false;
    UPROPERTY()
    bool bShowsRicochet = false;
    UPROPERTY()
    ue5parse::runtime::FString DebugWallPenetrationTypeName;

    // A native function in another namespace than the runtime's, for its generated thunk.
    UFUNCTION(BlueprintPure)
    float GetEnergyAfter(float Energy, bool bRicochet) const { return bRicochet && bShowsRicochet ? Energy : Energy * EnergyReductionMultiplier; }

    // An event that only Blueprints implement, and one with a native body: both run through ProcessEvent.
    UFUNCTION(BlueprintImplementableEvent)
    void ReceivePenetrated(float Energy, const ue5parse::runtime::FVector& Location);
    UFUNCTION(BlueprintNativeEvent)
    float ScaleEnergy(float Energy, ue5parse::runtime::FName& OutReason) const;
    virtual float ScaleEnergy_Implementation(float Energy, ue5parse::runtime::FName& OutReason) const {
        OutReason = ue5parse::runtime::FName("Native");
        return Energy * EnergyReductionMultiplier;
    }
    UFUNCTION(BlueprintCallable)
    void SetNames(const ue5parse::runtime::TArray<ue5parse::runtime::FName>& Names, ue5parse::runtime::UObject* Owner, ue5parse::runtime::UClass* Class) {}
};

}
