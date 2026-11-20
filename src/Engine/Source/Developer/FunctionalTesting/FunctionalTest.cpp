#include "Engine/Source/Developer/FunctionalTesting/FunctionalTest.h"

#include "Engine/Source/Runtime/Engine/Components/BillboardComponent.h"

namespace ue5parse::runtime {

AFunctionalTest::AFunctionalTest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SpriteComponent = ObjectInitializer.CreateDefaultSubobject<UBillboardComponent>(FName("Sprite"));
    SpriteComponent->bHiddenInGame = true;
    RootComponent = SpriteComponent;
}

}
