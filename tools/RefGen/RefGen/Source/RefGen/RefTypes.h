#pragma once

#if __has_include("CoreMinimal.h")
#include "CoreMinimal.h"
#else
#include "Engine.h" // <4.15
#endif
#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION == 15
#include "Engine.h" // 4.15's first IWYU headers miss some transitive includes (FRuntimeFloatCurve) under MSVC
#endif
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "RefTypes.generated.h"

UENUM(BlueprintType)
enum class ERefNativeEnum : uint8
{
	First,
	Second,
	Third
};

UENUM(BlueprintType, meta = (Bitflags))
enum class ERefFlags : uint8
{
	None = 0,
	A = 1,
	B = 2,
	C = 4
};

USTRUCT(BlueprintType)
struct FRefInnerStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<float> Weights;
};

USTRUCT(BlueprintType)
struct FRefTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Damage = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Range = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERefNativeEnum Kind = ERefNativeEnum::First;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UObject> Asset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag Tag;
};

UCLASS(BlueprintType)
class REFGEN_API URefDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bFlag = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) uint8 Byte = 0;
	UPROPERTY(EditAnywhere) int8 Int8 = 0;
	UPROPERTY(EditAnywhere) int16 Int16 = 0;
	UPROPERTY(EditAnywhere) uint16 UInt16 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Int32 = 0;
	UPROPERTY(EditAnywhere) uint32 UInt32 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int64 Int64 = 0;
	UPROPERTY(EditAnywhere) uint64 UInt64 = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Float = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) double Double = 0.0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString String;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ERefNativeEnum Enum = ERefNativeEnum::First;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/RefGen.ERefFlags")) int32 Flags = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Vector = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector2D Vector2D = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector4 Vector4 = FVector4(0, 0, 0, 0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRotator Rotator = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FQuat Quat = FQuat::Identity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FTransform Transform;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FLinearColor LinearColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FColor Color = FColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FBox Box = FBox(ForceInit);
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGuid Guid;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FDateTime DateTime;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FTimespan Timespan;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FSoftObjectPath SoftPath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FSoftClassPath SoftClassPath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UObject> HardObject;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftObjectPtr<UObject> SoftObject;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AActor> ActorClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSoftClassPtr<AActor> SoftActorClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TWeakObjectPtr<UObject> WeakObject;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTag Tag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FGameplayTagContainer Tags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRefInnerStruct Inner;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<int32> IntArray;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FString> StringArray;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FRefInnerStruct> StructArray;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<UObject>> ObjectArray;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSet<FName> NameSet;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FName, int32> NameToInt;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<FString, FRefInnerStruct> StringToStruct;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TMap<int32, TSoftObjectPtr<UObject>> IntToSoft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FDataTableRowHandle RowHandle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRuntimeFloatCurve RuntimeCurve;
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite) TObjectPtr<UObject> InstancedSubobject;
	UPROPERTY(EditAnywhere) int32 StaticArray[3] = { 0, 0, 0 };
};

UCLASS(EditInlineNew, DefaultToInstanced, BlueprintType)
class REFGEN_API URefInstancedObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Value = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FString Note;
};

UCLASS()
class REFGEN_API URefAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayAttributeData Health;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayAttributeData Armor;
};

UCLASS(Blueprintable)
class REFGEN_API ARefNativeActor : public AActor
{
	GENERATED_BODY()

public:
	ARefNativeActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<AActor>> LinkedActors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FRefInnerStruct Settings;
};
