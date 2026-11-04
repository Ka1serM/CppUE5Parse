#pragma once

#include <string_view>

#include "CppUE5Parse/Core/FName.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"

namespace ue5parse::runtime {

using ue5parse::FName;
using ue5parse::FNameHasher;

inline const FName NAME_None;

FName MakeName(std::string_view Text);
inline FName MakeName(const char* Text) { return MakeName(std::string_view(Text)); }
FName MakeName(const FString& Text);
FString NameToString(FName Name);

}
