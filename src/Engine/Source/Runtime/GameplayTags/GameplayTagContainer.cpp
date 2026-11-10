#include "Engine/Source/Runtime/GameplayTags/GameplayTagContainer.h"

namespace ue5parse::runtime {

namespace {

bool IsSameOrChildName(std::string_view Name, std::string_view Other) {
    if (Name.size() < Other.size() || Name.compare(0, Other.size(), Other) != 0)
        return false;
    return Name.size() == Other.size() || Name[Other.size()] == '.';
}

}

bool FGameplayTagContainer::Serialize(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(8);
    GameplayTags.Empty(Count);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        FGameplayTag Tag;
        Ar << Tag.TagName;
        GameplayTags.Add(Tag);
    }
    ParentTags.Empty();
    for (const FGameplayTag& Tag : GameplayTags) {
        std::string Name = NameToString(Tag.TagName).ToUtf8();
        for (std::size_t Dot = Name.rfind('.'); Dot != std::string::npos; Dot = Name.rfind('.', Dot - 1)) {
            FGameplayTag Parent;
            Parent.TagName = MakeName(std::string_view(Name).substr(0, Dot));
            ParentTags.AddUnique(Parent);
            if (Dot == 0)
                break;
        }
    }
    return true;
}

bool FGameplayTagContainer::operator==(const FGameplayTagContainer& Other) const {
    if (GameplayTags.Num() != Other.GameplayTags.Num())
        return false;
    for (const FGameplayTag& Tag : GameplayTags)
        if (!Other.HasTagExact(Tag))
            return false;
    return true;
}

}
