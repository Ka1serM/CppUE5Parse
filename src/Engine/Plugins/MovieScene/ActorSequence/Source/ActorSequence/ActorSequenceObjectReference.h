#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ActorSequenceObjectReference.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EActorSequenceObjectReferenceType : uint8 { ContextActor, ExternalActor, Component };

USTRUCT()
struct FActorSequenceObjectReference {
    GENERATED_BODY()

    UPROPERTY()
    EActorSequenceObjectReferenceType Type = EActorSequenceObjectReferenceType::ContextActor;
    UPROPERTY()
    FGuid ActorId;
    UPROPERTY()
    FString PathToComponent;
};

USTRUCT()
struct FActorSequenceObjectReferences {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FActorSequenceObjectReference> Array;
};

USTRUCT()
struct FActorSequenceObjectReferenceMap {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FGuid> BindingIds;
    UPROPERTY()
    TArray<FActorSequenceObjectReferences> References;
};

}
