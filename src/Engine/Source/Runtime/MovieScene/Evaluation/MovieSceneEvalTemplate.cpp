#include "Engine/Source/Runtime/MovieScene/Evaluation/MovieSceneEvalTemplate.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

//! Like UE, a type that is not found is read as an empty struct, so its tagged properties are skipped.
bool SerializeMovieSceneInlineValue(FArchive& Ar, FInstancedStruct& Value) {
    FString TypeName;
    Ar << TypeName;
    Value.Reset();
    if (TypeName.IsEmpty() || Ar.IsError())
        return true;
    const std::string Path = TypeName.ToUtf8();
    const UScriptStruct* Struct = FindNativeStruct(MakeName(std::string_view(Path).substr(Path.rfind('.') + 1)));
    if (Struct == nullptr)
        ReportLoadProblem("load.unknown_type", TypeName);
    Value.InitializeAs(Struct != nullptr ? Struct : FMovieSceneEmptyStruct::StaticStruct());
    Value.GetScriptStruct()->SerializeItem(Ar, Value.GetMutableMemory());
    return true;
}

}
