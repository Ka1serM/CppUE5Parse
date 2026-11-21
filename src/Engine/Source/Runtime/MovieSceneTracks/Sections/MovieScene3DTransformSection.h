#pragma once

#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneDoubleChannel.h"
#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneFloatChannel.h"
#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneSectionChannelOverrideRegistry.h"
#include "Engine/Source/Runtime/MovieScene/MovieSceneSection.h"

#include "MovieScene3DTransformSection.generated.h"

namespace ue5parse::runtime {

enum class EMovieSceneTransformChannel : uint32 {
    None = 0x000,
    TranslationX = 0x001,
    TranslationY = 0x002,
    TranslationZ = 0x004,
    Translation = TranslationX | TranslationY | TranslationZ,
    RotationX = 0x008,
    RotationY = 0x010,
    RotationZ = 0x020,
    Rotation = RotationX | RotationY | RotationZ,
    ScaleX = 0x040,
    ScaleY = 0x080,
    ScaleZ = 0x100,
    Scale = ScaleX | ScaleY | ScaleZ,
    AllTransform = Translation | Rotation | Scale,
    Weight = 0x200,
    All = Translation | Rotation | Scale | Weight,
};

USTRUCT()
struct FMovieSceneTransformMask {
    GENERATED_BODY()

    UPROPERTY()
    uint32 Mask = 0;
};

UCLASS()
class UMovieScene3DTransformSectionConstraints : public UObject {
    GENERATED_BODY()
public:
    explicit UMovieScene3DTransformSectionConstraints(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UMovieScene3DTransformSection : public UMovieSceneSection {
    GENERATED_BODY()
public:
    explicit UMovieScene3DTransformSection(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FMovieSceneTransformMask TransformMask;
    UPROPERTY()
    FMovieSceneDoubleChannel Translation[3];
    UPROPERTY()
    FMovieSceneDoubleChannel Rotation[3];
    UPROPERTY()
    FMovieSceneDoubleChannel Scale[3];
    UPROPERTY()
    FMovieSceneFloatChannel ManualWeight;
    UPROPERTY()
    bool bUseQuaternionInterpolation = false;
    UPROPERTY()
    TObjectPtr<UMovieSceneSectionChannelOverrideRegistry> OverrideRegistry;
    UPROPERTY()
    TObjectPtr<UMovieScene3DTransformSectionConstraints> Constraints;
};

}
