#pragma once

#include "GameTypes/OuterWorlds2/PropertyDataWithObjects.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"
#include "Engine/Source/Runtime/GameplayTags/GameplayTagContainer.h"

#include "AffinityTable.generated.h"

namespace ue5parse::runtime {

//! The cells of one struct of the table, row by row, with one cell for each pair of a row tag and a column tag.
USTRUCT()
struct FAffinityTableStructCells {
    GENERATED_BODY()
    UPROPERTY()
    FString StructName;
    UPROPERTY()
    TArray<FInstancedStruct> Cells;
};

USTRUCT()
struct FAffinityTableTagPair {
    GENERATED_BODY()
    UPROPERTY()
    FName First;
    UPROPERTY()
    FName Second;

    friend FArchive& operator<<(FArchive& Ar, FAffinityTableTagPair& Pair) { return Ar << Pair.First << Pair.Second; }
};

USTRUCT()
struct FAffinityTableTags {
    GENERATED_BODY()
    UPROPERTY()
    TMap<FString, FAffinityTableTagPair> Tags;

    friend FArchive& operator<<(FArchive& Ar, FAffinityTableTags& Value) { return Ar << Value.Tags; }
};

//! Nightingale saves the cells of each struct of the table and the row, column and tag maps after the properties.
UCLASS()
class UAffinityTable : public UObject {
    GENERATED_BODY()
public:
    explicit UAffinityTable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TArray<FGameplayTag> RowTags;
    UPROPERTY()
    TArray<FGameplayTag> ColumnTags;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> Structures;
    UPROPERTY()
    TArray<FAffinityTableStructCells> Tables;
    UPROPERTY()
    TMap<FName, FVector4f> Rows;
    UPROPERTY()
    TMap<FName, FVector4f> Columns;
    UPROPERTY()
    TMap<FName, FAffinityTableTags> Tags;
};

}
