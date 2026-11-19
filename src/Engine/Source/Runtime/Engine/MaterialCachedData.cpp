#include "Engine/Source/Runtime/Engine/MaterialCachedData.h"

#include "CppUE5Parse/Provider/FileProvider.h"

namespace ue5parse::runtime {

namespace {

bool HasNestedParameters(EGame Game) {
    return Game < EGame::GAME_UE5_1;
}

bool HasParameterInfoSet(EGame Game) {
    return Game >= EGame::GAME_UE5_0;
}

bool HasRuntimeEntry(EMaterialParameterType Type, EGame Game) {
    switch (Type) {
    case EMaterialParameterType::DoubleVector:
        return Game >= EGame::GAME_UE5_0;
    case EMaterialParameterType::TextureCollection:
        return Game >= EGame::GAME_UE5_5;
    case EMaterialParameterType::SparseVolumeTexture:
    case EMaterialParameterType::StaticSwitch:
        return Game >= EGame::GAME_UE5_2;
    case EMaterialParameterType::ParameterCollection:
        return Game >= EGame::GAME_UE5_7;
    case EMaterialParameterType::ToonProfile:
        return Game >= EGame::GAME_UE5_9;
    default:
        return Type < EMaterialParameterType::NumRuntime;
    }
}

int32 GetRuntimeEntryIndex(EMaterialParameterType Type, EGame Game) {
    if (!HasRuntimeEntry(Type, Game))
        return INDEX_NONE;
    int32 Index = 0;
    for (uint8 Earlier = 0; Earlier < static_cast<uint8>(Type); Earlier++)
        if (HasRuntimeEntry(static_cast<EMaterialParameterType>(Earlier), Game))
            Index++;
    return Index;
}

}

int32 FMaterialCachedParameterEntry::FindParameterIndex(const FMaterialParameterInfo& ParameterInfo) const {
    if (HasParameterInfoSet(GetRuntimeProvider().getGame()))
        return ParameterInfoSet.FindId(ParameterInfo);
    return ParameterInfos.Find(ParameterInfo);
}

void FMaterialCachedParameterEntry::GetParameterInfos(TArray<FMaterialParameterInfo>& OutParameterInfo) const {
    if (!HasParameterInfoSet(GetRuntimeProvider().getGame())) {
        for (const FMaterialParameterInfo& Info : ParameterInfos)
            OutParameterInfo.Add(Info);
        return;
    }
    for (const FMaterialParameterInfo& Info : ParameterInfoSet)
        OutParameterInfo.Add(Info);
}

const UStruct* FMaterialCachedExpressionData::GetParametersStruct() const {
    if (HasNestedParameters(GetRuntimeProvider().getGame()))
        return FMaterialCachedParameters::StaticStruct();
    return StaticStruct();
}

const void* FMaterialCachedExpressionData::GetParameters() const {
    if (HasNestedParameters(GetRuntimeProvider().getGame()))
        return &Parameters;
    return this;
}

const FMaterialCachedParameterEntry* FMaterialCachedExpressionData::GetParameterTypeEntry(EMaterialParameterType Type) const {
    const EGame Game = GetRuntimeProvider().getGame();
    const int32 Index = GetRuntimeEntryIndex(Type, Game);
    if (Index == INDEX_NONE)
        return nullptr;
    if (!HasNestedParameters(Game))
        return &RuntimeEntries[Index];
    return Game < EGame::GAME_UE4_26 ? &Parameters.Entries[Index] : &Parameters.RuntimeEntries[Index];
}

}
