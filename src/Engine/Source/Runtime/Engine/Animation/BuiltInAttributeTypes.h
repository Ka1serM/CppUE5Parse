#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "BuiltInAttributeTypes.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FFloatAnimationAttribute {
    GENERATED_BODY()
    UPROPERTY()
    float Value = 0;
};

USTRUCT()
struct FIntegerAnimationAttribute {
    GENERATED_BODY()
    UPROPERTY()
    int32 Value = 0;
};

USTRUCT()
struct FStringAnimationAttribute {
    GENERATED_BODY()
    UPROPERTY()
    FString Value;
};

USTRUCT()
struct FTransformAnimationAttribute {
    GENERATED_BODY()
    UPROPERTY()
    FTransform Value;
};

USTRUCT()
struct FVectorAnimationAttribute {
    GENERATED_BODY()
    UPROPERTY()
    FVector Value;
};

USTRUCT()
struct FQuaternionAnimationAttribute {
    GENERATED_BODY()
    UPROPERTY()
    FQuat Value;
};

USTRUCT()
struct FNonBlendableQuaternionAnimationAttribute : public FQuaternionAnimationAttribute {
    GENERATED_BODY()
};

USTRUCT()
struct FNonBlendableVectorAnimationAttribute : public FVectorAnimationAttribute {
    GENERATED_BODY()
};

USTRUCT()
struct FNonBlendableTransformAnimationAttribute : public FTransformAnimationAttribute {
    GENERATED_BODY()
};

USTRUCT()
struct FNonBlendableFloatAnimationAttribute : public FFloatAnimationAttribute {
    GENERATED_BODY()
};

USTRUCT()
struct FNonBlendableIntegerAnimationAttribute : public FIntegerAnimationAttribute {
    GENERATED_BODY()
};

}
