#pragma once

#include "ue5parse/runtime/Core/Serialization/Archive.h"

// FStripDataFlags (UE's EngineUtils.h): what the cook left out of an object's native data.

namespace ue5parse::runtime {

class FStripDataFlags {
public:
    enum EStrippedData : uint8 {
        None = 0,
        Editor = 1,
        Server = 2,
        AllPlatforms = 0xff,
    };

    // Reads the global and the class flags.
    explicit FStripDataFlags(FArchive& Ar) { Ar << GlobalStripFlags << ClassStripFlags; }

    bool IsEditorDataStripped() const { return (GlobalStripFlags & Editor) != 0; }
    bool IsAudioVisualDataStripped() const { return (GlobalStripFlags & Server) != 0; }
    bool IsDataStrippedForServer() const { return IsAudioVisualDataStripped(); }
    bool IsClassDataStripped(uint8 InFlags) const { return (ClassStripFlags & InFlags) != 0; }

private:
    uint8 GlobalStripFlags = 0;
    uint8 ClassStripFlags = 0;
};

}
