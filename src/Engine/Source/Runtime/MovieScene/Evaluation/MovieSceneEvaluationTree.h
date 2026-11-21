#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneFrameMigration.h"

namespace ue5parse::runtime {

struct FEvaluationTreeEntryHandle {
    int32 EntryIndex = -1;
    bool operator==(const FEvaluationTreeEntryHandle&) const = default;
};
inline FArchive& operator<<(FArchive& Ar, FEvaluationTreeEntryHandle& Handle) {
    return Ar << Handle.EntryIndex;
}

template <class ElementType>
struct TEvaluationTreeEntryContainer {
    struct FEntry {
        int32 StartIndex = 0;
        int32 Size = 0;
        int32 Capacity = 0;
        bool operator==(const FEntry&) const = default;
    };
    friend FArchive& operator<<(FArchive& Ar, FEntry& Entry) { return Ar << Entry.StartIndex << Entry.Size << Entry.Capacity; }
    friend FArchive& operator<<(FArchive& Ar, TEvaluationTreeEntryContainer& Container) { return Ar << Container.Entries << Container.Items; }
    bool operator==(const TEvaluationTreeEntryContainer&) const = default;

    TArray<FEntry> Entries;
    TArray<ElementType> Items;
};

struct FMovieSceneEvaluationTreeNodeHandle {
    FEvaluationTreeEntryHandle ChildrenHandle;
    int32 Index = -1;
    bool operator==(const FMovieSceneEvaluationTreeNodeHandle&) const = default;
};
inline FArchive& operator<<(FArchive& Ar, FMovieSceneEvaluationTreeNodeHandle& Handle) {
    return Ar << Handle.ChildrenHandle << Handle.Index;
}

//! 4.19 stores float ranges, which 4.20 converted to frame numbers at the legacy 60000 fps.
inline FArchive& SerializeMovieSceneFrameNumberRange(FArchive& Ar, FFrameNumberRange& Range) {
    if (Ar.CustomVerAtLeast(FSequencerObjectVersion::GUID, FSequencerObjectVersion::FloatToIntConversion, EGame::GAME_UE4_20))
        return Ar << Range.LowerBound << Range.UpperBound;
    FFloatRangeBound Lower;
    FFloatRangeBound Upper;
    TEnumAsByte<ERangeBoundTypes::Type> LowerType;
    TEnumAsByte<ERangeBoundTypes::Type> UpperType;
    Ar << LowerType << Lower.Value << UpperType << Upper.Value;
    Lower.Type = LowerType;
    Upper.Type = UpperType;
    Range = {UpgradeLegacyRangeBound(Lower), UpgradeLegacyRangeBound(Upper)};
    return Ar;
}

struct FMovieSceneEvaluationTreeNode {
    FFrameNumberRange Range;
    FMovieSceneEvaluationTreeNodeHandle Parent;
    FEvaluationTreeEntryHandle ChildrenID;
    FEvaluationTreeEntryHandle DataID;
    bool operator==(const FMovieSceneEvaluationTreeNode&) const = default;
};
inline FArchive& operator<<(FArchive& Ar, FMovieSceneEvaluationTreeNode& Node) {
    SerializeMovieSceneFrameNumberRange(Ar, Node.Range);
    return Ar << Node.Parent << Node.ChildrenID << Node.DataID;
}

struct FMovieSceneEvaluationTree {
    FMovieSceneEvaluationTreeNode RootNode;
    TEvaluationTreeEntryContainer<FMovieSceneEvaluationTreeNode> ChildNodes;
    bool operator==(const FMovieSceneEvaluationTree&) const = default;
};
inline FArchive& operator<<(FArchive& Ar, FMovieSceneEvaluationTree& Tree) {
    return Ar << Tree.RootNode << Tree.ChildNodes;
}

template <class DataType>
struct TMovieSceneEvaluationTree : FMovieSceneEvaluationTree {
    TEvaluationTreeEntryContainer<DataType> Data;
    bool operator==(const TMovieSceneEvaluationTree&) const = default;
    friend FArchive& operator<<(FArchive& Ar, TMovieSceneEvaluationTree& Tree) { return Ar << static_cast<FMovieSceneEvaluationTree&>(Tree) << Tree.Data; }
};

}
