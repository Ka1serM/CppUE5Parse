#include "Engine/Plugins/Runtime/WorldConditions/Source/WorldConditions/WorldConditionQuery.h"

namespace ue5parse::runtime {

bool FWorldConditionQueryDefinition::Serialize(FArchive& Ar) {
    StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(this));
    if (!Ar.CustomVerAtLeast(FWorldConditionCustomVersion::GUID, FWorldConditionCustomVersion::StructSharedDefinition, EGame::GAME_UE5_3))
        return true;
    Ar << bHasSharedDefinitionStruct;
    if (bHasSharedDefinitionStruct)
        FWorldConditionQuerySharedDefinition::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&SharedDefinitionStruct));
    return true;
}

}
