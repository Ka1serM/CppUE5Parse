#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionActor.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionComponent.h"
#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionDebugDrawComponent.h"

namespace ue5parse::runtime {

AGeometryCollectionActor::AGeometryCollectionActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GeometryCollectionComponent = ObjectInitializer.CreateDefaultSubobject<UGeometryCollectionComponent>(FName("GeometryCollectionComponent0"));
    RootComponent = GeometryCollectionComponent;
    const EGame Game = GetRuntimeProvider().getGame();
    if (Game >= EGame::GAME_UE4_24 && Game < EGame::GAME_UE5_0)
        GeometryCollectionDebugDrawComponent = ObjectInitializer.CreateDefaultSubobject<UGeometryCollectionDebugDrawComponent>(FName("GeometryCollectionDrawComponent0"));
}

}
