#include "GameTypes/Psychonauts2/P2Attribute.h"

#include <cstring>

namespace ue5parse::runtime {

bool FLinecode::Serialize(FArchive& Ar) {
    if (Ar.GetGame() != EGame::GAME_Psychonauts2)
        return false;
    char Bytes[13] = {};
    Ar.Serialize(Bytes, sizeof(Bytes));
    Value = FString(std::string_view(Bytes, strnlen(Bytes, sizeof(Bytes))));
    return true;
}

bool FP2Attribute::Serialize(FArchive& Ar) {
    if (Ar.GetGame() != EGame::GAME_Psychonauts2)
        return false;
    Ar << Type;
    switch (Type) {
    case EAttribute::Invalid:
        return true;
    case EAttribute::Enum:
    case EAttribute::Name:
        Ar << NameValue;
        return true;
    case EAttribute::Integer:
        Ar << IntValue;
        return true;
    case EAttribute::Float:
        Ar << FloatValue;
        return true;
    case EAttribute::Boolean:
        Ar << BoolValue;
        return true;
    case EAttribute::String:
        Ar << StringValue;
        return true;
    case EAttribute::Text:
        Ar << TextValue;
        return true;
    case EAttribute::Object:
    case EAttribute::Class:
        Ar << ObjectValue;
        return true;
    case EAttribute::Vector:
        Ar << VectorValue;
        return true;
    case EAttribute::Quat:
        Ar << QuatValue;
        return true;
    case EAttribute::Color:
        Ar << ColorValue;
        return true;
    default:
        Ar.SetError(ErrorCode::InvalidData, "FP2Attribute.Type");
        return true;
    }
}

}
