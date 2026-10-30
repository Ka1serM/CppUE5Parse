#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/CoreUObject/Templates/SubclassOf.h"
#include "ue5parse/runtime/Core/Containers/UnrealString.h"
#include "ue5parse/runtime/Engine/Engine/DataTable.h"
#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"
#include "ue5parse/runtime/Engine/GameFramework/Actor.h"
#include "ue5parse/runtime/Engine/GameFramework/Character.h"
#include "ue5parse/runtime/Engine/GameFramework/GameModeBase.h"
#include "ue5parse/runtime/Engine/GameFramework/GameStateBase.h"
#include "ue5parse/runtime/Engine/GameFramework/Info.h"
#include "ue5parse/runtime/Engine/Components/ActorComponent.h"
#include "ue5parse/runtime/Engine/GameFramework/CharacterMovementComponent.h"
#include "ue5parse/runtime/Engine/GameFramework/ProjectileMovementComponent.h"
#include "ue5parse/runtime/Engine/Kismet/BlueprintFunctionLibrary.h"
#include "ue5parse/runtime/Engine/Kismet/KismetSystemLibrary.h"

#include "EngineNativeMissing.generated.h"

namespace ue5parse::runtime {

// FTableRowBase: the empty base of every row struct (UE's Engine/DataTable.h).
USTRUCT()
struct FTableRowBase { GENERATED_BODY() };


class AActor;
class ACharacter;
class AController;
class AGameStateBase;
class APawn;
class APlayerController;
class UActorComponent;
class UInputComponent;
class ULegacyCameraShake;
class UMaterialInterface;
class UObject;
class UPrimitiveComponent;
class UStaticMesh;
class UUserWidget;

UDELEGATE()
struct FMovementModeChangedSignature_DelegateSignature {
    static void Broadcast(ACharacter* Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode);
};

UDELEGATE()
struct FOnCinematicModeUpdated_DelegateSignature {
    static void Broadcast();
};

UDELEGATE()
struct FTakeAnyDamageSignature_DelegateSignature {
    static void Broadcast(AActor* DamagedActor, float Damage, UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
};

UDELEGATE()
struct FTimerDynamicDelegate_DelegateSignature {
    static void Broadcast();
};

UCLASS()
class UDataTableFunctionLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    // OutRow is the Blueprint's row struct, not an FTableRowBase: the thunk takes its address and checks its type.
    UFUNCTION(BlueprintCallable, CustomThunk)
    static bool GetDataTableRowFromName(UDataTable* Table, FName RowName, FTableRowBase& OutRow);
    static bool Generic_GetDataTableRowFromName(const UDataTable* Table, FName RowName, void* OutRowPtr);
    DECLARE_FUNCTION(execGetDataTableRowFromName);
    UFUNCTION(BlueprintCallable)
    static void GetDataTableRowNames(UDataTable* Table, TArray<FName>& OutRowNames) {
        if (Table != nullptr) OutRowNames = Table->GetRowNames();
    }
};

UCLASS()
class UKismetNodeHelperLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static FString GetEnumeratorUserFriendlyName(UObject* Enum, uint8 EnumeratorValue) {
        const UEnum* EnumType = Cast<UEnum>(Enum);
        return EnumType != nullptr ? EnumType->GetDisplayNameTextByIndex(EnumType->GetIndexByValue(EnumeratorValue)).ToString() : FString();
    }
};

UCLASS()
class UNetPushModelHelpers : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    static void MarkPropertyDirtyFromRepIndex(UObject* Object, int32 RepIndex, FName PropertyName) {  }
};

UCLASS()
class UDamageType : public UObject {
    GENERATED_BODY()
public:
};

UCLASS()
class USubsystem : public UObject {
    GENERATED_BODY()
public:
};

UCLASS()
class UGameInstanceSubsystem : public USubsystem {
    GENERATED_BODY()
public:
};

UCLASS()
class AGameMode : public AGameModeBase {
    GENERATED_BODY()
public:
};

UCLASS()
class AGameState : public AGameStateBase {
    GENERATED_BODY()
public:
};

UCLASS()
class UInputBindingComponent : public UActorComponent {
    GENERATED_BODY()
public:
};

}
