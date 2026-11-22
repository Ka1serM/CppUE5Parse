#include "GameTypes/FinalFantasy7Rebirth/EndStreamableAssetPump.h"

namespace ue5parse::runtime {

namespace {

struct FAssetPumpEntry {
    uint16 Index = 0;
    uint16 Type = 0;
};

struct FAssetPumpSection {
    TArray<FAssetPumpEntry> Entries;
    int32 Index = 0;
};

}

void UEndStreamableAssetPump::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FMemoryMappedImageReader Reader(Image);
    TArray<FAssetPumpSection> Sections = Reader.ReadArray<FAssetPumpSection>([&Reader] {
        FAssetPumpSection Section;
        Section.Entries = Reader.ReadRawArray<FAssetPumpEntry>();
        Reader.ReadArray<int32>([&Reader] {
            Reader.ReadArray<TArray<uint8>>([&Reader] { return Reader.ReadRawArray<uint8>(); }, 1);
            return Reader.Read<int32>();
        });
        Section.Index = Reader.Read<int32>();
        return Section;
    });
    TArray<FString> Assets = Reader.ReadArray<FString>([&Reader] { return Reader.ReadString(); });
    if (Reader.IsError()) {
        Ar.SetError(ErrorCode::InvalidData, "UEndStreamableAssetPump.Sections");
        return;
    }
    for (const FAssetPumpSection& Section : Sections) {
        FEndStreamableAssetPumpSection& Mapped = AssetMap.FindOrAdd(Section.Index);
        for (const FAssetPumpEntry& Entry : Section.Entries) {
            if (Entry.Index >= Assets.Num()) {
                Ar.SetError(ErrorCode::InvalidData, "UEndStreamableAssetPump.AssetIndex");
                return;
            }
            Mapped.Assets.FindOrAdd(Assets[Entry.Index]) = Entry.Type;
        }
    }
}

}
