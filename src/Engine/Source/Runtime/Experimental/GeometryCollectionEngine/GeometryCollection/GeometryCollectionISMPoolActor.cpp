#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionISMPoolActor.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionISMPoolComponent.h"
#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/GeometryCollectionISMPoolDebugDrawComponent.h"

namespace ue5parse::runtime {

AGeometryCollectionISMPoolActor::AGeometryCollectionISMPoolActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ISMPoolComp = ObjectInitializer.CreateDefaultSubobject<UGeometryCollectionISMPoolComponent>(FName("ISMPoolComp"));
    RootComponent = ISMPoolComp;
    if (GetRuntimeProvider().getGame() < EGame::GAME_UE5_3)
        return;
    ISMPoolDebugDrawComp = ObjectInitializer.CreateDefaultSubobject<UGeometryCollectionISMPoolDebugDrawComponent>(FName("ISMPoolDebug"));
    ISMPoolDebugDrawComp->SetupAttachment(ISMPoolComp);
}

}
