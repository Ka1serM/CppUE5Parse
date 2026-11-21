#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FControlRigObjectVersion {
    static constexpr FGuid GUID{0xA7820CFB, 0x20A74359, 0x8C542C14, 0x9623CF50};
    enum Type : int32 {
        SwitchedToRigVM = 3,
        RigHierarchyMultiParentConstraints = 8,
        RigHierarchyControlSpaceFavorites = 9,
        StorageMinMaxValuesAsFloatStorage = 10,
        PerChannelLimits = 13,
        RemovedMultiParentParentCache = 14,
        CurveElementValueStateFlag = 16,
        ControlAnimationType = 17,
        PreferredEulerAnglesForControls = 19,
        HierarchyElementMetadata = 20,
        RestrictSpaceSwitchingForControls = 22,
        ControlTransformChannelFiltering = 23,
        StoreFunctionsInGeneratedClass = 24,
        RigHierarchyStoringPreviousNames = 25,
        RigHierarchyControlPreferredRotationOrder = 26,
        RigHierarchyControlPreferredRotationOrderFlag = 27,
        RigHierarchyStoresElementMetadata = 28,
        ConnectorsWithType = 29,
        RigHierarchyIndirectElementStorage = 32,
        RigHierarchyCompressElements = 33,
        RigHierarchyStoresComponents = 34,
        RigHierarchyCompactTransformSerialization = 35,
        RigHierarchyArrayConnectors = 36,
        RigHierarchyParentContraintWithLabel = 37,
        RigHierarchyPreviousNameAndParentMapUsingHierarchyKey = 38,
        RigHierarchyPostConstructionConnectors = 39,
        OverridesStoreTOCDataForProperties = 40,
        OverridesStoreDatSkipOffsetAsInt64 = 41,
        OverridesStorePathAndLeafPropertyOnly = 42,
        OverridesStoreLeafPropertyHashOnly = 43,
    };
};

}
