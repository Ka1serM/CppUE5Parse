#pragma once

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

struct FControlRigObjectVersion {
    static constexpr FGuid GUID{0xA7820CFB, 0x20A74359, 0x8C542C14, 0x9623CF50};
    enum Type : int32 {
        BeforeCustomVersionWasAdded = 0,
        RemovalOfHierarchyRefPins,
        OperatorsStoringPropertyPaths,
        SwitchedToRigVM,
        ControlOffsetTransform,
        RigElementKeyCache,
        BlueprintVariableSupport,
        RigHierarchyV2,
        RigHierarchyMultiParentConstraints,
        RigHierarchyControlSpaceFavorites,
        StorageMinMaxValuesAsFloatStorage,
        RenameGizmoToShape,
        BoundVariableWithInjectionNode,
        PerChannelLimits,
        RemovedMultiParentParentCache,
        RemoveParameters,
        CurveElementValueStateFlag,
        ControlAnimationType,
        TemplatesPreferredPermutatation,
        PreferredEulerAnglesForControls,
        HierarchyElementMetadata,
        LibraryNodeTemplates,
        RestrictSpaceSwitchingForControls,
        ControlTransformChannelFiltering,
        StoreFunctionsInGeneratedClass,
        RigHierarchyStoringPreviousNames,
        RigHierarchyControlPreferredRotationOrder,
        RigHierarchyControlPreferredRotationOrderFlag,
        RigHierarchyStoresElementMetadata,
        ConnectorsWithType,
        RigPoseWithParentKey,
        ControlRigStoresPhysicsSolvers,
        RigHierarchyIndirectElementStorage,
        RigHierarchyCompressElements,
        RigHierarchyStoresComponents,
        RigHierarchyCompactTransformSerialization,
        RigHierarchyArrayConnectors,
        RigHierarchyParentContraintWithLabel,
        RigHierarchyPreviousNameAndParentMapUsingHierarchyKey,
        RigHierarchyPostConstructionConnectors,
        OverridesStoreTOCDataForProperties,
        OverridesStoreDatSkipOffsetAsInt64,
        OverridesStorePathAndLeafPropertyOnly,
        OverridesStoreLeafPropertyHashOnly,
        VersionPlusOne,
        LatestVersion = VersionPlusOne - 1
    };

    static bool AtLeast(const FArchive& Ar, Type Version) { return Ar.CustomVerAtLeast(GUID, Version, Since(Version)); }

    //! The highest version the archive reaches, which UE reads as the custom version of the archive.
    static Type Get(const FArchive& Ar) {
        int32 Version = LatestVersion;
        while (Version > BeforeCustomVersionWasAdded && !AtLeast(Ar, static_cast<Type>(Version)))
            --Version;
        return static_cast<Type>(Version);
    }

private:
    static EGame Since(Type Version) {
        if (Version >= OverridesStoreDatSkipOffsetAsInt64)
            return EGame::GAME_UE5_8;
        if (Version >= RigHierarchyPostConstructionConnectors)
            return EGame::GAME_UE5_7;
        if (Version >= RigHierarchyCompressElements)
            return EGame::GAME_UE5_6;
        if (Version >= ControlRigStoresPhysicsSolvers)
            return EGame::GAME_UE5_5;
        if (Version >= RigHierarchyStoresElementMetadata)
            return EGame::GAME_UE5_4;
        if (Version >= RigHierarchyControlPreferredRotationOrder)
            return EGame::GAME_UE5_3;
        if (Version >= RestrictSpaceSwitchingForControls)
            return EGame::GAME_UE5_2;
        if (Version >= RemovedMultiParentParentCache)
            return EGame::GAME_UE5_1;
        if (Version >= RigHierarchyV2)
            return EGame::GAME_UE5_0;
        if (Version >= ControlOffsetTransform)
            return EGame::GAME_UE4_26;
        if (Version >= SwitchedToRigVM)
            return EGame::GAME_UE4_25;
        return EGame::GAME_UE4_23;
    }
};

}
