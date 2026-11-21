#pragma once

#include <optional>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvaluationTree.h"

namespace ue5parse::runtime {

struct FMovieSceneWarpCounter {
    TArray<uint32> WarpCounts;
    bool operator==(const FMovieSceneWarpCounter&) const = default;
};

struct FMovieSceneSubSequenceTreeEntry {
    uint32 SequenceID = 0;
    uint8 Flags = 0;
    std::optional<FMovieSceneWarpCounter> RootToSequenceWarpCounter;
    bool operator==(const FMovieSceneSubSequenceTreeEntry&) const = default;

    friend FArchive& operator<<(FArchive& Ar, FMovieSceneSubSequenceTreeEntry& Entry) {
        Ar << Entry.SequenceID << Entry.Flags;
        if (Ar.IsError() || Ar.GetGame() >= EGame::GAME_UE5_5)
            return Ar;
        if (Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::AddedSubSequenceEntryWarpCounter, EGame::GAME_UE4_27) ||
            (Ar.GetGame() >= EGame::GAME_UE5_0 &&
             Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::AddedSubSequenceEntryWarpCounter, EGame::GAME_UE5_0))) {
            const UScriptStruct* Struct = FindNativeStruct(FName("MovieSceneWarpCounter"));
            if (Struct == nullptr)
                Ar.SetError(ErrorCode::MissingSchema, "MovieSceneWarpCounter");
            else
                Struct->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&Entry.RootToSequenceWarpCounter.emplace()));
        }
        return Ar;
    }
};

struct FMovieSceneSubSequenceTree {
    TMovieSceneEvaluationTree<FMovieSceneSubSequenceTreeEntry> Data;
    bool operator==(const FMovieSceneSubSequenceTree&) const = default;

    bool Serialize(FArchive& Ar) {
        Ar << Data;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMovieSceneSubSequenceTree> : TStructOpsTypeTraitsBase2<FMovieSceneSubSequenceTree> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
