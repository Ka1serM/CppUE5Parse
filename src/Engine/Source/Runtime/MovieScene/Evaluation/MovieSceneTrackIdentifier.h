#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

#include "MovieSceneTrackIdentifier.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneTrackIdentifier {
    GENERATED_BODY()

    UPROPERTY()
    uint32 Value = static_cast<uint32>(-1);

    //! 4.15 and 4.16 save the identifier as tagged properties.
    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() < EGame::GAME_UE4_17)
            return false;
        Ar << Value;
        return true;
    }
    //! 4.15 to 4.18 key their track maps by uint32.
    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
        if (Tag.Type != FName("UInt32Property"))
            return false;
        Ar << Value;
        return true;
    }
    bool operator==(const FMovieSceneTrackIdentifier&) const = default;
    friend uint32 GetTypeHash(const FMovieSceneTrackIdentifier& Identifier) { return Identifier.Value; }
};
inline FArchive& operator<<(FArchive& Ar, FMovieSceneTrackIdentifier& Identifier) {
    return Ar << Identifier.Value;
}

template <>
struct TStructOpsTypeTraits<FMovieSceneTrackIdentifier> : TStructOpsTypeTraitsBase2<FMovieSceneTrackIdentifier> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true, WithSerializeFromMismatchedTag = true };
};

USTRUCT()
struct FMovieSceneTrackIdentifiers {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMovieSceneTrackIdentifier> Data;

    //! Game for Peace saves the identifiers as an array without property tags.
    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_GameForPeace)
            return false;
        Ar << Data;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMovieSceneTrackIdentifiers> : TStructOpsTypeTraitsBase2<FMovieSceneTrackIdentifiers> {
    enum { WithSerializer = true };
};

}
