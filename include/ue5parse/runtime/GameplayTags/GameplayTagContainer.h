#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"

#include "GameplayTagContainer.generated.h"

// Gameplay tags (UE's GameplayTags/Classes/GameplayTagContainer.h).

namespace ue5parse::runtime {

// FGameplayTag: a hierarchical name ("A.B.C"). The container serializes natively: its tag names.
USTRUCT()
struct FGameplayTag {
    GENERATED_BODY()
    UPROPERTY()
    FName TagName;
    bool IsValid() const { return !TagName.isNone(); }
    bool operator==(const FGameplayTag& Other) const { return TagName == Other.TagName; }
    // "A.B" matches "A.B" and "A.B.C" asked for "A.B" (UE's MatchesTag): this tag is Other or a child of it.
    bool MatchesTag(const FGameplayTag& Other) const;
};

USTRUCT()
struct FGameplayTagContainer {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FGameplayTag> GameplayTags;
    UPROPERTY()
    TArray<FGameplayTag> ParentTags;

    int32 Num() const { return GameplayTags.Num(); }
    bool HasTag(const FGameplayTag& Tag) const;
    bool HasTagExact(const FGameplayTag& Tag) const { return GameplayTags.Contains(Tag); }
    bool HasAny(const FGameplayTagContainer& Other) const;
    bool HasAll(const FGameplayTagContainer& Other) const;
    void AddTag(const FGameplayTag& Tag) { GameplayTags.AddUnique(Tag); }
    bool RemoveTag(const FGameplayTag& Tag) { return GameplayTags.Remove(Tag) > 0; }
    // The cooked value: the tag names (UE's WithStructuredSerializer).
    bool Serialize(FArchive& Ar);
    bool operator==(const FGameplayTagContainer& Other) const;
};

template <>
struct TStructOpsTypeTraits<FGameplayTagContainer> : TStructOpsTypeTraitsBase2<FGameplayTagContainer> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
