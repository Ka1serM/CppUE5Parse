#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

namespace ue5parse::runtime {

// UE's FActorInstanceHandle::DoesRepresentClass (Engine/ActorInstanceHandle.cpp). UE asks the instance's manager for
// the class of an instance without an actor; the runtime has no such managers.
bool FActorInstanceHandle::DoesRepresentClass(const UClass* OtherClass) const {
    AActor* Resolved = Actor.Get();
    return OtherClass != nullptr && Resolved != nullptr && Resolved->IsA(OtherClass);
}

}
