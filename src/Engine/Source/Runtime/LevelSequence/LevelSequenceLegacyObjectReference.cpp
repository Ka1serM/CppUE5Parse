#include "Engine/Source/Runtime/LevelSequence/LevelSequenceLegacyObjectReference.h"

namespace ue5parse::runtime {

bool FLevelSequenceObjectReferenceMap::Serialize(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(16 + 4);
    Map.SetNum(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        Ar << Map[Index].first << Map[Index].second;
    return true;
}

bool FLevelSequenceObjectReferenceMap::operator==(const FLevelSequenceObjectReferenceMap& Other) const {
    if (Map.Num() != Other.Map.Num())
        return false;
    for (int32 Index = 0; Index < Map.Num(); Index++)
        if (Map[Index].first != Other.Map[Index].first || !(Map[Index].second == Other.Map[Index].second))
            return false;
    return true;
}

}
