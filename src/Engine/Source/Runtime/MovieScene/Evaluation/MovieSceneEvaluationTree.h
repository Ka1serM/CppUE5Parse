#pragma once

#include "Engine/Source/Runtime/MovieScene/MovieSceneFrameMigration.h"

namespace ue5parse::runtime {

struct FEvaluationTreeEntryHandle {
    int32 EntryIndex = INDEX_NONE;
    bool operator==(const FEvaluationTreeEntryHandle&) const = default;

    friend FArchive& operator<<(FArchive& Ar, FEvaluationTreeEntryHandle& Handle) { return Ar << Handle.EntryIndex; }
};

struct FMovieSceneEvaluationTreeNodeHandle {
    FEvaluationTreeEntryHandle ChildrenHandle;
    int32 Index = INDEX_NONE;
    bool operator==(const FMovieSceneEvaluationTreeNodeHandle&) const = default;
};

struct FMovieSceneEvaluationTreeNode {
    FMovieSceneFrameRange Range;
    FMovieSceneEvaluationTreeNodeHandle Parent;
    FEvaluationTreeEntryHandle ChildrenID;
    FEvaluationTreeEntryHandle DataID;
    bool operator==(const FMovieSceneEvaluationTreeNode&) const = default;

    friend FArchive& operator<<(FArchive& Ar, FMovieSceneEvaluationTreeNode& Node) {
        return Ar << Node.Range.LowerBound << Node.Range.UpperBound << Node.Parent.ChildrenHandle << Node.Parent.Index << Node.ChildrenID << Node.DataID;
    }
};

template <class ElementType>
struct TEvaluationTreeEntryContainer {
    struct FEntry {
        int32 StartIndex = 0;
        int32 Size = 0;
        int32 Capacity = 0;
        bool operator==(const FEntry&) const = default;

        friend FArchive& operator<<(FArchive& Ar, FEntry& Entry) { return Ar << Entry.StartIndex << Entry.Size << Entry.Capacity; }
    };

    TArray<FEntry> Entries;
    TArray<ElementType> Items;
    bool operator==(const TEvaluationTreeEntryContainer&) const = default;

    friend FArchive& operator<<(FArchive& Ar, TEvaluationTreeEntryContainer& Container) { return Ar << Container.Entries << Container.Items; }
};

template <class DataType>
struct TMovieSceneEvaluationTree {
    FMovieSceneEvaluationTreeNode RootNode;
    TEvaluationTreeEntryContainer<FMovieSceneEvaluationTreeNode> ChildNodes;
    TEvaluationTreeEntryContainer<DataType> Data;
    bool operator==(const TMovieSceneEvaluationTree&) const = default;

    friend FArchive& operator<<(FArchive& Ar, TMovieSceneEvaluationTree& Tree) { return Ar << Tree.RootNode << Tree.ChildNodes << Tree.Data; }
};

}
