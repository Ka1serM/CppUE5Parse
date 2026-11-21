#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

struct FMovieSceneInlineValue {
    FString TypeName;
    FInstancedStruct Data;
    bool operator==(const FMovieSceneInlineValue&) const = default;

    bool Serialize(FArchive& Ar) {
        //! Without mappings a versioned cook can store the pointer as tagged properties.
        const UScriptStruct* EmptyStruct = FindNativeStruct(FName("MovieSceneEmptyStruct"));
        if (EmptyStruct == nullptr)
            return false;
        Data.Reset();
        Ar << TypeName;
        if (Ar.IsError() || TypeName.IsEmpty())
            return true;
        const std::string Path = TypeName.ToUtf8();
        const UScriptStruct* Struct = FindNativeStruct(FName(std::string_view(Path).substr(Path.rfind('.') + 1)));
        if (Struct == nullptr) {
            FInstancedStruct Empty;
            Empty.InitializeAs(EmptyStruct);
            EmptyStruct->SerializeItem(Ar, Empty.GetMutableMemory());
            return true;
        }
        Data.InitializeAs(Struct);
        Struct->SerializeItem(Ar, Data.GetMutableMemory());
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMovieSceneInlineValue> : TStructOpsTypeTraitsBase2<FMovieSceneInlineValue> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
