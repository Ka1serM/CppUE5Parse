#include "Engine/Source/Runtime/AIModule/Navigation/NavLinkProxy.h"

#include "Engine/Source/Runtime/NavigationSystem/NavLinkCustomComponent.h"

namespace ue5parse::runtime {

ANavLinkProxy::ANavLinkProxy(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("PositionComponent"));
    PointLinks.AddDefaulted();
    bHidden = true;
    SmartLinkComp = ObjectInitializer.CreateDefaultSubobject<UNavLinkCustomComponent>(FName("SmartLinkComp"));
}

}
