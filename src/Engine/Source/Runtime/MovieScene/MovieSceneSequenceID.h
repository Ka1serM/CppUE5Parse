#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

#include "MovieSceneSequenceID.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneSequenceID {
    GENERATED_BODY()

    UPROPERTY()
    uint32 Value = static_cast<uint32>(-1);

    bool Serialize(FArchive& Ar) {
        Ar << Value;
        return true;
    }
    //! 4.15 to 4.18 key their hierarchy maps by uint32.
    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
        if (Tag.Type != FName("UInt32Property"))
            return false;
        Ar << Value;
        return true;
    }
    bool operator==(const FMovieSceneSequenceID&) const = default;
    friend uint32 GetTypeHash(const FMovieSceneSequenceID& SequenceID) { return SequenceID.Value; }
};
inline FArchive& operator<<(FArchive& Ar, FMovieSceneSequenceID& SequenceID) {
    return Ar << SequenceID.Value;
}

template <>
struct TStructOpsTypeTraits<FMovieSceneSequenceID> : TStructOpsTypeTraitsBase2<FMovieSceneSequenceID> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true, WithSerializeFromMismatchedTag = true };
};

}
