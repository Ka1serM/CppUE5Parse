#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "FontImportOptions.generated.h"

namespace ue5parse::runtime {

UENUM()
enum EFontImportCharacterSet : int32 { FontICS_Default, FontICS_Ansi, FontICS_Symbol, FontICS_MAX };

USTRUCT()
struct FFontImportOptionsData {
    GENERATED_BODY()
    UPROPERTY()
    FString FontName = "Arial";
    UPROPERTY()
    float Height = 16.0f;
    UPROPERTY()
    uint8 bEnableAntialiasing : 1 = true;
    UPROPERTY()
    uint8 bEnableBold : 1 = false;
    UPROPERTY()
    uint8 bEnableItalic : 1 = false;
    UPROPERTY()
    uint8 bEnableUnderline : 1 = false;
    UPROPERTY()
    uint8 bAlphaOnly : 1 = false;
    UPROPERTY()
    TEnumAsByte<EFontImportCharacterSet> CharacterSet = FontICS_Default;
    UPROPERTY()
    FString Chars;
    UPROPERTY()
    FString UnicodeRange;
    UPROPERTY()
    FString CharsFilePath;
    UPROPERTY()
    FString CharsFileWildcard;
    UPROPERTY()
    uint8 bCreatePrintableOnly : 1 = false;
    UPROPERTY()
    uint8 bIncludeASCIIRange : 1 = true;
    UPROPERTY()
    FLinearColor ForegroundColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
    UPROPERTY()
    uint8 bEnableDropShadow : 1 = false;
    UPROPERTY()
    int32 TexturePageWidth = 256;
    UPROPERTY()
    int32 TexturePageMaxHeight = 256;
    UPROPERTY()
    int32 XPadding = 1;
    UPROPERTY()
    int32 YPadding = 1;
    UPROPERTY()
    int32 ExtendBoxTop = 0;
    UPROPERTY()
    int32 ExtendBoxBottom = 0;
    UPROPERTY()
    int32 ExtendBoxRight = 0;
    UPROPERTY()
    int32 ExtendBoxLeft = 0;
    UPROPERTY()
    uint8 bEnableLegacyMode : 1 = false;
    UPROPERTY()
    int32 Kerning = 0;
    UPROPERTY()
    uint8 bUseDistanceFieldAlpha : 1 = false;
    UPROPERTY()
    int32 DistanceFieldScaleFactor = 16;
    UPROPERTY()
    float DistanceFieldScanRadiusScale = 1.0f;
};

UCLASS()
class UFontImportOptions : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FFontImportOptionsData Data;
};

}
