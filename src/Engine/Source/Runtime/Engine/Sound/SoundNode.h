#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "SoundNode.generated.h"

namespace ue5parse::runtime {

class UEdGraphNode;

UCLASS(Abstract)
class USoundNode : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<TObjectPtr<USoundNode>> ChildNodes;
    UPROPERTY()
    TObjectPtr<UEdGraphNode> GraphNode = nullptr;

    void Serialize(FArchive& Ar) override;
};

}
