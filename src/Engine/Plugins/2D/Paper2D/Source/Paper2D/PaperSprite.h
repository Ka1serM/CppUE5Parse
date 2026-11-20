#pragma once

#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/SpriteEditorOnlyTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PaperSprite.generated.h"

namespace ue5parse::runtime {

class UBodySetup;
class UMaterialInterface;
class UTexture;
class UTexture2D;

USTRUCT()
struct FPaperSpriteSocket {
    GENERATED_BODY()
    UPROPERTY()
    FTransform LocalTransform;
    UPROPERTY()
    FName SocketName;
};

UCLASS()
class UPaperSprite : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FVector2D SourceDimension = FVector2D::ZeroVector;
    UPROPERTY()
    TObjectPtr<UTexture2D> SourceTexture = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UTexture>> AdditionalSourceTextures;
    UPROPERTY()
    FVector2D BakedSourceUV = FVector2D::ZeroVector;
    UPROPERTY()
    FVector2D BakedSourceDimension = FVector2D::ZeroVector;
    UPROPERTY()
    TObjectPtr<UTexture2D> BakedSourceTexture = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> DefaultMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> AlternateMaterial = nullptr;
    UPROPERTY()
    TArray<FPaperSpriteSocket> Sockets;
    UPROPERTY()
    TEnumAsByte<ESpriteCollisionMode::Type> SpriteCollisionDomain = ESpriteCollisionMode::Use3DPhysics;
    UPROPERTY()
    float PixelsPerUnrealUnit = 2.56f;
    UPROPERTY()
    TObjectPtr<UBodySetup> BodySetup = nullptr;
    UPROPERTY()
    int32 AlternateMaterialSplitIndex = INDEX_NONE;
    UPROPERTY()
    TArray<FVector4> BakedRenderData;
};

}
