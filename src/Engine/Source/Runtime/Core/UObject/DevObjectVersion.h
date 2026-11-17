#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FRenderingObjectVersion {
    static constexpr FGuid GUID{0x12F88B9F, 0x88754AFC, 0xA67CD90C, 0x383ABD29};
    enum Type : int32 {
        TextureStreamingMeshUVChannelData = 10,
        StaticMeshSectionForceOpaqueField = 37,
    };
};

struct FEditorObjectVersion {
    static constexpr FGuid GUID{0xE4B068ED, 0xF49442E9, 0xA231DA0B, 0x2E46BB41};
    enum Type : int32 {
        RefactorMeshEditorMaterials = 8,
    };
};

struct FFrameworkObjectVersion {
    static constexpr FGuid GUID{0xCFFC743F, 0x43B04480, 0x939114DF, 0x171D2073};
    enum Type : int32 {
        SmartNameRefactor = 7,
        MoveCurveTypesToSkeleton = 15,
    };
};

struct FAnimPhysObjectVersion {
    static constexpr FGuid GUID{0x29E575DD, 0xE0A34627, 0x9D10D276, 0x232CDCEA};
    enum Type : int32 {
        SmartNameRefactorForDeterministicCooking = 10,
        AddLODToCurveMetaData = 12,
    };
};

struct FAnimObjectVersion {
    static constexpr FGuid GUID{0xAF43A65D, 0x7FD34947, 0x98733E8E, 0xD9C1BB05};
    enum Type : int32 {
        StoreMarkerNamesOnSkeleton = 2,
    };
};

struct FUE5ReleaseStreamObjectVersion {
    static constexpr FGuid GUID{0xD89B5E42, 0x24BD4D46, 0x8412ACA8, 0xDF641779};
    enum Type : int32 {
        RemovingTessellation = 3,
        MaterialInterfaceSavedCachedData = 14,
        RemovedSmartNameContainerPayload = 64,
    };
};

struct FFortniteMainBranchObjectVersion {
    static constexpr FGuid GUID{0x601D1886, 0xAC644F84, 0xAA16D3DE, 0x0DEAC7D6};
    enum Type : int32 {
        MeshMaterialSlotOverlayMaterialAdded = 196,
    };
};

}
