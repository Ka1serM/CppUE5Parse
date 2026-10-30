#include "ue5parse/runtime/GameplayTags/GameplayTagContainer.h"

// UE's GameplayTagContainer.cpp: tag matching by the dotted hierarchy of the names, and the cooked container (its tag
// names; the parent tags are derived on load).

namespace ue5parse::runtime {

namespace {

// "A.B.C" is Other or below it when Other is "A.B.C", "A.B" or "A".
bool IsSameOrChildName(std::string_view Name, std::string_view Other) {
    if (Name.size() < Other.size() || Name.compare(0, Other.size(), Other) != 0) return false;
    return Name.size() == Other.size() || Name[Other.size()] == '.';
}

}

bool FGameplayTag::MatchesTag(const FGameplayTag& Other) const {
    if (!IsValid() || !Other.IsValid()) return false;
    if (TagName == Other.TagName) return true;
    // Names compare by text; FNames hold no number in tags.
    return IsSameOrChildName(NameToString(TagName).ToUtf8(), NameToString(Other.TagName).ToUtf8());
}

bool FGameplayTagContainer::HasTag(const FGameplayTag& Tag) const {
    if (!Tag.IsValid()) return false;
    for (const FGameplayTag& Own : GameplayTags)
        if (Own.MatchesTag(Tag)) return true;
    return false;
}

bool FGameplayTagContainer::HasAny(const FGameplayTagContainer& Other) const {
    for (const FGameplayTag& Tag : Other.GameplayTags)
        if (HasTag(Tag)) return true;
    return false;
}

bool FGameplayTagContainer::HasAll(const FGameplayTagContainer& Other) const {
    for (const FGameplayTag& Tag : Other.GameplayTags)
        if (!HasTag(Tag)) return false;
    return true;
}

bool FGameplayTagContainer::Serialize(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(8);
    GameplayTags.Empty(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        FGameplayTag Tag;
        Ar << Tag.TagName;
        GameplayTags.Add(Tag);
    }
    // FillParentTags: every ancestor of every tag.
    ParentTags.Empty();
    for (const FGameplayTag& Tag : GameplayTags) {
        std::string Name = NameToString(Tag.TagName).ToUtf8();
        for (std::size_t Dot = Name.rfind('.'); Dot != std::string::npos; Dot = Name.rfind('.', Dot - 1)) {
            FGameplayTag Parent;
            Parent.TagName = MakeName(std::string_view(Name).substr(0, Dot));
            ParentTags.AddUnique(Parent);
            if (Dot == 0) break;
        }
    }
    return true;
}

bool FGameplayTagContainer::operator==(const FGameplayTagContainer& Other) const {
    if (GameplayTags.Num() != Other.GameplayTags.Num()) return false;
    for (const FGameplayTag& Tag : GameplayTags)
        if (!Other.HasTagExact(Tag)) return false;
    return true;
}

}
