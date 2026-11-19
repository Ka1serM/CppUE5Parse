#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"
#include "Engine/Source/Runtime/SlateCore/Fonts/PreprocessedFontGeometry.h"

#include "CompositeFont.generated.h"

namespace ue5parse::runtime {

class UFontBulkData;

UENUM()
enum class EFontHinting : uint8 { Default, Auto, AutoLight, Monochrome, None };

UENUM()
enum class EFontLoadingPolicy : uint8 { LazyLoad, Stream, Inline };

UENUM()
enum class EFontLayoutMethod : uint8 { Metrics, BoundingBox };

struct FFontFaceData {
    TArray<uint8> Data;
    TArray<UE::Slate::FPreprocessedFontGeometry> PreprocessedFontGeometries;

    void Serialize(FArchive& Ar);
};

USTRUCT()
struct FFontData {
    GENERATED_BODY()
    UPROPERTY()
    FString FontFilename;
    UPROPERTY()
    EFontHinting Hinting = EFontHinting::Default;
    UPROPERTY()
    EFontLoadingPolicy LoadingPolicy = EFontLoadingPolicy::LazyLoad;
    UPROPERTY()
    int32 SubFaceIndex = 0;
    UPROPERTY()
    TObjectPtr<const UObject> FontFaceAsset = nullptr;
    UPROPERTY()
    TObjectPtr<UFontBulkData> BulkDataPtr_DEPRECATED = nullptr;
    UPROPERTY()
    TArray<uint8> FontData_DEPRECATED;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FFontData> : TStructOpsTypeTraitsBase2<FFontData> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FTypefaceEntry {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FFontData Font;
};

USTRUCT()
struct FTypeface {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FTypefaceEntry> Fonts;
};

USTRUCT()
struct FCompositeFallbackFont {
    GENERATED_BODY()
    UPROPERTY()
    FTypeface Typeface;
    UPROPERTY()
    float ScalingFactor = 1.0f;
};

USTRUCT()
struct FCompositeSubFont : public FCompositeFallbackFont {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FInt32Range> CharacterRanges;
    UPROPERTY()
    FString Cultures;
    UPROPERTY()
    FName EditorName;
};

USTRUCT()
struct FCompositeFont {
    GENERATED_BODY()
    UPROPERTY()
    FTypeface DefaultTypeface;
    UPROPERTY()
    FCompositeFallbackFont FallbackTypeface;
    UPROPERTY()
    TArray<FCompositeSubFont> SubTypefaces;
    UPROPERTY()
    bool bEnableAscentDescentOverride = true;
};

}
