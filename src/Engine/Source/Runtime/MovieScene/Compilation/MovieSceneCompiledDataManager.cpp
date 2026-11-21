#include "Engine/Source/Runtime/MovieScene/Compilation/MovieSceneCompiledDataManager.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

#include <atomic>

namespace ue5parse::runtime {

bool FMovieSceneDeterminismFence::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("StructProperty") || Tag.TypeName.GetParameterName(0) != FName("FrameTime"))
        return false;
    static constinit std::atomic<UScriptStruct*> Cache = nullptr;
    UScriptStruct* FrameTimeStruct = Cache.load(std::memory_order_acquire);
    if (FrameTimeStruct == nullptr)
        FrameTimeStruct = FindNativeStruct(Cache, "FrameTime");
    FFrameTime FrameTime;
    FrameTimeStruct->SerializeItem(Ar, &FrameTime);
    FrameNumber = FrameTime.FrameNumber;
    return true;
}

//! A versioned 5.6 cook saves the flags as names joined by " | ".
bool FMovieSceneSequenceCompilerMaskStruct::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("EnumProperty"))
        return false;
    FName Value;
    Ar << Value;
    if (Value.isNone())
        return true;
    const std::string Text(Value.toString());
    std::string_view Rest(Text);
    while (!Rest.empty()) {
        const std::size_t Separator = Rest.find(" | ");
        std::string_view Entry = Rest.substr(0, Separator);
        Rest = Separator == std::string_view::npos ? std::string_view() : Rest.substr(Separator + 3);
        if (const std::size_t Scope = Entry.rfind("::"); Scope != std::string_view::npos)
            Entry = Entry.substr(Scope + 2);
        if (Entry == "Hierarchy")
            bHierarchy = true;
        else if (Entry == "EvaluationTemplate")
            bEvaluationTemplate = true;
        else if (Entry == "EvaluationTemplateField")
            bEvaluationTemplateField = true;
        else if (Entry == "EntityComponentField")
            bEntityComponentField = true;
        else
            return false;
    }
    return true;
}

}
