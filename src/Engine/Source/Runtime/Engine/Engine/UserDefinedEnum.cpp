#include "Engine/Source/Runtime/Engine/Engine/UserDefinedEnum.h"

namespace ue5parse::runtime {

FText UUserDefinedEnum::GetDisplayNameTextByIndex(int32 InIndex) const {
    const std::string EnumEntryName = GetNameStringByIndex(InIndex).ToUtf8();
    if (const FText* EnumEntryDisplayName = DisplayNameMap.Find(FName(EnumEntryName)))
        return *EnumEntryDisplayName;
    if (DisplayNames.IsValidIndex(InIndex))
        return DisplayNames[InIndex];
    return Super::GetDisplayNameTextByIndex(InIndex);
}

}
