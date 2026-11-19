#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime::UE::Slate {

class FPreprocessedFontGeometry {
public:
    friend FArchive& operator<<(FArchive& Ar, FPreprocessedFontGeometry& FontGeometry) {
        return Ar << FontGeometry.GlobalWindingReversal << FontGeometry.Glyphs << FontGeometry.ContourData << FontGeometry.CoordinateData;
    }

private:
    struct FGlyphHeader {
        uint8 Flags = 0;
        int32 ContourCount = 0;
        int32 ContourDataStart = 0;
        int32 ContourDataLength = 0;
        int32 CoordinateDataStart = 0;
        int32 CoordinateDataLength = 0;

        friend FArchive& operator<<(FArchive& Ar, FGlyphHeader& GlyphHeader) {
            return Ar << GlyphHeader.Flags << GlyphHeader.ContourCount << GlyphHeader.ContourDataStart << GlyphHeader.ContourDataLength
                      << GlyphHeader.CoordinateDataStart << GlyphHeader.CoordinateDataLength;
        }
    };

    bool GlobalWindingReversal = false;
    TMap<int32, FGlyphHeader> Glyphs;
    TArray<uint8> ContourData;
    TArray<int16> CoordinateData;
};

}
