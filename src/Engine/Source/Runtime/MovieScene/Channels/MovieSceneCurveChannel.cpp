#include <cstring>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"
#include "Engine/Source/Runtime/Engine/Curves/RichCurve.h"
#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneDoubleChannel.h"
#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneFloatChannel.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneFrameMigration.h"

namespace ue5parse::runtime {

namespace {

static_assert(sizeof(FMovieSceneFloatValue) == 28);
static_assert(sizeof(FMovieSceneDoubleValue) == 32);

bool SerializesChannelValues(const FArchive& Ar) {
    return Ar.CustomVerAtLeast(FSequencerObjectVersion::GUID, FSequencerObjectVersion::SerializeFloatChannel, EGame::GAME_UE4_22);
}

bool SerializesChannelsCompletely(const FArchive& Ar) {
    return Ar.CustomVerAtLeast(FSequencerObjectVersion::GUID, FSequencerObjectVersion::SerializeFloatChannelCompletely, EGame::GAME_UE4_25);
}

template <class TValue>
void SerializeChannelValue(FArchive& Ar, TValue& Value) {
    if (!SerializesChannelsCompletely(Ar)) {
        Ar << Value.InterpMode << Value.TangentMode << Value.Tangent;
        return;
    }
    FMovieSceneTangentData& Tangent = Value.Tangent;
    Ar << Tangent.ArriveTangent << Tangent.LeaveTangent << Tangent.ArriveTangentWeight << Tangent.LeaveTangentWeight << Tangent.TangentWeightMode;
    Ar << Value.InterpMode << Value.TangentMode << Value.PaddingByte;
}

template <class T>
void SerializeBulkArray(FArchive& Ar, TArray<T>& Array) {
    int32 ElementSize = 0;
    Ar << ElementSize;
    if (ElementSize != static_cast<int32>(sizeof(T))) {
        Ar << Array;
        return;
    }
    const int32 Count = Ar.ReadCount(ElementSize);
    Array.SetNum(Count);
    if (Count > 0)
        Ar.Serialize(Array.GetData(), static_cast<int64>(Count) * ElementSize);
}

template <class TChannel>
bool SerializeChannel(FArchive& Ar, TChannel& Channel) {
    const bool bShowCurve =
        Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::SerializeFloatChannelShowCurve, EGame::GAME_UE5_0);
    if (!SerializesChannelsCompletely(Ar) && !bShowCurve)
        return false;
    //! Delta Force saves its movie scene channels as tagged properties.
    if (Ar.GetGame() == EGame::GAME_DeltaForce)
        return false;
    Ar << Channel.PreInfinityExtrap << Channel.PostInfinityExtrap;
    SerializeBulkArray(Ar, Channel.Times);
    SerializeBulkArray(Ar, Channel.Values);
    //! Split Fiction saves a second copy of the values.
    if (Ar.GetGame() == EGame::GAME_SplitFiction) {
        const int32 ElementSize = Ar.Read<int32>();
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(ElementSize)) * ElementSize);
    }
    Ar << Channel.DefaultValue << Channel.bHasDefaultValue << Channel.TickResolution.Numerator << Channel.TickResolution.Denominator;
    if (bShowCurve)
        Ar << Channel.bShowCurve;
    if (Ar.GetGame() == EGame::GAME_SplitFiction)
        Ar.SkipBytes(4);
    return true;
}

