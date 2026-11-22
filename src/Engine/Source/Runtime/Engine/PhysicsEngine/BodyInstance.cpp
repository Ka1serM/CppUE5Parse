#include "Engine/Source/Runtime/Engine/PhysicsEngine/BodyInstance.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"

namespace ue5parse::runtime {

//! Dune: Awakening and Conan Exiles Enhanced save the collision settings of the body binary, with the channel responses as names and bytes.
bool FBodyInstance::Serialize(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    if (Game != EGame::GAME_DuneAwakening && Game != EGame::GAME_ConanExilesEnhanced)
        return false;
    FName CollisionEnabledName;
    Ar << CollisionEnabledName;
    if (const UEnum* Enum = FindNativeEnum(FName("ECollisionEnabled"))) {
        const int64 Value = Enum->GetValueByName(CollisionEnabledName);
        if (Value != INDEX_NONE)
            CollisionEnabled = static_cast<ECollisionEnabled::Type>(Value);
    }
    int32 Count = 0;
    Ar << Count;
    if (Count < 0 || Count > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::InvalidData, "FBodyInstance.ResponseCount");
        return true;
    }
    TArray<FResponseChannel>& Responses = CollisionResponses.ResponseArray;
    Responses.SetNum(Count);
    for (FResponseChannel& Response : Responses)
        Ar << Response.Channel;
    for (FResponseChannel& Response : Responses)
        Ar << Response.Response;
    uint64 Flags = 0;
    Ar << Flags;
    if (Game == EGame::GAME_ConanExilesEnhanced) {
        Ar.SkipBytes(114);
        return true;
    }
    FVector3f SomeVector;
    Ar << SomeVector;
    Ar.SkipBytes(96);
    Ar << Scale3D;
    return true;
}

}
