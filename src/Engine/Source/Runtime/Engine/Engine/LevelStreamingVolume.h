#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "LevelStreamingVolume.generated.h"

namespace ue5parse::runtime {

class ULevelStreaming;

UENUM()
enum EStreamingVolumeUsage : int32 { SVB_Loading, SVB_LoadingAndVisibility, SVB_VisibilityBlockingOnLoad, SVB_BlockingOnLoad, SVB_LoadingNotVisible, SVB_MAX };

UCLASS()
class ALevelStreamingVolume : public AVolume {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<ULevelStreaming>> StreamingLevels;
    UPROPERTY()
    uint8 bEditorPreVisOnly : 1 = false;
    UPROPERTY()
    uint8 bDisabled : 1 = false;
    UPROPERTY()
    TEnumAsByte<EStreamingVolumeUsage> StreamingUsage = SVB_LoadingAndVisibility;
    UPROPERTY()
    TArray<FName> StreamingLevelNames;

    explicit ALevelStreamingVolume(const FObjectInitializer& ObjectInitializer);
};

}
