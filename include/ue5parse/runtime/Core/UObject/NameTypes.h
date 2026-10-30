#pragma once

#include <string_view>

#include "ue5parse/core/FName.h"
#include "ue5parse/runtime/Core/Containers/UnrealString.h"

namespace ue5parse::runtime {

using ue5parse::FName;
using ue5parse::FNameHasher;

inline const FName NAME_None;

// FName(Text, FNAME_Add): a name whose text the runtime keeps for its whole life, so that it can be stored anywhere.
// Names from packages and the mappings already have stable text and need no interning.
FName MakeName(std::string_view Text);
inline FName MakeName(const char* Text) { return MakeName(std::string_view(Text)); }
FName MakeName(const FString& Text);
FString NameToString(FName Name);

}
