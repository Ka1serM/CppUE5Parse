#pragma once

#include "Engine/Source/Runtime/Engine/Engine/Blueprint.h"

#include "AnimBlueprint.generated.h"

namespace ue5parse::runtime {

class USkeleton;

USTRUCT()
struct FAnimGroupInfo {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FLinearColor Color = FLinearColor::White;
};

UCLASS()
class UAnimBlueprint : public UBlueprint {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USkeleton> TargetSkeleton = nullptr;
    UPROPERTY()
    TArray<FAnimGroupInfo> Groups;
};

}
