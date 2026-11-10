#pragma once

#include "Engine/Source/Runtime/Core/Containers/UnrealString.h"
#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/Kismet/BlueprintFunctionLibrary.h"

#include "GameplayTasksNative.generated.h"

namespace ue5parse::runtime {

class AActor;
class ACharacter;
class AController;
class AGameStateBase;
class APawn;
class APlayerController;
class UActorComponent;
class UAkCallbackInfo;
class UDamageType;
class UDataTable;
class UInputComponent;
class ULegacyCameraShake;
class UMaterialInterface;
class UObject;
class UPrimitiveComponent;
class UStaticMesh;
class UUserWidget;

UCLASS()
class UGameplayTasksComponent : public UActorComponent {
    GENERATED_BODY()
public:
};

}
