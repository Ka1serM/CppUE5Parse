#include "GameTypes/MortalKombat1/MK1Structs.h"

#include <string_view>

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FMK1ParameterValue& Value) {
    Ar << Value.Type;
    const std::string_view Type = Value.Type.getText();
    if (Type == "bool")
        Ar << Value.BoolValue;
    else if (Type == "float")
        Ar << Value.FloatValue;
    else if (Type == "int32")
        Ar << Value.IntValue;
    else if (Type == "object")
        Ar << Value.ObjectValue;
    else if (Type == "FVector")
        Ar << Value.VectorValue;
    else if (Type == "FName")
        Ar << Value.NameValue;
    return Ar;
}

void UMKDialogueTable::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetGame() != EGame::GAME_MortalKombat1)
        return;
    EmptyTable();
    const int32 RowCount = Ar.ReadCount(8);
    const bool bUnversioned = Ar.UseUnversionedPropertySerialization();
    Ar.SetUseUnversionedPropertySerialization(false);
    for (int32 Index = 0; Index < RowCount && !Ar.IsError(); Index++) {
        FName RowName;
        Ar << RowName;
        LoadRow(Ar, RowName);
    }
    Ar.SetUseUnversionedPropertySerialization(bUnversioned);
}

}
