#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "SoundGroups.generated.h"

namespace ue5parse::runtime {

UENUM()
enum ESoundGroup : int32 {
    SOUNDGROUP_Default,
    SOUNDGROUP_Effects,
    SOUNDGROUP_UI,
    SOUNDGROUP_Music,
    SOUNDGROUP_Voice,
    SOUNDGROUP_GameSoundGroup1,
    SOUNDGROUP_GameSoundGroup2,
    SOUNDGROUP_GameSoundGroup3,
    SOUNDGROUP_GameSoundGroup4,
    SOUNDGROUP_GameSoundGroup5,
    SOUNDGROUP_GameSoundGroup6,
    SOUNDGROUP_GameSoundGroup7,
    SOUNDGROUP_GameSoundGroup8,
    SOUNDGROUP_GameSoundGroup9,
    SOUNDGROUP_GameSoundGroup10,
    SOUNDGROUP_GameSoundGroup11,
    SOUNDGROUP_GameSoundGroup12,
    SOUNDGROUP_GameSoundGroup13,
    SOUNDGROUP_GameSoundGroup14,
    SOUNDGROUP_GameSoundGroup15,
    SOUNDGROUP_GameSoundGroup16,
    SOUNDGROUP_GameSoundGroup17,
    SOUNDGROUP_GameSoundGroup18,
    SOUNDGROUP_GameSoundGroup19,
    SOUNDGROUP_GameSoundGroup20,
};

USTRUCT()
struct FSoundGroup {
    GENERATED_BODY()
    UPROPERTY()
    TEnumAsByte<ESoundGroup> SoundGroup = SOUNDGROUP_Default;
    UPROPERTY()
    FString DisplayName;
    UPROPERTY()
    uint32 bAlwaysDecompressOnLoad : 1 = 0;
    UPROPERTY()
    float DecompressedDuration = 0.0f;
};

UCLASS()
class USoundGroups : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FSoundGroup> SoundGroupProfiles;
};

}