template <class TChannel>
bool SerializeChannelFromRichCurve(const FPropertyTag& Tag, FArchive& Ar, TChannel& Channel) {
    if (Tag.Type != FName("StructProperty") || Tag.TypeName.GetParameterName(0) != FName("RichCurve"))
        return false;
    FRichCurve Curve;
    FRichCurve::StaticStruct()->SerializeItem(Ar, &Curve);
    if (Curve.DefaultValue != UE_MAX_FLT) {
        Channel.bHasDefaultValue = true;
        Channel.DefaultValue = Curve.DefaultValue;
    }
    Channel.PreInfinityExtrap = Curve.PreInfinityExtrap;
    Channel.PostInfinityExtrap = Curve.PostInfinityExtrap;
    const FFrameRate LegacyFrameRate = GetLegacyConversionFrameRate();
    const float Interval = static_cast<float>(LegacyFrameRate.AsInterval());
    for (const FRichCurveKey& Key : Curve.Keys) {
        typename TChannel::ValueType Value;
        Value.Value = Key.Value;
        Value.InterpMode = Key.InterpMode;
        Value.TangentMode = Key.TangentMode;
        Value.Tangent.ArriveTangent = Key.ArriveTangent * Interval;
        Value.Tangent.LeaveTangent = Key.LeaveTangent * Interval;
        const FFrameNumber Time = UpgradeLegacyMovieSceneTime(LegacyFrameRate, Key.Time);
        int32 Index = Channel.Times.Num();
        while (Index > 0 && Channel.Times[Index - 1].Value > Time.Value)
            Index--;
        Channel.Times.Insert(Time, Index);
        Channel.Values.Insert(Value, Index);
    }
    return true;
}

}

bool FMovieSceneTangentData::Serialize(FArchive& Ar) {
    if (!SerializesChannelValues(Ar))
        return false;
    if (!SerializesChannelsCompletely(Ar)) {
        Ar << ArriveTangent << LeaveTangent << TangentWeightMode << ArriveTangentWeight << LeaveTangentWeight;
        return true;
    }
    Ar << ArriveTangent << LeaveTangent << ArriveTangentWeight << LeaveTangentWeight << TangentWeightMode;
    return true;
}

bool FMovieSceneFloatValue::Serialize(FArchive& Ar) {
    if (!SerializesChannelValues(Ar))
        return false;
    Ar << Value;
    SerializeChannelValue(Ar, *this);
    return true;
}

bool FMovieSceneDoubleValue::Serialize(FArchive& Ar) {
    if (!SerializesChannelValues(Ar))
        return false;
    if (Ar.UEVer() >= EUnrealEngineObjectUE5Version::LARGE_WORLD_COORDINATES) {
        Ar << Value;
    } else {
        float FloatValue = 0;
        Ar << FloatValue;
        Value = FloatValue;
    }
    SerializeChannelValue(Ar, *this);
    return true;
}

bool FMovieSceneFloatChannel::Serialize(FArchive& Ar) {
    return SerializeChannel(Ar, *this);
}

bool FMovieSceneFloatChannel::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    return SerializeChannelFromRichCurve(Tag, Ar, *this);
}

bool FMovieSceneDoubleChannel::Serialize(FArchive& Ar) {
    return SerializeChannel(Ar, *this);
}

bool FMovieSceneDoubleChannel::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (SerializeChannelFromRichCurve(Tag, Ar, *this))
        return true;
    if (Tag.Type != FName("StructProperty") || Tag.TypeName.GetParameterName(0) != FName("MovieSceneFloatChannel"))
        return false;
    FMovieSceneFloatChannel FloatChannel;
    FloatChannel.PreInfinityExtrap = PreInfinityExtrap;
    FloatChannel.PostInfinityExtrap = PostInfinityExtrap;
    FloatChannel.DefaultValue = static_cast<float>(DefaultValue);
    FloatChannel.bHasDefaultValue = bHasDefaultValue;
    FloatChannel.TickResolution = TickResolution;
    FMovieSceneFloatChannel::StaticStruct()->SerializeItem(Ar, &FloatChannel);
    PreInfinityExtrap = FloatChannel.PreInfinityExtrap;
    PostInfinityExtrap = FloatChannel.PostInfinityExtrap;
    Times = FloatChannel.Times;
    Values.SetNum(FloatChannel.Values.Num());
    for (int32 Index = 0; Index < FloatChannel.Values.Num(); Index++) {
        const FMovieSceneFloatValue& Source = FloatChannel.Values[Index];
        FMovieSceneDoubleValue& Target = Values[Index];
        Target.Value = Source.Value;
        Target.Tangent = Source.Tangent;
        Target.InterpMode = Source.InterpMode;
        Target.TangentMode = Source.TangentMode;
    }
    DefaultValue = FloatChannel.DefaultValue;
    bHasDefaultValue = FloatChannel.bHasDefaultValue;
    TickResolution = FloatChannel.TickResolution;
    bShowCurve = FloatChannel.bShowCurve;
    return true;
}

}
