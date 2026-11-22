#pragma once

#include "GameTypes/OuterWorlds2/PropertyDataWithObjects.h"

#include "AIBehaviorTreeReference.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FAISubBehaviorTreeInstance {
    GENERATED_BODY()

    UPROPERTY()
    FName ObjectName;
    UPROPERTY()
    uint32 ObjectPersistentFlags = 0;
    UPROPERTY()
    FPropertyDataWithObjects PropertyDataWithObjects;

    friend FArchive& operator<<(FArchive& Ar, FAISubBehaviorTreeInstance& Instance) {
        Ar << Instance.ObjectName << Instance.ObjectPersistentFlags;
        Instance.PropertyDataWithObjects.Serialize(Ar);
        return Ar;
    }
};

USTRUCT()
struct FAIBehaviorTreeReference {
    GENERATED_BODY()

    UPROPERTY()
    uint8 Type = 0;
    UPROPERTY()
    FSoftObjectPath BehaviorTreeClass;
    UPROPERTY()
    TArray<FAISubBehaviorTreeInstance> SubBehaviorTreeInstances;
    UPROPERTY()
    FString SubTreeInstanceCommonPackageName;
    UPROPERTY()
    FString SubTreeInstanceCommonOuterPath;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FAIBehaviorTreeReference> : TStructOpsTypeTraitsBase2<FAIBehaviorTreeReference> {
    enum { WithSerializer = true };
};

//! CUE4Parse reads this selector of The Outer Worlds 2 as a soft object path.
USTRUCT()
struct FAIGroupBehaviorClassSelector {
    GENERATED_BODY()

    UPROPERTY()
    FSoftObjectPath ClassPath;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_OuterWorlds2)
            return false;
        Ar << ClassPath;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FAIGroupBehaviorClassSelector> : TStructOpsTypeTraitsBase2<FAIGroupBehaviorClassSelector> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FBlueprintFunctionLibraryConditional {
    GENERATED_BODY()

    UPROPERTY()
    FInstancedStruct FunctionReference;
    UPROPERTY()
    FInstancedStruct Parameters;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FBlueprintFunctionLibraryConditional> : TStructOpsTypeTraitsBase2<FBlueprintFunctionLibraryConditional> {
    enum { WithSerializer = true };
};

//! The Outer Worlds 2 saves this script like FBlueprintFunctionLibraryConditional, without the type byte.
USTRUCT()
struct FBlueprintDefinedScript {
    GENERATED_BODY()

    UPROPERTY()
    FInstancedStruct FunctionReference;
    UPROPERTY()
    FInstancedStruct Parameters;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FBlueprintDefinedScript> : TStructOpsTypeTraitsBase2<FBlueprintDefinedScript> {
    enum { WithSerializer = true };
};

}
