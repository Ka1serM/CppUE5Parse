#pragma once

#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class UStruct;

struct FCoreRedirects {
    static FName GetRedirectedPropertyName(const UStruct* Struct, FName OldName);
    static FName GetRedirectedClassName(FName OldName);
    static FName GetRedirectedStructName(FName OldName);
    static FName GetRedirectedEnumValueName(FName EnumName, FName OldValueName);
};

}
