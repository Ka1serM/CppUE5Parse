#include "GameTypes/OuterWorlds2/AIBehaviorTreeReference.h"

namespace ue5parse::runtime {

namespace {

void SerializeConditional(FArchive& Ar, bool bHasType, FInstancedStruct& FunctionReference, FInstancedStruct& Parameters) {
    if (bHasType)
        Ar.SkipBytes(1);
    Ar.SkipBytes(1);
    SerializeStructFallback(Ar, FName("MemberReference"), FunctionReference);
    if (!Ar.IsError())
        SerializeOW2PropertyTags(Ar, Parameters);
}

}

bool FAIBehaviorTreeReference::Serialize(FArchive& Ar) {
    if (Ar.GetGame() != EGame::GAME_OuterWorlds2)
        return false;
    Ar << Type << BehaviorTreeClass;
    const int32 Count = Ar.ReadCount(1);
    Ar << SubTreeInstanceCommonPackageName << SubTreeInstanceCommonOuterPath;
    SubBehaviorTreeInstances.Empty(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        Ar << SubBehaviorTreeInstances[SubBehaviorTreeInstances.AddDefaulted()];
    return true;
}

bool FBlueprintFunctionLibraryConditional::Serialize(FArchive& Ar) {
    if (Ar.GetGame() != EGame::GAME_OuterWorlds2)
        return false;
    SerializeConditional(Ar, true, FunctionReference, Parameters);
    return true;
}

bool FBlueprintDefinedScript::Serialize(FArchive& Ar) {
    if (Ar.GetGame() != EGame::GAME_OuterWorlds2)
        return false;
    SerializeConditional(Ar, false, FunctionReference, Parameters);
    return true;
}

}
