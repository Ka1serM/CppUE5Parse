#include "GameTypes/Nightingale/AffinityTable.h"

namespace ue5parse::runtime {

void UAffinityTable::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    Ar.SkipBytes(4);
    const int32 StructCount = Ar.ReadCount(4);
    Tables.Empty(StructCount);
    for (int32 StructIndex = 0; StructIndex < StructCount && !Ar.IsError(); StructIndex++) {
        FAffinityTableStructCells& Table = Tables[Tables.AddDefaulted()];
        Ar << Table.StructName;
        Ar.SkipBytes(4);
        const UObject* Struct = Structures.IsValidIndex(StructIndex) ? Structures[StructIndex].Get() : nullptr;
        const FName StructName = MakeName(Table.StructName);
        Table.Cells.Empty(RowTags.Num() * ColumnTags.Num());
        for (int32 Cell = 0; Cell < RowTags.Num() * ColumnTags.Num() && !Ar.IsError(); Cell++) {
            FInstancedStruct& Value = Table.Cells[Table.Cells.AddDefaulted()];
            if (Struct != nullptr)
                SerializeStructByObject(Ar, Struct, Value);
            else
                SerializeStructFallback(Ar, StructName, Value);
        }
    }
    Ar << Rows << Columns << Tags;
}

}
