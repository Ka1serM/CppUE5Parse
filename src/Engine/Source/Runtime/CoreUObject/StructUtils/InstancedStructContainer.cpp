#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStructContainer.h"

namespace ue5parse::runtime {

bool FInstancedStructContainer::Serialize(FArchive& Ar) {
    uint8 Version = 0;
    Ar << Version;
    if (Version != 0) {
        Ar.SetError(ErrorCode::InvalidData, "FInstancedStructContainer.Version");
        return true;
    }
    const int32 NumItems = Ar.ReadCount(4);
    Items.Empty(NumItems);
    for (int32 Index = 0; Index < NumItems && !Ar.IsError(); Index++) {
        UObject* StructObject = nullptr;
        Ar << StructObject;
        Items[Items.AddDefaulted()].InitializeAs(Cast<UScriptStruct>(StructObject));
    }
    for (FInstancedStruct& Item : Items) {
        int32 SerialSize = 0;
        Ar << SerialSize;
        if (Ar.IsError())
            return true;
        SerializeStructInstance(Ar, Item.GetScriptStruct(), Item.GetMutableMemory(), SerialSize);
    }
    return true;
}

}
