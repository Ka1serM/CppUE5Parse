#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FStripDataFlags {
public:
    enum EStrippedData : uint8 {
        None = 0,
        Editor = 1,
        Server = 2,
        AllPlatforms = 0xff,
    };

    explicit FStripDataFlags(FArchive& Ar) { Ar << GlobalStripFlags << ClassStripFlags; }

    bool IsEditorDataStripped() const { return (GlobalStripFlags & Editor) != 0; }
    bool IsAudioVisualDataStripped() const { return (GlobalStripFlags & Server) != 0; }
    bool IsClassDataStripped(uint8 InFlags) const { return (ClassStripFlags & InFlags) != 0; }

private:
    uint8 GlobalStripFlags = 0;
    uint8 ClassStripFlags = 0;
};

}
