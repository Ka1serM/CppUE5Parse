#include "Engine/Source/Runtime/Engine/Components/ReflectionCaptureComponent.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

UReflectionCaptureComponent::UReflectionCaptureComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Mobility = EComponentMobility::Static;
}

//! UE 4.19 moved the captured data to the map build data.
void UReflectionCaptureComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetGame() >= EGame::GAME_UE4_19 || Ar.UEVer() < EUnrealEngineObjectUE4Version::REFLECTION_CAPTURE_COOKING)
        return;
    bool bCooked = false;
    Ar << bCooked;
    if (!bCooked) {
        Ar.SetError(ErrorCode::InvalidData, "UReflectionCaptureComponent.bCooked");
        return;
    }
    if (Ar.GetGame() >= EGame::GAME_UE4_14)
        Ar << AverageBrightness;
    const bool bHasCubemapSize = Ar.GetGame() >= EGame::GAME_UE4_14 ||
                                 Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::CustomReflectionCaptureResolutionSupport, EGame::GAME_UE4_12);
    int32 NumFormats = 0;
    Ar << NumFormats;
    for (int32 FormatIndex = 0; FormatIndex < NumFormats && !Ar.IsError(); FormatIndex++) {
        FName CurrentFormat;
        bool bValid = false;
        Ar << CurrentFormat << bValid;
        if (!bValid)
            continue;
        if (CurrentFormat != FName("FullHDR")) {
            Ar << EncodedHDRCapturedData;
            continue;
        }
        if (bHasCubemapSize)
            Ar << FullHDRCubemapSize;
        Ar << FullHDRCapturedData;
    }
}

}
