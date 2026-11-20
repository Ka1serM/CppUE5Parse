#pragma once

#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "InterpTrack.generated.h"

namespace ue5parse::runtime {

UENUM()
enum ETrackActiveCondition : int32 { ETAC_Always, ETAC_GoreEnabled, ETAC_GoreDisabled };

UCLASS(Abstract)
class UInterpTrack : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TEnumAsByte<ETrackActiveCondition> ActiveCondition = ETAC_Always;
    UPROPERTY()
    FString TrackTitle = "Track";
    UPROPERTY()
    bool bOnePerGroup = false;
    UPROPERTY()
    bool bDirGroupOnly = false;
    UPROPERTY()
    bool bDisableTrack = false;
};

}
