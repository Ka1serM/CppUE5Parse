#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectPtr.h"

#include "PaperFlipbook.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UPaperSprite;

USTRUCT()
struct FPaperFlipbookKeyFrame {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UPaperSprite> Sprite = nullptr;
    UPROPERTY()
    int32 FrameRun = 1;
};

namespace EFlipbookCollisionMode {

UENUM()
enum Type : int32 { NoCollision, FirstFrameCollision, EachFrameCollision };

}

UCLASS()
class UPaperFlipbook : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    float FramesPerSecond = 15.0f;
    UPROPERTY()
    TArray<FPaperFlipbookKeyFrame> KeyFrames;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> DefaultMaterial = nullptr;
    UPROPERTY()
    TEnumAsByte<EFlipbookCollisionMode::Type> CollisionSource = EFlipbookCollisionMode::NoCollision;
};

}
