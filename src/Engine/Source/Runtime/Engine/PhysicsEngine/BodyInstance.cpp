#include "Engine/Source/Runtime/Engine/PhysicsEngine/BodyInstance.h"

#include <iterator>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

namespace {

ECollisionEnabled::Type FindCollisionEnabled(FName Name) {
    static const FName Names[] = {FName("NoCollision"),     FName("QueryOnly"), FName("PhysicsOnly"),
                                  FName("QueryAndPhysics"), FName("ProbeOnly"), FName("QueryAndProbe")};
    for (int32 Index = 0; Index < static_cast<int32>(std::size(Names)); Index++)
        if (Names[Index] == Name)
            return static_cast<ECollisionEnabled::Type>(Index);
    return ECollisionEnabled::QueryAndPhysics;
}

}

bool FBodyInstance::Serialize(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    //! Dune: Awakening and Conan Exiles Enhanced save the body instance in a custom native format.
    if (Game != EGame::GAME_DuneAwakening && Game != EGame::GAME_ConanExilesEnhanced)
        return false;
    FName CollisionEnabledName;
    Ar << CollisionEnabledName;
    CollisionEnabled = FindCollisionEnabled(CollisionEnabledName);
    const int32 Count = Ar.ReadCount(9);
    TArray<FResponseChannel>& Responses = CollisionResponses.ResponseArray;
    Responses.SetNum(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        Ar << Responses[Index].Channel;
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++) {
        uint8 Response = 0;
        Ar << Response;
        Responses[Index].Response = static_cast<ECollisionResponse>(Response);
    }
    uint64 Flags = 0;
    Ar << Flags;
    if (Game == EGame::GAME_ConanExilesEnhanced) {
        Ar.SkipBytes(114);
        return true;
    }
    Ar.SkipBytes(12 + 96);
    Ar << Scale3D;
    return true;
}

}
