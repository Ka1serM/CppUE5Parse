#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvaluationTree.h"

#include "MovieSceneSegment.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ESectionEvaluationFlags : uint8 { None = 0x00, PreRoll = 0x01, PostRoll = 0x02, ForceKeepState = 0x04, ForceRestoreState = 0x08 };

USTRUCT()
struct FMovieSceneSegmentIdentifier {
    GENERATED_BODY()

    UPROPERTY()
    int32 IdentifierIndex = -1;

    bool Serialize(FArchive& Ar) {
        Ar << IdentifierIndex;
        return true;
    }
    bool operator==(const FMovieSceneSegmentIdentifier&) const = default;
};
inline FArchive& operator<<(FArchive& Ar, FMovieSceneSegmentIdentifier& ID) {
    ID.Serialize(Ar);
    return Ar;
}

template <>
struct TStructOpsTypeTraits<FMovieSceneSegmentIdentifier> : TStructOpsTypeTraitsBase2<FMovieSceneSegmentIdentifier> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

//! The 4.15 to 4.19 section data, whose forced time is in seconds.
USTRUCT()
struct FLegacySectionEvaluationData {
    GENERATED_BODY()

    UPROPERTY()
    int32 ImplIndex = -1;
    UPROPERTY()
    float ForcedTime = std::numeric_limits<float>::lowest();
    UPROPERTY()
    ESectionEvaluationFlags Flags = ESectionEvaluationFlags::None;
};

USTRUCT()
struct FSectionEvaluationData {
    GENERATED_BODY()

    UPROPERTY()
    int32 ImplIndex = -1;
    UPROPERTY()
    FFrameNumber ForcedTime{std::numeric_limits<int32>::lowest()};
    UPROPERTY()
    ESectionEvaluationFlags Flags = ESectionEvaluationFlags::None;

    bool Serialize(FArchive& Ar);
    bool operator==(const FSectionEvaluationData&) const = default;
};

template <>
struct TStructOpsTypeTraits<FSectionEvaluationData> : TStructOpsTypeTraitsBase2<FSectionEvaluationData> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};
FArchive& operator<<(FArchive& Ar, FSectionEvaluationData& Data);

USTRUCT()
struct FMovieSceneSegment {
    GENERATED_BODY()

    FFrameNumberRange Range;
    FMovieSceneSegmentIdentifier ID;
    bool bAllowEmpty = false;
    TArray<FSectionEvaluationData> Impls;

    bool Serialize(FArchive& Ar);
    bool operator==(const FMovieSceneSegment&) const = default;
};

template <>
struct TStructOpsTypeTraits<FMovieSceneSegment> : TStructOpsTypeTraitsBase2<FMovieSceneSegment> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
