#pragma once

#include "Engine/Source/Runtime/Core/Misc/Optional.h"
#include "Engine/Source/Runtime/SlateCore/Fonts/CompositeFont.h"
#include "Engine/Source/Runtime/SlateCore/Fonts/FontRasterizationMode.h"

#include "FontFace.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FFontFacePlatformRasterizationOverrides {
    GENERATED_BODY()
    UPROPERTY()
    EFontRasterizationMode MsdfOverride = EFontRasterizationMode::Msdf;
    UPROPERTY()
    EFontRasterizationMode SdfOverride = EFontRasterizationMode::Sdf;
    UPROPERTY()
    EFontRasterizationMode SdfApproximationOverride = EFontRasterizationMode::SdfApproximation;
};

UCLASS()
class UFontFace : public UObject {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;
    const FFontFaceData& GetFontFaceData() const { return FontFaceData; }

    UPROPERTY()
    FString SourceFilename;
    UPROPERTY()
    EFontHinting Hinting = EFontHinting::Default;
    UPROPERTY()
    EFontLoadingPolicy LoadingPolicy = EFontLoadingPolicy::LazyLoad;
    UPROPERTY()
    EFontLayoutMethod LayoutMethod = EFontLayoutMethod::Metrics;
    UPROPERTY()
    int32 AscendOverriddenValue = 0;
    UPROPERTY()
    bool bIsAscendOverridden = false;
    UPROPERTY()
    int32 DescendOverriddenValue = 0;
    UPROPERTY()
    bool bIsDescendOverridden = false;
    UPROPERTY()
    int32 StrikeBrushHeightPercentage = 0;
    UPROPERTY()
    TArray<uint8> FontFaceData_DEPRECATED;
    UPROPERTY(Transient)
    TArray<FString> SubFaces;
    UPROPERTY()
    bool bEnableDistanceFieldRendering = false;
    UPROPERTY()
    int32 MinDistanceFieldPpem = 32;
    UPROPERTY()
    int32 MidDistanceFieldPpem = 48;
    UPROPERTY()
    int32 MaxDistanceFieldPpem = 64;
    UPROPERTY()
    int32 MinMultiDistanceFieldPpem = 32;
    UPROPERTY()
    int32 MidMultiDistanceFieldPpem = 40;
    UPROPERTY()
    int32 MaxMultiDistanceFieldPpem = 56;
    UPROPERTY()
    TOptional<FFontFacePlatformRasterizationOverrides> PlatformRasterizationModeOverrides;

private:
    FFontFaceData FontFaceData;
};

}
