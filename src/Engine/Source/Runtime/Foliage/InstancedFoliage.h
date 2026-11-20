#pragma once

#include <variant>

#include "Engine/Source/Runtime/Engine/Components/HierarchicalInstancedStaticMeshComponent.h"

namespace ue5parse::runtime {

class AActor;

enum class EFoliageImplType : uint8 {
    Unknown = 0,
    StaticMesh = 1,
    Actor = 2,
    ISMActor = 3
};

struct FFoliageStaticMesh {
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Component = nullptr;
};

struct FFoliageActor {
    TArray<TObjectPtr<AActor>> ActorInstances;
    TObjectPtr<UClass> ActorClass = nullptr;
};

struct FFoliageInfo {
    EFoliageImplType Type = EFoliageImplType::Unknown;
    std::variant<std::monostate, FFoliageStaticMesh, FFoliageActor> Implementation;

    friend FArchive& operator<<(FArchive& Ar, FFoliageInfo& Info);
};

}
