#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

namespace ue5parse::runtime {

bool FActorInstanceHandle::DoesRepresentClass(const UClass* OtherClass) const {
    AActor* Resolved = Actor.Get();
    return OtherClass != nullptr && Resolved != nullptr && Resolved->IsA(OtherClass);
}

}
