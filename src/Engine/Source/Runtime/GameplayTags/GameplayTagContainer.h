#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "GameplayTagContainer.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FGameplayTag {
    GENERATED_BODY()
    UPROPERTY()
    FName TagName;
    bool IsValid() const { return !TagName.isNone(); }
    bool operator==(const FGameplayTag& Other) const { return TagName == Other.TagName; }
    friend uint32 GetTypeHash(const FGameplayTag& Tag) { return GetTypeHash(Tag.TagName); }
};

USTRUCT()
struct FGameplayTagContainer {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FGameplayTag> GameplayTags;
    UPROPERTY(Transient)
    TArray<FGameplayTag> ParentTags;

    int32 Num() const { return GameplayTags.Num(); }
    bool HasTagExact(const FGameplayTag& Tag) const { return GameplayTags.Contains(Tag); }
    bool Serialize(FArchive& Ar);
    bool operator==(const FGameplayTagContainer& Other) const;
};

template <>
struct TStructOpsTypeTraits<FGameplayTagContainer> : TStructOpsTypeTraitsBase2<FGameplayTagContainer> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

USTRUCT()
struct FGameplayTagQuery {
    GENERATED_BODY()
    UPROPERTY()
    int32 TokenStreamVersion = 0;
    UPROPERTY()
    TArray<FGameplayTag> TagDictionary;
    UPROPERTY()
    TArray<uint8> QueryTokenStream;
    UPROPERTY()
    FString UserDescription;
    UPROPERTY()
    FString AutoDescription;
};

}
