#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/AI/Navigation/NavAgentSelector.h"

#include "NavLinkDefinition.generated.h"

namespace ue5parse::runtime {

class UNavAreaBase;

namespace ENavLinkDirection {

UENUM()
enum Type : int32 { BothWays, LeftToRight, RightToLeft };

}

USTRUCT()
struct FNavigationLinkBase {
    GENERATED_BODY()

    UPROPERTY()
    float LeftProjectHeight = 0.0f;
    UPROPERTY()
    float MaxFallDownLength = 1000.0f;
    UPROPERTY()
    TEnumAsByte<ENavLinkDirection::Type> Direction = ENavLinkDirection::BothWays;
    UPROPERTY()
    float SnapRadius = 30.0f;
    UPROPERTY()
    float SnapHeight = 50.0f;
    UPROPERTY()
    FNavAgentSelector SupportedAgents;
    UPROPERTY()
    uint32 bSupportsAgent0 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent1 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent2 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent3 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent4 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent5 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent6 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent7 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent8 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent9 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent10 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent11 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent12 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent13 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent14 : 1 = true;
    UPROPERTY()
    uint32 bSupportsAgent15 : 1 = true;
    UPROPERTY()
    FString Description;
    UPROPERTY()
    uint32 bUseSnapHeight : 1 = false;
    UPROPERTY()
    uint32 bSnapToCheapestArea : 1 = true;
    UPROPERTY()
    uint32 bCustomFlag0 : 1 = false;
    UPROPERTY()
    uint32 bCustomFlag1 : 1 = false;
    UPROPERTY()
    uint32 bCustomFlag2 : 1 = false;
    UPROPERTY()
    uint32 bCustomFlag3 : 1 = false;
    UPROPERTY()
    uint32 bCustomFlag4 : 1 = false;
    UPROPERTY()
    uint32 bCustomFlag5 : 1 = false;
    UPROPERTY()
    uint32 bCustomFlag6 : 1 = false;
    UPROPERTY()
    uint32 bCustomFlag7 : 1 = false;
    UPROPERTY()
    TSubclassOf<UNavAreaBase> AreaClass;
};

USTRUCT()
struct FNavigationLink : public FNavigationLinkBase {
    GENERATED_BODY()

    UPROPERTY()
    FVector Left = FVector(0.0, -50.0, 0.0);
    UPROPERTY()
    FVector Right = FVector(0.0, 50.0, 0.0);
};

USTRUCT()
struct FNavigationSegmentLink : public FNavigationLinkBase {
    GENERATED_BODY()

    UPROPERTY()
    FVector LeftStart = FVector(-25.0, -50.0, 0.0);
    UPROPERTY()
    FVector LeftEnd = FVector(25.0, -50.0, 0.0);
    UPROPERTY()
    FVector RightStart = FVector(-25.0, 50.0, 0.0);
    UPROPERTY()
    FVector RightEnd = FVector(25.0, 50.0, 0.0);
};

}
