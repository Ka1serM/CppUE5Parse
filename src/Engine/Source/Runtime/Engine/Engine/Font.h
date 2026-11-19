#pragma once

#include "Engine/Source/Runtime/Engine/Engine/FontImportOptions.h"
#include "Engine/Source/Runtime/SlateCore/Fonts/CompositeFont.h"
#include "Engine/Source/Runtime/SlateCore/Fonts/FontRasterizationMode.h"
#include "Engine/Source/Runtime/SlateCore/Fonts/FontSdfSettings.h"

#include "Font.generated.h"

namespace ue5parse::runtime {

class UTexture2D;

UENUM()
enum class EFontCacheType : uint8 { Offline, Runtime };

UENUM()
enum class ERuntimeFontSource : uint8 { Asset, CoreStyleDefault };

USTRUCT()
struct FFontCharacter {
    GENERATED_BODY()
    UPROPERTY()
    int32 StartU = 0;
    UPROPERTY()
    int32 StartV = 0;
    UPROPERTY()
    int32 USize = 0;
    UPROPERTY()
    int32 VSize = 0;
    UPROPERTY()
    uint8 TextureIndex = 0;
    UPROPERTY()
    int32 VerticalOffset = 0;

    bool Serialize(FArchive& Ar) {
        Ar << StartU << StartV << USize << VSize << TextureIndex << VerticalOffset;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FFontCharacter> : TStructOpsTypeTraitsBase2<FFontCharacter> {
    enum { WithSerializer = true };
};

UCLASS()
class UFont : public UObject {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    EFontCacheType FontCacheType = EFontCacheType::Offline;
    UPROPERTY()
    EFontRasterizationMode FontRasterizationMode = EFontRasterizationMode::Bitmap;
    UPROPERTY()
    FFontSdfSettings SdfSettings;
    UPROPERTY()
    ERuntimeFontSource RuntimeFontSource = ERuntimeFontSource::Asset;
    UPROPERTY()
    TArray<FFontCharacter> Characters;
    UPROPERTY()
    TArray<TObjectPtr<UTexture2D>> Textures;
    UPROPERTY()
    int32 IsRemapped = 0;
    UPROPERTY()
    float EmScale = 0.0f;
    UPROPERTY()
    float Ascent = 0.0f;
    UPROPERTY()
    float Descent = 0.0f;
    UPROPERTY()
    float Leading = 0.0f;
    UPROPERTY()
    int32 Kerning = 0;
    UPROPERTY()
    FFontImportOptionsData ImportOptions;
    UPROPERTY(Transient)
    int32 NumCharacters = 0;
    UPROPERTY(Transient)
    TArray<int32> MaxCharHeight;
    UPROPERTY()
    float ScalingFactor = 1.0f;
    UPROPERTY()
    int32 LegacyFontSize = 9;
    UPROPERTY()
    FName LegacyFontName;
    UPROPERTY()
    FCompositeFont CompositeFont;

    TMap<uint16, uint16> CharRemap;
};

}
