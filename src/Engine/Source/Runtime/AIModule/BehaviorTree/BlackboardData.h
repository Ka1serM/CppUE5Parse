#pragma once

#include "Engine/Source/Runtime/AIModule/BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "Engine/Source/Runtime/Engine/Engine/DataAsset.h"

#include "BlackboardData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FBlackboardEntry {
    GENERATED_BODY()
    UPROPERTY()
    FName EntryName;
    UPROPERTY()
    TObjectPtr<UBlackboardKeyType> KeyType = nullptr;
    UPROPERTY()
    uint32 bInstanceSynced : 1 = false;
};

UCLASS()
class UBlackboardData : public UDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UBlackboardData> Parent = nullptr;
    UPROPERTY()
    TArray<FBlackboardEntry> Keys;
    UPROPERTY()
    uint32 bHasSynchronizedKeys : 1 = false;
};

}
