#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/NoExportTypes.h"
#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/CoreUObject/Templates/SubclassOf.h"
#include "ue5parse/runtime/Core/Containers/UnrealString.h"
#include "ue5parse/runtime/Engine/Components/ActorComponent.h"
#include "ue5parse/runtime/Engine/Components/SceneComponent.h"
#include "ue5parse/runtime/Engine/Kismet/BlueprintFunctionLibrary.h"

#include "AkAudioNative.generated.h"

namespace ue5parse::runtime {

enum class EAkCallbackType : uint8 {};

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

UDELEGATE()
struct FOnAkPostEventCallback_DelegateSignature {
    static void Broadcast(uint8 UnderlyingType, EAkCallbackType CallbackType, UAkCallbackInfo* CallbackInfo);
};

UCLASS()
class UAkGameObject : public USceneComponent {
    GENERATED_BODY()
public:
};

UCLASS()
class UAkComponent : public UAkGameObject {
    GENERATED_BODY()
public:
};

}
