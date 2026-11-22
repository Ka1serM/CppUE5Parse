#pragma once

#include "Engine/Source/Runtime/Engine/HLOD/HLODProxyMesh.h"

#include "HLODProxy.generated.h"

namespace ue5parse::runtime {

class UHLODProxyDesc;

UCLASS()
class UHLODProxy : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FHLODProxyMesh> ProxyMeshes;
    UPROPERTY()
    TMap<TObjectPtr<UHLODProxyDesc>, FHLODProxyMesh> HLODActors;
};

}
