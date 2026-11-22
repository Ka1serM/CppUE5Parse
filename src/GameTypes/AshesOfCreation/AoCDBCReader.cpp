#include "GameTypes/AshesOfCreation/AoCDBCReader.h"

#include <algorithm>
#include <string>

#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

constexpr std::array<std::string_view, 6> RecordTypes = {"CertificationLevel", "GameMusicOverride", "RuntimeFoliageType", "RuntimeFoliageSpawnDef", "TestData", "Toast"};

constexpr std::array<std::string_view, 16> PlainTypes = {"DestinySystem",       "EquipableComponent",  "FishingSizeData",   "InteractableStateTreeDef",
                                                         "InteractionStateDef", "DialogueTable",       "DialogueNode",      "DialogueRoot",
                                                         "GlobalKillVolumeList", "KillVolumeDef",      "WarPhaseRecordV2",  "WarRecordV2",
                                                         "NCSRemapCache",       "NarrativeQuestV2",    "NarrativeObjectiveV2", "ParticipationNameRegistry"};

constexpr std::array<std::string_view, 4> Aligned16Types = {"CraftingStationVisualDefRecordBase", "PathingRouteRecordBase", "StoryArcPhaseRecordBase",
                                                            "SurveyPylonVisualDefRecordBase"};

//! The last two properties of a record are not in the cache database.
constexpr int32 SkippedRecordProperties = 2;

std::string GetRecordStructName(std::string_view Category) {
    std::string Name(Category);
    if (std::ranges::find(RecordTypes, Category) != RecordTypes.end())
        return Name + "Record";
    if (std::ranges::find(PlainTypes, Category) != PlainTypes.end() || Category.ends_with("Record"))
        return Name;
    return Name + "RecordBase";
}

int64 GetInt64Value(const UStruct* Struct, const uint8* Data, FName Name) {
    const auto* Property = CastField<FNumericProperty>(Struct->FindPropertyByName(Name));
    if (Property == nullptr || !Property->IsInteger())
        return 0;
    return Property->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Data));
}

FName GetNameValue(const UStruct* Struct, const uint8* Data, FName Name) {
    const auto* Property = CastField<FNameProperty>(Struct->FindPropertyByName(Name));
    if (Property == nullptr)
        return FName();
    return *Property->ContainerPtrToValuePtr<FName>(Data);
}

int64 AlignUp(int64 Value, int64 Alignment) {
    return (Value + Alignment - 1) / Alignment * Alignment;
}

}

FAoCDBCReader::FAoCDBCReader(std::span<const std::byte> InBytes) : FMemoryReaderView(InBytes) {
    SetGame(EGame::GAME_AshesOfCreation);
    SetUseUnversionedPropertySerialization(false);
    SetUseRawPropertySerialization(true);
    Read<uint32>();
    Read<uint64>();
    SkipBytes(8);
    const int32 ChunkCount = ReadCount(32);
    SkipBytes(16);
    const int64 StringsOffset = Read<int64>();
    Read<int32>();
    Read<uint32>();
    Read<int32>();
    for (int32 Index = 0; Index < ChunkCount && !IsError(); Index++) {
        FAoCDataChunk& Chunk = Chunks[Chunks.AddDefaulted()];
        *this << Chunk.Hash << Chunk.Offset << Chunk.Size << Chunk.FileCount << Chunk.Checksum;
    }
    if (IsError())
        return;
    Seek(StringsOffset);
    while (!IsError() && Tell() < TotalSize()) {
        const auto Offset = static_cast<int32>(Tell() - StringsOffset);
        FString Text;
        *this << Text;
        NameMap[Offset] = MakeName(Text);
    }
}

FArchive& FAoCDBCReader::operator<<(FName& Value) {
    const int32 Offset = Read<int32>();
    const auto Found = NameMap.find(Offset);
    Value = Found != NameMap.end() ? Found->second : FName();
    return *this;
}

FArchive& FAoCDBCReader::operator<<(UObject*& Value) {
    Read<int32>();
    Value = nullptr;
    return *this;
}

FArchive& FAoCDBCReader::operator<<(FSoftObjectPath& Value) {
    FName Name;
    *this << Name;
    const std::string_view Text = Name.getText();
    const std::size_t Colon = Text.rfind(':');
    Value.Reset();
    Value.SetPath(FString(Text.substr(0, Colon)));
    if (Colon != std::string_view::npos && Colon > 0)
        Value.SubPathString = FString(Text.substr(Colon + 1));
    return *this;
}

bool FAoCDBCReader::TryReadChunk(int32 Index, FString& Category, TArray<FAoCFile>& Files) {
    Files.Empty();
    ClearError();
    if (!Chunks.IsValidIndex(Index))
        return false;
    const FAoCDataChunk& Chunk = Chunks[Index];
    const auto* Type = std::ranges::lower_bound(AoCChunkTypes, Chunk.Hash, {}, &FAoCChunkType::Hash);
    if (Type == AoCChunkTypes.end() || Type->Hash != Chunk.Hash)
        return false;
    Category = FString(Type->Name);
    const FString StructName(GetRecordStructName(Type->Name));
    Seek(Chunk.Offset);
    for (int32 File = 0; File < Chunk.FileCount && !IsError(); File++)
        if (!ReadFile(StructName, Files[Files.AddDefaulted()]))
            return false;
    return !IsError();
}

bool FAoCDBCReader::ReadFile(const FString& Type, FAoCFile& File) {
    const std::string TypeName = Type.ToUtf8();
    int64 Position = AlignUp(Tell(), 8);
    if (std::ranges::find(Aligned16Types, TypeName) != Aligned16Types.end())
        Position = AlignUp(Position, 16);
    Seek(Position);
    const UScriptStruct* Struct = FindNativeStruct(MakeName(Type));
    if (Struct == nullptr) {
        SetError(ErrorCode::MissingSchema, "FAoCDBCReader.Record");
        return false;
    }
    File.Properties.InitializeAs(Struct);
    uint8* Data = File.Properties.GetMutableMemory();
    Struct->SerializeRawProperties(*this, Data, SkippedRecordProperties);
    if (IsError())
        return false;
    if (GetInt64Value(Struct, Data, FName("ParentGuid")) != 0)
        SkipBytes(TypeName == "ContributionThresholdsRecordBase" ? 2 : 4);
    const std::string Guid = std::to_string(GetInt64Value(Struct, Data, FName("Guid")));
    std::string Name(GetNameValue(Struct, Data, FName("Name")).getText());
    if (!Name.ends_with(Guid))
        Name += "_" + Guid;
    File.Name = FString(Name);
    return !IsError();
}

}
