#include "Engine/Plugins/PCG/Source/PCG/Data/Registry/PCGDataTypeIdentifier.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

namespace ue5parse::runtime {

bool FPCGDataTypeIdentifier::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("EnumProperty"))
        return false;
    FName EnumValueName;
    Ar << EnumValueName;
    if (EnumValueName.isNone())
        return true;
    const UEnum* Enum = StaticEnum<EPCGDataType>();
    uint32 Flags = 0;
    std::string Names = EnumValueName.toString();
    for (std::size_t Begin = 0; Begin <= Names.size();) {
        std::size_t End = std::min(Names.find('|', Begin), Names.size());
        std::string_view Token = std::string_view(Names).substr(Begin, End - Begin);
        while (!Token.empty() && Token.front() == ' ')
            Token.remove_prefix(1);
        while (!Token.empty() && Token.back() == ' ')
            Token.remove_suffix(1);
        if (int64 Value = Enum->GetValueByName(MakeName(Token)); Value != INDEX_NONE)
            Flags |= static_cast<uint32>(Value);
        Begin = End + 1;
    }
    LegacyDataType = static_cast<EPCGDataType>(Flags);
    return true;
}

}
