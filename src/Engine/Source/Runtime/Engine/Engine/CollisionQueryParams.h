#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class AActor;

struct FCollisionQueryParams {
    FName TraceTag;
    bool bTraceComplex = false;
    TArray<const AActor*> IgnoredActors;

    FCollisionQueryParams() = default;
    FCollisionQueryParams(const FName InTraceTag, const bool bInTraceComplex) : TraceTag(InTraceTag), bTraceComplex(bInTraceComplex) {}

    void AddIgnoredActor(const AActor* Actor) { IgnoredActors.AddUnique(Actor); }
    void AddIgnoredActors(const TArray<AActor*>& Actors) {
        for (const AActor* Actor : Actors)
            AddIgnoredActor(Actor);
    }
};

}
