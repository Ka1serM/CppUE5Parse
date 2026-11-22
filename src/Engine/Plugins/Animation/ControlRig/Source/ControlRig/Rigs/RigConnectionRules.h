#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "RigConnectionRules.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FRigConnectionRuleStash {
    GENERATED_BODY()

    void Load(FArchive& Ar);

    UPROPERTY()
    FString ScriptStructPath;
    UPROPERTY()
    FString ExportedText;
};

}
