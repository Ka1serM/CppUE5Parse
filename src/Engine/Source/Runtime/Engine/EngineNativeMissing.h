#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Character.h"
#include "Engine/Source/Runtime/Engine/GameFramework/GameModeBase.h"
#include "Engine/Source/Runtime/Engine/GameFramework/GameStateBase.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"
#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"
#include "Engine/Source/Runtime/Engine/GameFramework/CharacterMovementComponent.h"
#include "Engine/Source/Runtime/Engine/GameFramework/ProjectileMovementComponent.h"
#include "Engine/Source/Runtime/Engine/Kismet/BlueprintFunctionLibrary.h"
#include "Engine/Source/Runtime/Engine/Kismet/KismetSystemLibrary.h"

#include "EngineNativeMissing.generated.h"

namespace ue5parse::runtime {

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

UCLASS(abstract)
class ATriggerBase : public AActor {
    GENERATED_BODY()
public:
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
