#pragma once

#include <variant>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "MovieSceneEventSection.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovieSceneEventParameters {
    GENERATED_BODY()

    std::variant<FSoftObjectPath, UObject*> StructType;
    TArray<uint8> StructBytes;
    bool operator==(const FMovieSceneEventParameters&) const = default;

    bool Serialize(FArchive& Ar) {
        if (Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::EventSectionParameterStringAssetRef, EGame::GAME_UE4_16))
            Ar << StructType.emplace<FSoftObjectPath>();
        else
            Ar << StructType.emplace<UObject*>(nullptr);
        Ar << StructBytes;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMovieSceneEventParameters> : TStructOpsTypeTraitsBase2<FMovieSceneEventParameters> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
