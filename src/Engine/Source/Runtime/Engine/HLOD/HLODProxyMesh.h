#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/LazyObjectPtr.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "HLODProxyMesh.generated.h"

namespace ue5parse::runtime {

class ALODActor;
class UStaticMesh;

USTRUCT()
struct FHLODProxyMesh {
    GENERATED_BODY()
    UPROPERTY()
    TLazyObjectPtr<ALODActor> LODActor;
    UPROPERTY()
    TObjectPtr<UStaticMesh> StaticMesh = nullptr;
    UPROPERTY()
    FName Key;
};

}
