#include "GameTypes/Palia/VAL_PremiumItemAsset.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"

namespace ue5parse::runtime {

void UVAL_PremiumItemAsset::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetRemaining() <= 0)
        return;
    const FString ExportType = GetClass()->GetName();
    const bool bToolSkin = ExportType.StartsWith(FString("VAL_CharacterCustomizationItem_"), ESearchCase::CaseSensitive) &&
                           ExportType.EndsWith(FString("Skin"), ESearchCase::CaseSensitive);
    const FName StructName = bToolSkin ? FName("VAL_CharacterCustomizationItem_ToolSkin_SkeletalMesh_PerBodyTypeConfig")
                                       : MakeName(ExportType + FString("_PerBodyTypeConfig"));
    const int32 Count = Ar.ReadCount(1);
    PerBodyTypeConfig.Empty(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++) {
        FPerBodyTypeConfig& Config = PerBodyTypeConfig[PerBodyTypeConfig.AddDefaulted()];
        Ar << Config.ItemType;
        SerializeStructFallback(Ar, StructName, Config.Config);
    }
}

bool FVAL_CharacterCustomizationVariantOptionsArray::Serialize(FArchive& Ar) {
    FPackageIndex StructIndex;
    Ar << StructIndex;
    FLinkerLoad* Linker = Ar.GetLinker();
    if (Ar.IsError() || StructIndex.IsNull() || Linker == nullptr)
        return true;
    OptionStruct = Linker->IndexToObject(StructIndex);
    const int32 Count = Ar.ReadCount(1);
    Options.Empty(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        SerializeStructByObject(Ar, StructIndex, Options[Options.AddDefaulted()]);
    return true;
}

}
