#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"
#include "ue5parse/runtime/Engine/Components/TimelineComponent.h"
#include "ue5parse/runtime/Engine/Engine/EngineBaseTypes.h"

#include "TimelineTemplate.generated.h"

namespace ue5parse::runtime {

class UCurveFloat;
class UCurveLinearColor;
class UCurveVector;

USTRUCT()
struct FTTTrackBase {
    GENERATED_BODY()
    UPROPERTY()
    FName TrackName;
    UPROPERTY()
    bool bIsExternalCurve = false;

    virtual ~FTTTrackBase() = default;
    FName GetTrackName() const { return TrackName; }
};

USTRUCT()
struct FTTEventTrack : public FTTTrackBase {
    GENERATED_BODY()
    UPROPERTY()
    FName FunctionName;
    UPROPERTY()
    UCurveFloat* CurveKeys = nullptr;

    FName GetFunctionName() const { return FunctionName; }
};

USTRUCT()
struct FTTPropertyTrack : public FTTTrackBase {
    GENERATED_BODY()
    UPROPERTY()
    FName PropertyName;

    FName GetPropertyName() const { return PropertyName; }
};

USTRUCT()
struct FTTFloatTrack : public FTTPropertyTrack {
    GENERATED_BODY()
    UPROPERTY()
    UCurveFloat* CurveFloat = nullptr;
};

USTRUCT()
struct FTTVectorTrack : public FTTPropertyTrack {
    GENERATED_BODY()
    UPROPERTY()
    UCurveVector* CurveVector = nullptr;
};

USTRUCT()
struct FTTLinearColorTrack : public FTTPropertyTrack {
    GENERATED_BODY()
    UPROPERTY()
    UCurveLinearColor* CurveLinearColor = nullptr;
};

// UTimelineTemplate: a Blueprint timeline: its length, its tracks, and the Blueprint functions and variables it drives
// (UE's Engine/TimelineTemplate.h).
UCLASS()
class UTimelineTemplate : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    float TimelineLength = 5.0f;
    UPROPERTY()
    float TimelineTickInterval = 0;
    UPROPERTY()
    TEnumAsByte<ETimelineLengthMode> LengthMode = TL_TimelineLength;
    UPROPERTY()
    bool bAutoPlay = false;
    UPROPERTY()
    bool bLoop = false;
    UPROPERTY()
    bool bReplicated = false;
    UPROPERTY()
    bool bIgnoreTimeDilation = false;
    UPROPERTY()
    TArray<FTTEventTrack> EventTracks;
    UPROPERTY()
    TArray<FTTFloatTrack> FloatTracks;
    UPROPERTY()
    TArray<FTTVectorTrack> VectorTracks;
    UPROPERTY()
    TArray<FTTLinearColorTrack> LinearColorTracks;
    UPROPERTY()
    FGuid TimelineGuid;
    UPROPERTY()
    TEnumAsByte<ETickingGroup> TimelineTickGroup = TG_PrePhysics;
    UPROPERTY()
    FName VariableName;
    UPROPERTY()
    FName DirectionPropertyName;
    UPROPERTY()
    FName UpdateFunctionName;
    UPROPERTY()
    FName FinishedFunctionName;

    FName GetVariableName() const { return VariableName; }
    FName GetDirectionPropertyName() const { return DirectionPropertyName; }
    FName GetUpdateFunctionName() const { return UpdateFunctionName; }
    FName GetFinishedFunctionName() const { return FinishedFunctionName; }
};

}
